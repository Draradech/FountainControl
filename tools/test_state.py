#!/usr/bin/env python3
"""Run Lua persistence tests using the host Lua 5.4 library (no game needed)."""
import ctypes
import ctypes.util
import tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
lua=ctypes.CDLL(ctypes.util.find_library('lua5.4'))
lua.luaL_newstate.restype=ctypes.c_void_p
lua.luaL_openlibs.argtypes=[ctypes.c_void_p]
lua.luaL_loadstring.argtypes=[ctypes.c_void_p,ctypes.c_char_p]
lua.lua_pcallk.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_ssize_t,ctypes.c_void_p]
lua.lua_tolstring.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_void_p];lua.lua_tolstring.restype=ctypes.c_char_p
lua.lua_close.argtypes=[ctypes.c_void_p]
state=lua.luaL_newstate();lua.luaL_openlibs(state)
try:
    with tempfile.TemporaryDirectory() as temporary:
        # Lua long strings avoid shell/string interpolation of paths.
        code=f"arg={{[1]=[==[{root}]==],[2]=[==[{temporary}]==]}}; dofile(arg[1]..'/tests/state.lua')"
        status=lua.luaL_loadstring(state,code.encode()) or lua.lua_pcallk(state,0,0,0,0,None)
        if status: raise RuntimeError(lua.lua_tolstring(state,-1,None).decode())
finally:
    lua.lua_close(state)
