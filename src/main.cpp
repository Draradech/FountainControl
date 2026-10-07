#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <Mod/CppUserModBase.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>

namespace {
std::filesystem::path mod_path;
bool remove_exclusion = true;

void log(const char* message) {
    std::ofstream(mod_path / "FountainControl.log", std::ios::app) << message << '\n';
}

int native_info(const RC::LuaMadeSimple::Lua& lua) {
    lua.set_string("FountainControl native bridge 0.1.0");
    lua.set_bool(remove_exclusion);
    return 2;
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
        log("DLL constructed; UE4SS ABI initialized");
    }
    void on_unreal_init() override { log("Unreal initialized"); }
    void on_lua_start(RC::StringViewType name, RC::LuaMadeSimple::Lua& lua,
                      RC::LuaMadeSimple::Lua& main_lua, RC::LuaMadeSimple::Lua& async_lua,
                      RC::LuaMadeSimple::Lua* hook_lua) override {
        if (name != L"FountainControl") return;
        auto register_bridge = [](RC::LuaMadeSimple::Lua& state) {
            state.register_function("FCNativeInfo", native_info);
        };
        register_bridge(lua);
        if (&main_lua != &lua) register_bridge(main_lua);
        if (&async_lua != &lua && &async_lua != &main_lua) register_bridge(async_lua);
        if (hook_lua && hook_lua != &lua && hook_lua != &main_lua && hook_lua != &async_lua) register_bridge(*hook_lua);
        log("Lua native bridge registered");
    }
};
}

extern "C" __declspec(dllexport) RC::CppUserModBase* start_mod() { return new FountainControl(); }
extern "C" __declspec(dllexport) void uninstall_mod(RC::CppUserModBase* mod) { delete mod; }
