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

static bool CreateDirectory(const char *path, bool dry_run, bool verbose) {
    if (dry_run) {
        printf("[DRY RUN] Create directory: %s\n", path);
        return true;
    }

    if (CREATE_DIR(path)) {
        perror(path);
        return false;
    }

    if (verbose) {
        printf("[VERBOSE] Created directory: %s\n", path);
    }

    return true;
}

static bool CreateFile(const char *path, const char *data, size_t size, bool dry_run, bool verbose) {
    if (dry_run) {
        printf("[DRY RUN] Create file: %s\n", path);
        return true;
    }

    FILE *file = fopen(path, "wb");

    if (file == NULL) {
        perror(path);
        return false;
    }

    bool success = true;

    if (data != nullptr && size != 0) {
        success = fwrite(data, 1, size, file) == size;
    }

    if (fclose(file) == EOF) {
        success = false;
    }

    if (!success) {
        perror(path);
        return false;
    }

    if (verbose) {
        printf("[VERBOSE] Created file: %s\n", path);
    }

    return true;
}

static bool CreateFileFprintf(const char *path, const char *format, const char* string, bool dry_run, bool verbose) {
    if (dry_run) {
        printf("[DRY RUN] Create file: %s\n", path);
        return true;
    }

    FILE *file = fopen(path, "wb");

    if (file == NULL) {
        perror(path);
        return false;
    }

    bool success = fprintf(file, format, string) == 0;

    if (fclose(file) == EOF) {
        success = false;
    }

    if (!success) {
        perror(path);
        return false;
    }

    if (verbose) {
        printf("[VERBOSE] Created file: %s\n", path);
    }

    return true;
}

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
    const bool is_dry_run = ap_found(cmd_parser, "dry-run");
    const bool is_verbose = ap_found(cmd_parser, "verbose");

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

    if (!CreateDirectory(arg, is_dry_run, is_verbose)) {
        perror("Error creating directory");
        return 1;
    }

    char temp[128];

    snprintf(temp, sizeof(temp), "%s/.kiso", arg);
    if (!CreateDirectory(temp, is_dry_run, is_verbose)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/src", arg);
    if (!CreateDirectory(temp, is_dry_run, is_verbose)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/shaders", arg);
    if (!CreateDirectory(temp, is_dry_run, is_verbose)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/output", arg);
    if (!CreateDirectory(temp, is_dry_run, is_verbose)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/assets", arg);
    if (!CreateDirectory(temp, is_dry_run, is_verbose)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/hooks", arg);
    if (!CreateDirectory(temp, is_dry_run, is_verbose)) {
        perror("Error creating directory");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/.kiso/kiso.d.lua", arg);
    constexpr char kiso_lua_def_dat[] = {
#embed ".kiso/kiso.d.lua"
    };
    if (!CreateFile(temp, kiso_lua_def_dat, sizeof(kiso_lua_def_dat), is_dry_run, is_verbose)) {
        perror("Error creating file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/.luarc.json", arg);
    constexpr char luarc_json_dat[] = {
#embed ".luarc.json"
    };
    if (!CreateFile(temp, luarc_json_dat, sizeof(luarc_json_dat), is_dry_run, is_verbose)) {
        perror("Error creating file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/src/main.lua", arg);
    constexpr char main_dat[] = {
#embed "example_main.lua"
    };
    if (!CreateFile(temp, main_dat, sizeof(main_dat), is_dry_run, is_verbose)) {
        perror("Error creating file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/.kiso/kiso.schema.json", arg);
    constexpr char kiso_schema_json_dat[] = {
#embed ".kiso/kiso.schema.json"
    };
    if (!CreateFile(temp, kiso_schema_json_dat, sizeof(kiso_schema_json_dat), is_dry_run, is_verbose)) {
        perror("Error creating file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/kiso.yaml", arg);
    constexpr char kiso_yaml_dat[] = {
#embed "kiso.yaml"
        , '\0'
    };
    if (!CreateFileFprintf(temp, kiso_yaml_dat, arg, is_dry_run, is_verbose)) {
        perror("Error creating file");
        return 1;
    }

    snprintf(temp, sizeof(temp), "%s/hooks/prepack.lua", arg);
    if (!CreateFile(temp, nullptr, 0, is_dry_run, is_verbose)) {
        perror("Error creating file");
        return 1;
    }

    return 0;
}

#endif // NEW_CMD_CALLBACK_H
