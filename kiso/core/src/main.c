// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0
#include "lua.h"

int main(const int argc, char *argv[]) {
    lua_State *state = nullptr;
    const int status = RunAppFolder("../../example", &state, argc - 1, argv + 1);
    if (status != 0) {
        lua_close(state);
        return status;
    }

    lua_close(state);
    return GetCurrentExitCode();
}
