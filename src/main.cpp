#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <Mod/CppUserModBase.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include "native.hpp"

namespace {
std::filesystem::path mod_path;
bool remove_exclusion = true;
bool development = false;

void write_log(const char* message) {
    std::ofstream(mod_path / "FountainControl.log", std::ios::app) << message << '\n';
}

int native_info(const RC::LuaMadeSimple::Lua& lua) {
    lua.set_string("FountainControl native bridge 0.1.0");
    lua.set_bool(remove_exclusion);
    lua.set_bool(fc::ready());
    return 3;
}

int native_set(const RC::LuaMadeSimple::Lua& lua) {
    auto pointer = reinterpret_cast<void*>(lua.get_integer());
    double drop = lua.get_number(), horizontal = lua.get_number();
    auto key=static_cast<uint32_t>(lua.get_integer());
    auto actor=reinterpret_cast<void*>(lua.get_integer());
    fc::set_facility(pointer, {std::clamp(drop,0.,1000.),horizontal > 500 ? -1 : horizontal},key,actor);
    return 0;
}
int native_remove(const RC::LuaMadeSimple::Lua& lua) { fc::remove_facility(reinterpret_cast<void*>(lua.get_integer())); return 0; }
int native_reset(const RC::LuaMadeSimple::Lua&) { fc::reset(); return 0; }
int native_generation(const RC::LuaMadeSimple::Lua& lua) { lua.set_integer(fc::load_generation()); return 1; }
int native_get(const RC::LuaMadeSimple::Lua& lua) {
    auto value=fc::limits_for_key(static_cast<uint32_t>(lua.get_integer()));
    lua.set_bool(value.has_value()); lua.set_number(value ? value->drop : 1000); lua.set_number(value ? value->horizontal : -1); return 3;
}
int native_probe(const RC::LuaMadeSimple::Lua& lua) {
    auto f=reinterpret_cast<void*>(lua.get_integer());
    fc::Vec delta{lua.get_number(),lua.get_number(),lua.get_number()};
    if (!development || !fc::ready()) return 0;
    auto p=fc::probe(f,delta);
    lua.set_bool(p.vanilla); lua.set_bool(p.modified);
    lua.set_number(p.origin.x); lua.set_number(p.origin.y); lua.set_number(p.origin.z);
    lua.set_number(p.flight ? p.flight->time : -1); lua.set_number(p.flight ? p.flight->vertical : -1);
    lua.set_bool(p.flight && p.flight->repaired); return 8;
}
int native_catch(const RC::LuaMadeSimple::Lua& lua) {
    auto receiver=reinterpret_cast<void*>(lua.get_integer());
    if (!development || !fc::ready()) return 0;
    auto p=fc::catch_point(receiver); lua.set_number(p.x); lua.set_number(p.y); lua.set_number(p.z); return 3;
}
int native_stats(const RC::LuaMadeSimple::Lua& lua) {
    if (!development) return 0;
    for (auto n:fc::flight_stats()) lua.set_integer(n); return 3;
}
int native_hash(const RC::LuaMadeSimple::Lua& lua) {
    auto input = lua.get_string();
    auto result = fc::fingerprint(std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(input.data()),input.size())));
    lua.set_string(result); return 1;
}
int native_save_root(const RC::LuaMadeSimple::Lua& lua) {
    auto input=lua.get_string();
    fc::set_save_root(std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(input.data()),input.size())));
    return 0;
}

int native_root(const RC::LuaMadeSimple::Lua& lua) {
    auto path = mod_path.u8string();
    lua.set_string(std::string_view(reinterpret_cast<const char*>(path.data()), path.size()));
    lua.set_bool(development);
    return 2;
}

int native_read(const RC::LuaMadeSimple::Lua& lua) {
    auto address = static_cast<uintptr_t>(lua.get_integer(1));
    auto length = static_cast<size_t>(lua.get_integer(1));
    MEMORY_BASIC_INFORMATION mbi{};
    if (!development || length > 16384 || !VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof mbi)
        || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        || address + length > reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize) {
        lua.set_nil(); return 1;
    }
    lua.set_string(reinterpret_cast<const uint8_t*>(address), length);
    return 1;
}

class FountainControl final : public RC::CppUserModBase {
public:
    FountainControl() {
        ModName = L"FountainControl";
        ModVersion = L"0.1.0";
        ModDescription = L"Per-fountain range controls for Alchemy Factory";
        ModAuthors = L"draradech";
        HMODULE module;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&native_info), &module);
        wchar_t path[32768];
        GetModuleFileNameW(module, path, 32768);
        mod_path = std::filesystem::path(path).parent_path().parent_path();
        wchar_t value[32];
        GetPrivateProfileStringW(L"UprightFountains", L"RemoveCenterExclusion", L"true", value, 32,
                                 (mod_path / "config.ini").c_str());
        remove_exclusion = _wcsicmp(value, L"false") != 0 && wcscmp(value, L"0") != 0;
        development = GetPrivateProfileIntW(L"Development", L"EnableCommands", 0, (mod_path / "config.ini").c_str()) != 0;
        write_log("DLL constructed; UE4SS ABI initialized");
    }
    void on_unreal_init() override { write_log("Unreal initialized"); fc::initialize(remove_exclusion); }
    void on_lua_start(RC::StringViewType name, RC::LuaMadeSimple::Lua& lua,
                      RC::LuaMadeSimple::Lua& main_lua, RC::LuaMadeSimple::Lua& async_lua,
                      RC::LuaMadeSimple::Lua* hook_lua) override {
        if (name != L"FountainControl") return;
        auto register_bridge = [](RC::LuaMadeSimple::Lua& state) {
            state.register_function("FCNativeInfo", native_info);
            state.register_function("FCNativeRoot", native_root);
            state.register_function("FCNativeRead", native_read);
            state.register_function("FCSetFacility", native_set);
            state.register_function("FCRemoveFacility", native_remove);
            state.register_function("FCResetFacilities", native_reset);
            state.register_function("FCLoadGeneration", native_generation);
            state.register_function("FCGetNativeLimits", native_get);
            state.register_function("FCProbeReach", native_probe);
            state.register_function("FCProbeCatch", native_catch);
            state.register_function("FCFlightStats", native_stats);
            state.register_function("FCFingerprint", native_hash);
            state.register_function("FCSetSaveRoot", native_save_root);
        };
        register_bridge(lua);
        if (&main_lua != &lua) register_bridge(main_lua);
        if (&async_lua != &lua && &async_lua != &main_lua) register_bridge(async_lua);
        if (hook_lua && hook_lua != &lua && hook_lua != &main_lua && hook_lua != &async_lua) register_bridge(*hook_lua);
        write_log("Lua native bridge registered");
    }
};
}

void fc::log(const std::string& message) { write_log(message.c_str()); }

extern "C" __declspec(dllexport) RC::CppUserModBase* start_mod() { return new FountainControl(); }
extern "C" __declspec(dllexport) void uninstall_mod(RC::CppUserModBase* mod) { delete mod; }
