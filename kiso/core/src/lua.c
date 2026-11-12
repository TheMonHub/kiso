// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include "lua.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <threads.h>

#include "logger.h"

#include "lauxlib.h"
#include "lualib.h"
#include "kiso/time.h"

#include "kiso/version.h"

static int g_argc = 0;
static char **g_argv = nullptr;

static bool g_should_exit = false;
static int g_exit_code = 0;

static lua_Integer g_limit_callbacks_rate = 0;

static sig_atomic_t g_signal_to_handle = 0;

static void HandleSigIntTerm(int signum) {
    g_signal_to_handle = signum;
}

static int LuaKisoGetVersion(lua_State *state) {
    lua_newtable(state);

    lua_pushinteger(state, kProjectVersionMajor);
    lua_setfield(state, -2, "major");

    lua_pushinteger(state, kProjectVersionMinor);
    lua_setfield(state, -2, "minor");

    lua_pushinteger(state, kProjectVersionPatch);
    lua_setfield(state, -2, "patch");

    return 1;
}

static int LuaKisoGetArgs(lua_State *state) {
    lua_newtable(state);

    for (int i = 0; i < g_argc; i++) {
        lua_pushstring(state, g_argv[i]);
        lua_rawseti(state, -2, i + 1);
    }

    return 1;
}

static int LuaKisoSleep(lua_State *state) {
    const double seconds = luaL_checknumber(state, 1);
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1e9);

    // Genuinely no idea how to handle this if it errors.
    (void)thrd_sleep(&ts, NULL);
    return 0;
}

static int LuaKisoGetMonotonicTime(lua_State *state) {
    lua_pushnumber(state, GetMonotonicTime());
    return 1;
}

static int LuaKisoSetLimitCallbacksRate(lua_State *state) {
    const size_t limit = luaL_checkinteger(state, 1);
    g_limit_callbacks_rate = (lua_Integer)limit;
    return 0;
}

static int LuaKisoGetLimitCallbacksRate(lua_State *state) {
    lua_pushinteger(state, g_limit_callbacks_rate);
    return 1;
}

static int LuaKisoExit(lua_State *state) {
    const long exit_code = luaL_checkinteger(state, 1);

    if (exit_code < INT_MIN || exit_code > INT_MAX) {
        return luaL_error(state, "Exit code must not be outside the range of %d to %d",
                          INT_MIN, INT_MAX);
    }

    const int base_top = lua_gettop(state);

    lua_getglobal(state, "kiso");
    if (!lua_istable(state, -1)) {
        lua_settop(state, base_top);
        return 0;
    }
    lua_getfield(state, -1, "callbacks");
    if (!lua_istable(state, -1)) {
        lua_settop(state, base_top);
        return 0;
    }

    lua_getfield(state, -1, "onExit");
    if (!lua_istable(state, -1)) {
        lua_settop(state, base_top);
        return 0;
    }

    const size_t count = lua_objlen(state, -1);

    for (size_t i = count; i > 0; i--) {
        const int loop_top = lua_gettop(state);
        // NOLINTNEXTLINE(bugprone-narrowing-conversions)
        lua_rawgeti(state, -1, (lua_Integer)i);

        if (lua_isfunction(state, -1)) {
            lua_pushinteger(state, exit_code);

            if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
                HandleLuaError(state);
                lua_settop(state, base_top);
                return 0;
            }
        } else {
            lua_settop(state, loop_top);
        }
    }
    lua_settop(state, base_top);
    g_exit_code = (int)exit_code;
    g_should_exit = true;
    return 0;
}

static int LuaKisoAbort(lua_State *state) {
    const long exit_code = luaL_checkinteger(state, 1);
    if (!(exit_code >= INT_MIN && exit_code <= INT_MAX)) {
        luaL_error(state, "Exit code must not be outside the range of %d to %d", INT_MIN, INT_MAX);
    }
    g_exit_code = (int)exit_code;
    g_should_exit = true;
    return 0;
}

static int LuaKisoKill(lua_State *state) {
    const long exit_code = luaL_checkinteger(state, 1);
    if (!(exit_code >= INT_MIN && exit_code <= INT_MAX)) {
        luaL_error(state, "Exit code must not be outside the range of %d to %d", INT_MIN, INT_MAX);
    }
    _Exit((int)exit_code);
    return 0;
}

void HandleLuaError(lua_State *state) {
    const char *error_message = lua_tostring(state, -1);

    lua_getglobal(state, "kiso");

    if (!lua_istable(state, -1)) {
        lua_pop(state, 1);
        LuaKisoAbort(state);
        return;
    }

    lua_getfield(state, -1, "callbacks");

    if (!lua_istable(state, -1)) {
        lua_pop(state, 2);
        LuaKisoAbort(state);
        return;
    }

    lua_getfield(state, -1, "onError");

    if (!lua_isfunction(state, -1)) {
        lua_pop(state, 3);
        LuaKisoAbort(state);
        return;
    }

    lua_pushstring(state, error_message);

    if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
        lua_pop(state, 1);
        lua_pushinteger(state, -1);
        LuaKisoAbort(state);
        return;
    }

    lua_pop(state, 3);
}

