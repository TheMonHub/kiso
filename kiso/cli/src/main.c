// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <string.h>

#include "kiso/app.h"

// Make our own argument parser or use a library
int main(const int argc, char *argv[]) {
    if (argc <= 1) {
        printf("Usage: %s <command>\n", argv[0]);
        return 0;
    }
    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            printf("Usage: %s run <app_folder>\n", argv[0]);
            return 0;
        }

        lua_State *state = nullptr;
        const int status = RunAppFolder(argv[2], &state, argc - 3, argv + 3);
        if (status != 0) {
            lua_close(state);
            return status;
        }

        lua_close(state);
        return GetCurrentExitCode();
    }
}
