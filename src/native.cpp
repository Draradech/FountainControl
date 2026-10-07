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
#include "blueprint.hpp"

namespace fc {
namespace {
uintptr_t image;
bool active = false, remove_centre = true;
std::mutex mutex;
std::unordered_map<void*,Limits> facilities;
std::unordered_map<void*,uint32_t> building_keys;
std::unordered_map<void*,void*> actor_classes;
std::unordered_map<void*,void*> interactions;
std::atomic<uintptr_t> pending_interaction{};
std::array<std::atomic<uint64_t>,3> repaired_counts{};
struct CloneSource { Limits limits; void* actor_class; };
// Cut/move can destroy the source before rebuilding it. Keep its settings for
// the current world; save snapshots still include only live facilities.
std::unordered_map<uint32_t,CloneSource> remembered;
thread_local std::optional<CloneSource> cloning;
uint32_t fountain_class_name{};
struct BlueprintIdentity {
    uint32_t key;
    uint64_t name;
    Vec position;
    bool operator==(const BlueprintIdentity& b) const {
        return key==b.key && name==b.name && position.x==b.position.x
            && position.y==b.position.y && position.z==b.position.z;
    }
};
struct BlueprintSetting { BlueprintIdentity identity; Limits limits; };
struct BlueprintSession { std::vector<BlueprintSetting> settings; };
// An empty session is intentional: a vanilla blueprint must not borrow limits
// from an unrelated building with the same original ID in the destination world.
std::unordered_map<void*,BlueprintSession> blueprint_sessions;
thread_local BlueprintSession* entering_blueprint{};
thread_local blueprint::Metadata* packing_blueprint{};
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
Worker original_interaction;
void interact(void* component) {
    {
        std::scoped_lock lock(mutex);
        auto found=interactions.find(component);
        // UActorComponent's owner guards against a recycled component address.
        if (found!=interactions.end() && field<void*>(component,0xa8)==found->second) {
            pending_interaction=reinterpret_cast<uintptr_t>(found->second);
            return;
        }
    }
    original_interaction(component);
}
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
BlueprintIdentity blueprint_identity(void* record) {
    return {field<uint32_t>(record,0x1ac),field<uint64_t>(record,0x1d0),field<Vec>(record,0x1b8)};
}
std::optional<Limits> blueprint_limits(const BlueprintSession& session,void* record) {
    auto identity=blueprint_identity(record);
    for (auto& entry:session.settings) if (entry.identity==identity) return entry.limits;
    return {};
}
bool clone(void* controller,const Vec* position,uint8_t rotation,void* command,bool ignore_cost) {
    auto previous=cloning;
    cloning.reset();
    {
        uint32_t source=field<uint32_t>(command,0x1ac);
        std::scoped_lock lock(mutex);
        auto imported=blueprint_sessions.find(controller);
        if (imported!=blueprint_sessions.end()) {
            if (auto limits=blueprint_limits(imported->second,command)) cloning=CloneSource{*limits,nullptr};
        } else {
            auto found=remembered.find(source);
            if (found!=remembered.end()) cloning=found->second;
        }
    }
    bool result=original_clone(controller,position,rotation,command,ignore_cost);
    cloning=previous;
    return result;
}
void* build(void* subsystem,void* out,void* controller,void* variant,bool a,uint64_t area,
            const Vec* position,bool b,uint8_t active_index,bool c,void* extension) {
    void* result=original_build(subsystem,out,controller,variant,a,area,position,b,active_index,c,extension);
    if (cloning && field<bool>(out,0x14)) {
        void* actor=field<void*>(out,0x20);
        void* actor_class=actor ? field<void*>(actor,0x10) : nullptr;
        bool matches=actor_class && (cloning->actor_class ? actor_class==cloning->actor_class
            : fountain_class_name && field<uint32_t>(actor_class,0x18)==fountain_class_name
                && field<void*>(actor,0x2f0) && field<int>(field<void*>(actor,0x2f0),0xc8)==2);
        if (matches) {
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
using ReadBlueprint = void*(*)(void*,void*,void*);
ReadBlueprint original_read_blueprint;
using EnterBlueprintIndex = bool(*)(void*,void*);
using EnterBlueprintPath = bool(*)(void*,void*,void*);
EnterBlueprintIndex original_enter_blueprint_index;
EnterBlueprintPath original_enter_blueprint_path;
Worker original_clear_clone;
// The FString/FText arguments here are pointers to by-value Unreal ABI copies.
using PackBlueprint = void*(*)(void*,void*,void*,void*,void*,int,uint64_t,void*,bool,void*,void*);
PackBlueprint original_pack_blueprint;
using SaveArray = bool(*)(void*,const wchar_t*,void*,uint32_t);
SaveArray original_save_array;
std::string memory_fingerprint(std::span<const uint8_t> bytes) {
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    if (bytes.size()>ULONG_MAX || BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0) return {};
    std::array<unsigned char,32> digest{};
    bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0
        && BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0
        && BCryptFinishHash(hash,digest.data(),digest.size(),0)>=0;
    if (hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm,0);
    if (!ok) return {};
    std::string result;
    for (auto b:digest) { result+="0123456789abcdef"[b>>4]; result+="0123456789abcdef"[b&15]; }
    return result;
}
void* read_blueprint(void* out,void* directory,void* filename) {
    auto path=std::filesystem::path(unreal_string(directory))/unreal_string(filename);
    void* result=original_read_blueprint(out,directory,filename);
    if ((!entering_blueprint && !packing_blueprint) || !field<bool>(out,0)) return result;
    void* state=static_cast<char*>(out)+0x50;
    int count=field<int>(state,8);
    // Editing a saved blueprint serializes its loaded records, not the current
    // world selection. Preserve its embedded settings before its PNG changes.
    if (packing_blueprint) *packing_blueprint=blueprint::Metadata{count,{}};
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if (!file) return result;
    auto length=file.tellg();
    if (length<0 || length>134217728) return result;
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    file.seekg(0); file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    if (!file) return result;
    auto png=blueprint::extract(bytes);
    if (!png || !png->metadata) return result;
    auto data=blueprint::decode(*png->metadata,memory_fingerprint(png->canonical),count);
    if (!data) { log("Invalid or stale blueprint metadata; using defaults"); return result; }
    if (packing_blueprint) *packing_blueprint=*data;
    if (!entering_blueprint) return result;
    auto records=field<char*>(state,0);
    if (count && !records) return result;
    for (auto [index,limits]:data->fountains)
        entering_blueprint->settings.push_back({blueprint_identity(records+index*0x200),limits});
    log("Loaded matching blueprint PNG metadata");
    return result;
}
template<class Function,class... Args> bool enter_blueprint(Function original,void* state,Args... args) {
    BlueprintSession session;
    auto previous=entering_blueprint;
    entering_blueprint=&session;
    bool result=original(state,args...);
    entering_blueprint=previous;
    std::scoped_lock lock(mutex);
    blueprint_sessions.erase(state);
    if (result) blueprint_sessions.emplace(state,std::move(session));
    return result;
}
bool enter_blueprint_index(void* state,void* index) {
    return enter_blueprint(original_enter_blueprint_index,state,index);
}
bool enter_blueprint_path(void* state,void* directory,void* filename) {
    return enter_blueprint(original_enter_blueprint_path,state,directory,filename);
}
void clear_clone(void* state) {
    { std::scoped_lock lock(mutex); blueprint_sessions.erase(state); }
    original_clear_clone(state);
}
void* pack_blueprint(void* state,void* out,void* name,void* title,void* description,int color,
                     uint64_t item,void* texture,bool force,void* directory,void* folder) {
    blueprint::Metadata data;
    void* selection=static_cast<char*>(state)+0x2d8;
    data.buildings=field<int>(selection,8);
    if (data.buildings>=0 && data.buildings<=100000) {
        std::scoped_lock lock(mutex);
        auto records=field<char*>(selection,0);
        auto imported=blueprint_sessions.find(state);
        for (int index=0;records && index<data.buildings;++index) {
            auto record=records+index*0x200;
            if (imported!=blueprint_sessions.end()) {
                if (auto value=blueprint_limits(imported->second,record)) data.fountains[index]=*value;
            } else {
                auto source=remembered.find(field<uint32_t>(record,0x1ac));
                if (source!=remembered.end()) data.fountains[index]=source->second.limits;
            }
        }
    }
    auto previous=packing_blueprint;
    packing_blueprint=&data;
    void* result=original_pack_blueprint(state,out,name,title,description,color,item,texture,force,directory,folder);
    packing_blueprint=previous;
    return result;
}
bool save_array(void* bytes,const wchar_t* filename,void* manager,uint32_t flags) {
    auto ordinary=[&] { return original_save_array(bytes,filename,manager,flags); };
    if (!packing_blueprint || !filename || std::filesystem::path(filename).extension()!=L".png") return ordinary();
    // Only the final PNG contains the native blueprint footer, not an image export.
    auto length=field<int64_t>(bytes,8);
    auto buffer=field<uint8_t*>(bytes,0);
    if (length<8 || length>134217728 || !buffer || std::memcmp(buffer+length-4,"\xd5\xaf\x42\x50",4)!=0) return ordinary();
    auto png=blueprint::extract({buffer,static_cast<size_t>(length)});
    if (!png) return ordinary();
    auto hash=memory_fingerprint(png->canonical);
    if (hash.empty()) return ordinary();
    auto embedded=blueprint::embed(*png,blueprint::encode(*packing_blueprint,hash));
    if (embedded.empty()) return ordinary();
    // SaveArrayToFile receives a TArrayView64, not an owning Unreal allocation.
    // Its synchronous writer borrows this buffer; the original array is intact.
    struct View { const uint8_t* data; int64_t count; } view{embedded.data(),static_cast<int64_t>(embedded.size())};
    bool result=original_save_array(&view,filename,manager,flags);
    if (result) log("Blueprint PNG settings written");
    return result;
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
        {0x144a24200,reinterpret_cast<void*>(&interact),reinterpret_cast<void**>(&original_interaction)},
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
        {0x144aca1e0,reinterpret_cast<void*>(&read_blueprint),reinterpret_cast<void**>(&original_read_blueprint)},
        {0x144ab6d90,reinterpret_cast<void*>(&enter_blueprint_index),reinterpret_cast<void**>(&original_enter_blueprint_index)},
        {0x144ab7180,reinterpret_cast<void*>(&enter_blueprint_path),reinterpret_cast<void**>(&original_enter_blueprint_path)},
        {0x144ab3ce0,reinterpret_cast<void*>(&clear_clone),reinterpret_cast<void**>(&original_clear_clone)},
        {0x144ac6000,reinterpret_cast<void*>(&pack_blueprint),reinterpret_cast<void**>(&original_pack_blueprint)},
        {0x1412efd40,reinterpret_cast<void*>(&save_array),reinterpret_cast<void**>(&original_save_array)},
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
void set_fountain_class_name(uint32_t name) { fountain_class_name=name; }
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
        remembered[key]={limits,actor_classes[f]};
    }
    // The normal activation path rebuilds the list on its next update.
    if (changed) field<bool>(f,0x298)=false;
}
std::optional<Limits> limits_for_key(uint32_t key) {
    std::scoped_lock lock(mutex);
    auto found=remembered.find(key);
    return found==remembered.end() ? std::optional<Limits>{} : found->second.limits;
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
    if (!receiver) return {};
    Vec result{}; auto vt=field<void**>(receiver,0);
    reinterpret_cast<Catch>(vt[0x568/8])(receiver,&result);
    return result;
}
bool deliver_probe(void* facility,void* receiver,void* subsystem,void* visuals,uint64_t item) {
    if (!active || !receiver || !subsystem || !visuals || !limits_for(facility)) return false;
    void* spatial=field<void*>(subsystem,0xc0);
    if (!spatial || !field<void*>(spatial,0x600)) return false;
    // Production receives FTypeGroupParam, not the GameState actor itself.
    alignas(16) std::array<char,0xd8> parameters{};
    field<void*>(parameters.data(),0x68)=field<void*>(subsystem,0xb8);
    field<void*>(parameters.data(),0x70)=spatial;
    field<void*>(parameters.data(),0x98)=visuals;
    Vec target=catch_point(receiver);
    if (!reach(facility,&target)) return false;
    auto config=address<void*(*)(void*,uint64_t)>(0x1449ec190)(facility,item);
    if (!config || !field<void*>(config,0x28)) return false;
    alignas(16) std::array<char,0x70> product{};
    field<int>(product.data(),8)=1;
    field<uint64_t>(product.data(),0x48)=item;
    field<int>(product.data(),0x50)=1;
    Vec origin{}; address<Emit>(0x144a1c6c0)(facility,&origin);
    void* previous=emitting; emitting=facility;
    // A synthetic one-item input exercises the real hopper worker, visual
    // entity and arrival path. It is not a replacement production launch path.
    receive<2>(receiver,0,parameters.data(),product.data(),&origin,500,field<void*>(config,0x28),2);
    emitting=previous;
    return true;
}
bool clone_probe(void* subsystem,void* state,uint32_t key,Vec position) {
    if (!active || !subsystem || !state || !limits_for_key(key)) return false;
    auto controller_class=address<void*(*)()>(0x1449bcb80)();
    // Clone is a SpatialCloneState member whose typed outer is the controller.
    if (!address<void*(*)(void*,void*)>(0x1415272f0)(state,controller_class)) return false;
    void* info=address<void*(*)(void*,uint32_t)>(0x144a62a60)(subsystem,key);
    if (!info) return false;
    alignas(16) std::array<char,0x218> packet{};
    auto p=packet.data();
    field<uint8_t>(p,0x1a0)=field<uint8_t>(info,0x20);
    field<uint8_t>(p,0x1a1)=field<uint8_t>(info,0x22);
    field<uint8_t>(p,0x1a2)=field<uint8_t>(info,0x23);
    field<uint64_t>(p,0x1a4)=field<uint64_t>(info,0x2c);
    field<uint32_t>(p,0x1ac)=key;
    field<uint64_t>(p,0x1b0)=field<uint64_t>(info,0x48);
    field<Vec>(p,0x1b8)=field<Vec>(info,0x50);
    field<uint64_t>(p,0x1d0)=field<uint64_t>(info,0x78);
    field<uint64_t>(p,0x198)=field<uint64_t>(info,0x48);
    field<uint64_t>(p,0x20c)=field<uint64_t>(info,0x48);
    return clone(state,&position,0,p,false);
}
std::array<uint64_t,3> flight_stats() { return {repaired_counts[0].load(),repaired_counts[1].load(),repaired_counts[2].load()}; }
int blueprint_place_probe(void* state,Vec position,uint8_t rotation,bool ignore_cost) {
    if (!active || !state || rotation>3) return -1;
    auto controller_class=address<void*(*)()>(0x1449bcb80)();
    if (!address<void*(*)(void*,void*)>(0x1415272f0)(state,controller_class)) return -1;
    void* selection=static_cast<char*>(state)+0x2d8;
    int count=field<int>(selection,8);
    if (count<0 || count>100000) return -1;
    auto records=field<char*>(selection,0);
    int placed=0;
    for (int index=0;records && index<count;++index) {
        alignas(16) std::array<char,0x218> packet{};
        std::memcpy(packet.data(),records+index*0x200,0x200);
        std::memcpy(packet.data()+0x200,static_cast<char*>(selection)+0x10,0x14);
        if (clone(state,&position,rotation,packet.data(),ignore_cost)) ++placed;
    }
    return placed;
}
void remove_facility(void* f) { std::scoped_lock lock(mutex); facilities.erase(f); building_keys.erase(f); actor_classes.erase(f); }
void reset() { std::scoped_lock lock(mutex); facilities.clear(); building_keys.clear(); actor_classes.clear(); remembered.clear(); interactions.clear(); blueprint_sessions.clear(); pending_interaction=0; }
void set_interaction(void* component,void* actor) {
    if (!active) return;
    std::scoped_lock lock(mutex);
    if (actor) interactions[component]=actor; else interactions.erase(component);
}
uintptr_t take_interaction() { return pending_interaction.exchange(0); }
void set_save_root(std::filesystem::path root) { std::scoped_lock lock(mutex); save_root=std::move(root); }
void shutdown() { if (active) { /* DLL mods are unloaded only after process shutdown. */ } }
}
