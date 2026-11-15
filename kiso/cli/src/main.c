// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>

#include "args.h"
#include "kiso/version.h"
#include "new_cmd_callback.h"
#include "run_cmd_callback.h"


static int VersionCmdCallback(char* cmd_name, ArgParser* cmd_parser) {
    printf("kiso version " PROJECT_VERSION_STRING "\n");

    if (!ap_found(cmd_parser, "build")) {
        return 0;
    }

    printf("version number: %u\n", kProjectVersionNumber);
    printf("commit: " PROJECT_GIT_COMMIT_HASH "\n");
    printf("branch: " PROJECT_GIT_BRANCH "\n");
    return 0;
}

// TODO: Implement the rest of the commands. new, run, pack, build.
int main(const int argc, char *argv[]) {
    fprintf(stderr, "KISO CLI is still unfinished, please do not use this in any production environment.\n");
    ArgParser* parser = ap_new_parser();
    if (!parser) {
        return 1;
    }

    ap_set_helptext(parser, "usage: kiso [-v | --version] [-h | --help] <command> [<args>]\n"
                            "\n"
                            "Available commands:\n"
                            "   new        Create a new project\n"
                            "   run        Run a project from a folder or .kiso file\n"
                            "   pack       Pack a project into a .kiso file\n"
                            "   build      Build a project from a folder or .kiso file\n"
                            "   version    Show the current version of KISO\n"
                            "See 'kiso help <command>' to read about a specific subcommand."
    );
    ap_set_version(parser, "kiso version " PROJECT_VERSION_STRING);

    ArgParser *version_cmd_parser = ap_new_cmd(parser, "version");
    if (!version_cmd_parser) {
        return 1;
    }

    ap_set_helptext(version_cmd_parser, "usage: kiso version\n"
                                        "\n"
                                        "Description:\n"
                                        "   Show the current version of KISO\n"
                                        "\n"
                                        "Options:\n"
                                        "   -b, --build  Show build information");
    ap_add_flag(version_cmd_parser, "b build");
    ap_set_cmd_callback(version_cmd_parser, VersionCmdCallback);

    ArgParser *new_cmd_parser = ap_new_cmd(parser, "new");
    if (!new_cmd_parser) {
        return 1;
    }

    ap_set_helptext(new_cmd_parser, "usage: kiso new <name>\n"
                                    "\n"
                                    "Description:\n"
                                    "   Create a new project."
                                    "   A new directory will be created with the given name in the current directory.\n"
                                    "\n"
                                    "Arguments:\n"
                                    "   name           Name of the project\n"
                                    "\n"
                                    "Options:\n"
                                    "   -n, --dry-run  Do not create any files and directories\n"
                                    "   -v, --verbose  Print verbose output");
    ap_add_flag(new_cmd_parser, "n dry-run");
    ap_add_flag(new_cmd_parser, "v verbose");
    ap_set_cmd_callback(new_cmd_parser, NewCmdCallback);

    ArgParser *run_cmd_parser = ap_new_cmd(parser, "run");
    if (!run_cmd_parser) {
        return 1;
    }

    ap_set_helptext(run_cmd_parser, "usage: kiso run <path>\n"
                                    "\n"
                                    "Description:\n"
                                    "   Run a KISO project from the given path."
                                    "   Whether it's .kiso file or a project folder.\n"
                                    "\n"
                                    "Arguments:\n"
                                    "   path           Path to the project or .kiso file\n"
                                    "\n"
                                    "Options:\n"
                                    "   none           Every option and flags after path are passed through the program");
    ap_set_cmd_callback(run_cmd_parser, RunCmdCallback);

    if (!ap_parse(parser, argc, argv)) {
        return 1;
    }

    const int exit_code = ap_get_cmd_exit_code(parser);

    if (!ap_found_cmd(parser)) {
        printf("%s\n", ap_get_helptext(parser));
    }
    ap_free(parser);

    return exit_code;
}
