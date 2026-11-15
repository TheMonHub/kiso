// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <string.h>

#include "args.h"
#include "kiso/app.h"

static int new_cmd_callback(char* cmd_name, ArgParser* cmd_parser) {
    return 1;
}

int main(const int argc, char *argv[]) {
    ArgParser* parser = ap_new_parser();
    if (!parser) {
        return 1;
    }

    ap_set_helptext(parser, "usage: kiso [-v | --version] [-h | --help] <command> [<args>]\n"
                            "\n"
                            "Available commands:\n"
                            "   new        Create a new application\n"
                            "   run        Run an application from a folder or .kiso file\n"
                            "   pack       Pack an application into a .kiso file\n"
                            "   build      Build an application from a folder or .kiso file\n"
                            "See 'kiso help <command>' to read about a specific subcommand."
    );
    ap_set_version(parser, "kiso version " PROJECT_VERSION_STRING);

    ArgParser *new_cmd_parser = ap_new_cmd(parser, "new");
    if (!new_cmd_parser) {
        return 1;
    }

    ap_set_helptext(new_cmd_parser, "usage: kiso new <name>\n"
                                    "       kiso new <path>\n"
                                    "\n"
                                    "Description:\n"
                                    "Create a new application\n"
                                    "\n"
                                    "If <name> is provided, a new directory will be created with the given name in the current directory.\n"
                                    "If <path> is provided, a new application will be initialized at the given path.\n"
                                    "\n"
                                    "Arguments:\n"
                                    "   <name>         Name of the application\n"
                                    "   <path>         Path to the application folder\n"
                                    "\n"
                                    "Options:"
                                    "   -n, --dry-run  Do not create any files and directories\n"
                                    "   -v, --verbose  Print verbose output\n");
    ap_set_cmd_callback(new_cmd_parser, new_cmd_callback);

    if (!ap_parse(parser, argc, argv)) {
        return 1;
    }

    const int exit_code = ap_get_cmd_exit_code(parser);

    if (!ap_found_cmd(parser)) {
        printf("%s\n", ap_get_helptext(parser));
    }
    ap_free(parser);

    return exit_code;

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