void handlePossibleExitRequest(lua_State *state) {
    if (!g_signal_to_handle) {
        return;
    }

    lua_getglobal(state, "kiso");
    if (!lua_istable(state, -1)) {
        lua_pop(state, 1);
        LuaKisoAbort(state);
        return;
    }

    lua_getfield(state, -1, "callbacks");

    if (!lua_istable(state, -1)) {
        lua_pop(state, 2);
        LuaKisoAbort(state);
        return;
    }

    lua_getfield(state, -1, "onExitRequest");

    if (!lua_isfunction(state, -1)) {
        lua_pop(state, 3);
        LuaKisoAbort(state);
        return;
    }

    lua_pushinteger(state, g_signal_to_handle);

    if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
        lua_pop(state, 1);
        lua_pushinteger(state, -1);
        LuaKisoAbort(state);
        return;
    }

    lua_pop(state, 3);
    g_signal_to_handle = 0;
}

void runUpdate(lua_State* state, double deltaTime) {
    if (g_should_exit) {
        return;
    }
    const int base_top = lua_gettop(state);

    lua_getglobal(state, "kiso");
    if (!lua_istable(state, -1)) {
        lua_settop(state, base_top);
        return;
    }
    lua_getfield(state, -1, "callbacks");
    if (!lua_istable(state, -1)) {
        lua_settop(state, base_top);
        return;
    }

    lua_getfield(state, -1, "update");
    if (!lua_istable(state, -1)) {
        lua_settop(state, base_top);
        return;
    }

    int i = 1;

    while (true) {
        lua_rawgeti(state, -1, i);
        if (lua_isnil(state, -1)) {
            lua_pop(state, 1);
            break;
        }
        if (lua_isfunction(state, -1)) {
            lua_pushnumber(state, deltaTime);
            if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
                HandleLuaError(state);
            }
        } else {
            lua_pop(state, 1);
        }
        i++;
    }
    lua_settop(state, base_top);
}

bool ShouldExit() {
    return g_should_exit;
}

int GetCurrentExitCode() {
    return g_exit_code;
}

double GetLimitCallbacksRate() {
    return (double)g_limit_callbacks_rate;
}

int RunAppFolder(const char *path, lua_State **out, const int argc, char *argv[]) {
    g_argc = argc;
    g_argv = argv;

    // "/src/main.lua" = 13 bytes
    const size_t path_allocate_size = strlen(path) + 13 + 1;
    char *file_to_run = malloc(path_allocate_size);
    if (file_to_run == nullptr) {
        log_error("Failed to allocate memory for file path");
        return 1;
    }
    strncpy(file_to_run, path, path_allocate_size);
    strncat(file_to_run, "/src/main.lua", path_allocate_size);

    lua_State *state = nullptr;
    state = luaL_newstate();
    *out = state;

    if (!state) {
        log_error("Failed to create Lua state");
        free(file_to_run);
        return 1;
    }
    luaL_openlibs(state);

    lua_newtable(state);

    lua_pushcfunction(state, LuaKisoGetVersion);
    lua_setfield(state, -2, "getVersion");

    lua_pushcfunction(state, LuaKisoGetArgs);
    lua_setfield(state, -2, "getArgs");

    lua_pushcfunction(state, LuaKisoSleep);
    lua_setfield(state, -2, "sleep");

    lua_pushcfunction(state, LuaKisoGetMonotonicTime);
    lua_setfield(state, -2, "getMonotonicTime");

    lua_pushcfunction(state, LuaKisoSetLimitCallbacksRate);
    lua_setfield(state, -2, "setLimitCallbacksRate");

    lua_pushcfunction(state, LuaKisoGetLimitCallbacksRate);
    lua_setfield(state, -2, "getLimitCallbacksRate");

    lua_pushcfunction(state, LuaKisoExit);
    lua_setfield(state, -2, "exit");

    lua_pushcfunction(state, LuaKisoAbort);
    lua_setfield(state, -2, "abort");

    lua_pushcfunction(state, LuaKisoKill);
    lua_setfield(state, -2, "kill");

    lua_setglobal(state, "kiso");

    static char bootLua[] = {
#embed "boot.luabc"
    };

    if (luaL_loadbufferx(state, bootLua, sizeof(bootLua), "=kiso_boot", "b")) {
        log_fatal("Failed to load built in boot script");
        free(file_to_run);
        return -1;
    }

    if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
        log_fatal("Failed to execute built in boot script: %s", lua_tostring(state, -1));

        free(file_to_run);
        return -1;
    }

    if (signal(SIGINT, HandleSigIntTerm) == SIG_ERR) {
        free(file_to_run);
        return -2;
    }
    if (signal(SIGTERM, HandleSigIntTerm) == SIG_ERR) {
        free(file_to_run);
        return -2;
    }

    if (luaL_dofile(state, file_to_run) != LUA_OK) {
        HandleLuaError(state);
        free(file_to_run);
        return GetCurrentExitCode();
    }

    free(file_to_run);

    double previous_time = GetMonotonicTime();

    while (!ShouldExit()) {
        handlePossibleExitRequest(state);

        const double current_time = GetMonotonicTime();
        const double deltaTime = current_time - previous_time;
        const double limit_update_rate = GetLimitCallbacksRate();

        if (limit_update_rate == 0 || deltaTime >= 1.0 / limit_update_rate) {
            previous_time = current_time;
            runUpdate(state, deltaTime);
            continue;
        }

        const double remaining_time = 1.0 / limit_update_rate - deltaTime;

        struct timespec ts = {
            .tv_sec = 0,
            .tv_nsec = (long)(remaining_time * 1'000'000'000)
        };

        (void)thrd_sleep(&ts, NULL);
    }

    return 0;
}