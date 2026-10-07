#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <array>
#include <atomic>
#include <cstring>
#include <fstream>
#include <sstream>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "native.hpp"

namespace fc {
namespace {
uintptr_t image;
bool active = false, remove_centre = true;
std::mutex mutex;
std::unordered_map<void*,Limits> facilities;
std::unordered_map<void*,uint32_t> building_keys;
std::unordered_map<void*,void*> actor_classes;
std::array<std::atomic<uint64_t>,3> repaired_counts{};
struct CloneSource { Limits limits; void* actor_class; };
thread_local std::optional<CloneSource> cloning;
std::filesystem::path save_root;
struct Snapshot { std::filesystem::path save; std::string settings; };
std::unordered_map<void*,Snapshot> pending_saves;
std::atomic<uint64_t> generation{};
thread_local void* emitting = nullptr;
// This marker exists only in queued throw arguments. It never reaches saved
// objects: the visual hook converts it to the game's ordinary MovePattern 2.
constexpr int custom_flight = 0x4643;
template<class T> T& field(void* p, size_t offset) { return *reinterpret_cast<T*>(static_cast<char*>(p)+offset); }
template<class T> T address(uintptr_t va) { return reinterpret_cast<T>(image+va-0x140000000); }
using Emit = Vec*(*)(void*,Vec*);
using Catch = Vec*(*)(void*,Vec*);
using Reach = bool(*)(void*,const Vec*);
using Select = void*(*)(void*,void*,void*,int);
using Launch = bool(*)(void*,void*,int);
using Receive = void(*)(void*,int,void*,void*,const Vec*,double,void*,int);
using Visual = void(*)(void*,void*,void*,void*,double,void*,int,bool);
using Worker = void(*)(void*);
Reach original_reach;
Select original_select;
Launch original_launch;
std::array<Receive,5> original_receive{};
Visual original_visual;
Worker original_hopper;
using SaveBegin = void(*)(void*,void*,void*,int);
SaveBegin original_save_begin;
Worker original_save_complete;
using Reload = void(*)(void*,void*,void*,bool);
Reload original_reload;
using Clone = bool(*)(void*,const Vec*,uint8_t,void*,bool);
Clone original_clone;
using Build = void*(*)(void*,void*,void*,void*,bool,uint64_t,const Vec*,bool,uint8_t,bool,void*);
Build original_build;
bool clone(void* controller,const Vec* position,uint8_t rotation,void* command,bool preview) {
    auto previous=cloning;
    cloning.reset();
    if (!preview) {
        uint32_t source=field<uint32_t>(command,0x1ac);
        std::scoped_lock lock(mutex);
        for (auto [facility,key]:building_keys) if (key==source) {
            cloning=CloneSource{facilities.at(facility),actor_classes.at(facility)}; break;
        }
    }
    bool result=original_clone(controller,position,rotation,command,preview);
    cloning=previous;
    return result;
}
void* build(void* subsystem,void* out,void* controller,void* variant,bool a,uint64_t area,
            const Vec* position,bool b,uint8_t active_index,bool c,void* extension) {
    void* result=original_build(subsystem,out,controller,variant,a,area,position,b,active_index,c,extension);
    if (cloning && field<bool>(out,0x14)) {
        void* actor=field<void*>(out,0x20);
        if (actor && field<void*>(actor,0x10)==cloning->actor_class) {
            set_facility(field<void*>(actor,0x2f8),cloning->limits,field<uint32_t>(actor,0x2c4),actor);
            log("Copied cloned fountain settings");
        }
    }
    return result;
}
void reload(void* subsystem,void* world,void* slot,bool sandbox) {
    original_reload(subsystem,world,slot,sandbox);
    reset();
    ++generation;
}
std::wstring unreal_string(void* p) {
    int count=field<int>(p,8);
    if (count<=0 || count>32768) return {};
    return {field<wchar_t*>(p,0),static_cast<size_t>(count-1)};
}
void save_begin(void* action,void* info,void* slot,int user) {
    {
        std::scoped_lock lock(mutex);
        if (!save_root.empty()) {
            std::ostringstream json;
            json << '{'; bool first=true;
            for (auto [facility,limits]:facilities) {
                auto key=building_keys.find(facility);
                if (key==building_keys.end()) continue;
                if (!first) json << ','; first=false;
                json << '"' << key->second << "\":{\"drop\":" << limits.drop << ",\"horizontal\":";
                if (limits.horizontal<0) json << "null"; else json << limits.horizontal;
                json << '}';
            }
            json << '}';
            auto path=save_root / unreal_string(slot);
            path += L".sav";
            pending_saves[action]={path,json.str()};
        }
    }
    original_save_begin(action,info,slot,user);
}
void save_complete(void* context) {
    std::optional<Snapshot> snapshot;
    {
        std::scoped_lock lock(mutex);
        auto found=pending_saves.find(field<void*>(context,0));
        if (found!=pending_saves.end()) { snapshot=std::move(found->second); pending_saves.erase(found); }
    }
    // This callback receives the actual SaveGameToSlot result, after the file
    // writer has closed the save. A failed save never gets a new companion.
    if (snapshot && field<bool>(context,0x1c)) {
        auto hash=fingerprint(snapshot->save);
        if (!hash.empty()) {
            auto companion=snapshot->save; companion.replace_extension(L".fountains.json");
            auto temporary=companion; temporary+=L".tmp";
            std::ofstream file(temporary,std::ios::binary|std::ios::trunc);
            file << "{\"schema\":1,\"fingerprint\":\"" << hash << "\",\"fountains\":" << snapshot->settings << "}\n";
            file.close();
            if (file && MoveFileExW(temporary.c_str(),companion.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
                log("Save companion committed");
            else log("Failed to write save companion; native save remains usable");
        } else log("Completed save could not be fingerprinted; companion skipped");
    }
    original_save_complete(context);
}

std::optional<Limits> limits_for(void* f) {
    std::scoped_lock lock(mutex);
    auto it = facilities.find(f);
    if (it == facilities.end()) return {};
    return it->second;
}
Vec delta_for(void* f, const Vec& target) {
    Vec origin;
    address<Emit>(0x144a1c6c0)(f,&origin);
    return {target.x-origin.x,target.y-origin.y,target.z-origin.z};
}
bool reach(void* f, const Vec* target) {
    auto limits = limits_for(f);
    if (!limits) return original_reach(f,target);
    return solve(delta_for(f,*target),*limits,remove_centre).has_value();
}
void* select(void* f,void* game,void* item,int count) {
    if (!limits_for(f)) return original_select(f,game,item,count);
    int attempts = std::clamp(field<int>(f,0x290),0,100000);
    for (int i=0; i<attempts; ++i) {
        void* receiver = original_select(f,game,item,count);
        if (!receiver) return nullptr;
        Vec target;
        auto vt = field<void**>(receiver,0);
        reinterpret_cast<Catch>(vt[0x568/8])(receiver,&target);
        if (reach(f,&target)) return receiver;
    }
    return nullptr;
}
bool launch(void* context,void* entity,int index) {
    void* previous = emitting;
    emitting = field<void*>(context,0);
    bool result = original_launch(context,entity,index);
    emitting = previous;
    return result;
}
template<size_t I> void receive(void* receiver,int index,void* game,void* entity,
                               const Vec* from,double speed,void* mesh,int pattern) {
    auto limits = limits_for(emitting);
    if (limits && pattern == 2 && remove_centre) {
        Vec target;
        auto vt = field<void**>(receiver,0);
        reinterpret_cast<Catch>(vt[0x568/8])(receiver,&target);
        auto flight = solve({target.x-from->x,target.y-from->y,target.z-from->z},*limits,true);
        if (flight && flight->repaired) { speed=flight->time; pattern=custom_flight; ++repaired_counts[0]; }
    }
    original_receive[I](receiver,index,game,entity,from,speed,mesh,pattern);
}
void visual(void* game,void* index,void* from,void* to,double speed,void* callback,int pattern,bool ignore) {
    bool custom = pattern == custom_flight;
    // Pattern 0 interprets Speed directly as duration; it avoids division by
    // horizontal distance, including exactly vertical throws.
    original_visual(game,index,from,to,speed,callback,custom ? 0 : pattern,ignore);
    if (!custom) return;
    int n = field<int>(index,4);
    if (n < 0 || n >= field<int>(game,0x2d8)) return;
    void* entity = static_cast<char*>(field<void*>(game,0x2d0))+n*0xf0;
    if (std::memcmp(static_cast<char*>(entity)+0x30,index,8) != 0) return;
    // Transforms are FTransform (double); translation begins at +0x20.
    double dz = field<double>(to,0x30)-field<double>(from,0x30);
    field<int>(entity,0x1c)=2;
    field<double>(entity,0x20)=dz/speed+490*speed;
    field<double>(entity,0x28)=speed;
    ++repaired_counts[1];
}
void hopper(void* context) {
    bool custom = field<int>(context,0xa8) == custom_flight;
    void* component = custom ? field<void*>(field<void*>(context,0x78),0xd0) : nullptr;
    int before = component ? field<int>(component,0x270) : 0;
    double duration = custom ? field<double>(context,0x98) : 0;
    original_hopper(context);
    if (custom && field<int>(component,0x270) == before+1) {
        void* entry = static_cast<char*>(field<void*>(component,0x268))+before*0x28;
        field<double>(entry,0x18)=duration;
        ++repaired_counts[2];
    }
}
struct Hook { uintptr_t va; void* detour; void** original; };
}

std::string fingerprint(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);
    if (!file) return {};
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    if (BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0) return {};
    bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    std::array<char,65536> bytes;
    while (ok && file) {
        file.read(bytes.data(),bytes.size());
        if (file.gcount()) ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(file.gcount()),0)>=0;
    }
    std::array<unsigned char,32> digest{};
    ok=ok && file.eof() && BCryptFinishHash(hash,digest.data(),digest.size(),0)>=0;
    if (hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm,0);
    if (!ok) return {};
    std::string result;
    for (auto b:digest) { result += "0123456789abcdef"[b>>4]; result += "0123456789abcdef"[b&15]; }
    return result;
}
void initialize(bool remove) {
    remove_centre=remove;
    wchar_t path[32768];
    GetModuleFileNameW(nullptr,path,32768);
    if (fingerprint(path)!="66618d029654d125723c95e8e62919f267973ad881e24abff3b80e0b355a52f8") {
        log("Unsupported game executable; native hooks disabled"); return;
    }
    image=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    std::vector<Hook> hooks {
        {0x144a1e1d0,reinterpret_cast<void*>(&reach),reinterpret_cast<void**>(&original_reach)},
        {0x144a19500,reinterpret_cast<void*>(&select),reinterpret_cast<void**>(&original_select)},
        {0x144a073d0,reinterpret_cast<void*>(&launch),reinterpret_cast<void**>(&original_launch)},
        {0x1449fae30,reinterpret_cast<void*>(&visual),reinterpret_cast<void**>(&original_visual)},
        {0x144a0d760,reinterpret_cast<void*>(&hopper),reinterpret_cast<void**>(&original_hopper)},
        {0x144a14120,reinterpret_cast<void*>(&receive<0>),reinterpret_cast<void**>(&original_receive[0])},
        {0x144a14380,reinterpret_cast<void*>(&receive<1>),reinterpret_cast<void**>(&original_receive[1])},
        {0x144a145f0,reinterpret_cast<void*>(&receive<2>),reinterpret_cast<void**>(&original_receive[2])},
        {0x144a14790,reinterpret_cast<void*>(&receive<3>),reinterpret_cast<void**>(&original_receive[3])},
        {0x144a149c0,reinterpret_cast<void*>(&receive<4>),reinterpret_cast<void**>(&original_receive[4])},
        {0x144acd490,reinterpret_cast<void*>(&save_begin),reinterpret_cast<void**>(&original_save_begin)},
        {0x144aaae90,reinterpret_cast<void*>(&save_complete),reinterpret_cast<void**>(&original_save_complete)},
        {0x144acb850,reinterpret_cast<void*>(&reload),reinterpret_cast<void**>(&original_reload)},
        {0x144ac84c0,reinterpret_cast<void*>(&clone),reinterpret_cast<void**>(&original_clone)},
        {0x144ad34d0,reinterpret_cast<void*>(&build),reinterpret_cast<void**>(&original_build)},
    };
    if (MH_Initialize()!=MH_OK) { log("MinHook initialization failed"); return; }
    for (auto h:hooks) {
        if (MH_CreateHook(address<void*>(h.va),h.detour,h.original)!=MH_OK) {
            log("Hook creation failed; all FountainControl hooks disabled");
            for (auto candidate:hooks) MH_RemoveHook(address<void*>(candidate.va));
            return;
        }
    }
    for (auto h:hooks) MH_QueueEnableHook(address<void*>(h.va));
    if (MH_ApplyQueued()!=MH_OK) {
        for (auto h:hooks) MH_DisableHook(address<void*>(h.va));
        log("Hook activation failed; hooks disabled"); return;
    }
    active=true;
    log("Verified Alchemy Factory 1.0.4963; all fountain hooks enabled");
}
bool ready() { return active; }
uint64_t load_generation() { return generation.load(); }
void set_facility(void* f,Limits limits,uint32_t key,void* actor) {
    if (!active) return;
    bool changed;
    {
        std::scoped_lock lock(mutex);
        auto old=facilities.find(f);
        changed=old==facilities.end() || old->second.drop!=limits.drop || old->second.horizontal!=limits.horizontal;
        facilities[f]=limits;
        building_keys[f]=key;
        actor_classes[f]=field<void*>(actor,0x10);
    }
    // The normal activation path rebuilds the list on its next update.
    if (changed) field<bool>(f,0x298)=false;
}
std::optional<Limits> limits_for_key(uint32_t key) {
    std::scoped_lock lock(mutex);
    for (auto [f,id]:building_keys) if (id==key) return facilities.at(f);
    return {};
}
Probe probe(void* f,Vec relative) {
    Probe result{};
    address<Emit>(0x144a1c6c0)(f,&result.origin);
    Vec target{result.origin.x+relative.x,result.origin.y+relative.y,result.origin.z+relative.z};
    result.vanilla=original_reach(f,&target);
    result.modified=reach(f,&target);
    auto limits=limits_for(f);
    if (limits) result.flight=solve(relative,*limits,remove_centre);
    return result;
}
Vec catch_point(void* receiver) {
    Vec result{}; auto vt=field<void**>(receiver,0);
    reinterpret_cast<Catch>(vt[0x568/8])(receiver,&result);
    return result;
}
std::array<uint64_t,3> flight_stats() { return {repaired_counts[0].load(),repaired_counts[1].load(),repaired_counts[2].load()}; }
void remove_facility(void* f) { std::scoped_lock lock(mutex); facilities.erase(f); building_keys.erase(f); actor_classes.erase(f); }
void reset() { std::scoped_lock lock(mutex); facilities.clear(); building_keys.clear(); actor_classes.clear(); }
void set_save_root(std::filesystem::path root) { std::scoped_lock lock(mutex); save_root=std::move(root); }
void shutdown() { if (active) { /* DLL mods are unloaded only after process shutdown. */ } }
}
