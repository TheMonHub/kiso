// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#ifndef NEW_CMD_CALLBACK_H
#define NEW_CMD_CALLBACK_H
#include <stdio.h>

#include "args.h"
#include "verify.h"


#ifdef _WIN32
#include <direct.h>
#define CREATE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define CREATE_DIR(path) mkdir(path, 0755)
#endif

// TODO: Implement dry run and verbose
static int NewCmdCallback(char* cmd_name, ArgParser* cmd_parser) {
    const int args_i = ap_count_args(cmd_parser);
    if (args_i > 1) {
        fprintf(stderr, "error: Too many arguments\n");
        return 1;
    }
    if (args_i == 0) {
        fprintf(stderr, "error: Too few arguments\n");
        return 1;
    }

    char *arg = TrimView(ap_get_arg_at_index(cmd_parser, 0));
    const bool valid = VerifyName(arg);
    if (!valid) {
        return 1;
    }

    struct stat st;

    if (stat(arg, &st) == 0 && S_ISDIR(st.st_mode)) {
        fprintf(stderr, "error: Directory already exists\n");
        return 1;
    }

    printf("New project: %s\n", arg);

    if (CREATE_DIR(arg)) {
        perror("Error creating directory");
        return 1;
    }

    char temp[128];

    snprintf(temp, sizeof(temp), "%s/.kiso", arg);
    if (CREATE_DIR(temp)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/src", arg);
    if (CREATE_DIR(temp)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/shaders", arg);
    if (CREATE_DIR(temp)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/output", arg);
    if (CREATE_DIR(temp)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/assets", arg);
    if (CREATE_DIR(temp)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/.kiso/kiso.d.lua", arg);
    FILE *kiso_lua_def = fopen(temp, "w");
    if (kiso_lua_def == NULL) {
        perror("Error creating file");
        return 1;
    }
    constexpr char kiso_lua_def_dat[] = {
#embed ".kiso/kiso.d.lua"
    };
    fwrite(kiso_lua_def_dat, 1, sizeof(kiso_lua_def_dat), kiso_lua_def);
    if (fclose(kiso_lua_def) == EOF) {
        perror("Error closing file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/.luarc.json", arg);
    FILE *luarc_json = fopen(temp, "w");
    if (luarc_json == NULL) {
        perror("Error creating file");
        return 1;
    }
    constexpr char luarc_json_dat[] = {
#embed ".luarc.json"
    };
    fwrite(luarc_json_dat, 1, sizeof(luarc_json_dat), luarc_json);
    if (fclose(luarc_json) == EOF) {
        perror("Error closing file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/src/main.lua", arg);
    FILE *main_src = fopen(temp, "w");
    if (main_src == NULL) {
        perror("Error creating file");
        return 1;
    }
    constexpr char main_dat[] = {
#embed "example_main.lua"
    };
    fwrite(main_dat, 1, sizeof(main_dat), main_src);
    if (fclose(main_src) == EOF) {
        perror("Error closing file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/.kiso/kiso.schema.json", arg);
    FILE *kiso_schema_json = fopen(temp, "w");
    if (kiso_schema_json == NULL) {
        perror("Error creating file");
        return 1;
    }

    constexpr char kiso_schema_json_dat[] = {
#embed ".kiso/kiso.schema.json"
    };
    fwrite(kiso_schema_json_dat, 1, sizeof(kiso_schema_json_dat), kiso_schema_json);
    if (fclose(kiso_schema_json) == EOF) {
        perror("Error closing file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/kiso.yaml", arg);
    FILE *kiso_yaml = fopen(temp, "w");
    if (kiso_yaml == NULL) {
        perror("Error creating file");
        return 1;
    }
    constexpr char kiso_yaml_dat[] = {
#embed "kiso.yaml"
        , '\0'
    };
    fprintf(kiso_yaml, kiso_yaml_dat, arg);
    if (fclose(kiso_yaml) == EOF) {
        perror("Error closing file");
        return 1;
    }

    return 0;
}

#endif // NEW_CMD_CALLBACK_H
