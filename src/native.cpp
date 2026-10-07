#include <Windows.h>
#include <bcrypt.h>
#include <MinHook.h>
#include <array>
#include <cstring>
#include <fstream>
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
        if (flight && flight->repaired) { speed=flight->time; pattern=custom_flight; }
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
void set_facility(void* f,Limits limits) {
    if (!active) return;
    bool changed;
    {
        std::scoped_lock lock(mutex);
        auto old=facilities.find(f);
        changed=old==facilities.end() || old->second.drop!=limits.drop || old->second.horizontal!=limits.horizontal;
        facilities[f]=limits;
    }
    // The normal activation path rebuilds the list on its next update.
    if (changed) field<bool>(f,0x298)=false;
}
void remove_facility(void* f) { std::scoped_lock lock(mutex); facilities.erase(f); }
void reset() { std::scoped_lock lock(mutex); facilities.clear(); }
void shutdown() { if (active) { /* DLL mods are unloaded only after process shutdown. */ } }
}
