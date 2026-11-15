// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#ifndef RUN_CMD_CALLBACK_H
#define RUN_CMD_CALLBACK_H

#include <lua.h>
#include <stdio.h>

#include "args.h"
#include "kiso/app.h"

// TODO: Implement .kiso file loading
// TODO: Implement reading yaml and load the entry point
static int RunCmdCallback(char* cmd_name, ArgParser* cmd_parser) {
    const int args_i = ap_count_args(cmd_parser);
    char **args = ap_get_args(cmd_parser);
    if (args_i == 0) {
        fprintf(stderr, "error: Required path to app folder or .kiso file\n");
        return 1;
    }
    const char *path = args[0];

    lua_State *state = nullptr;
    const int status = RunAppFolder(path, &state, args_i - 1, args + 1);
    if (status != 0) {
        lua_close(state);
        return status;
    }

    lua_close(state);
    return GetCurrentExitCode();
}


#endif // RUN_CMD_CALLBACK_H
