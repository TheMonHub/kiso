// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include "verify.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define DIR_SEPARATOR "\\"
#else
#define DIR_SEPARATOR "/"
#endif

char *TrimView(char *str) {
    while (isspace((unsigned char) *str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char) *end)) {
        end--;
    }
    *(end + 1) = '\0';
    return str;
}

bool VerifyName(const char *arg) {
    const size_t arg_len = strlen(arg);

    if (arg_len == 0) {
        fprintf(stderr, "error: Invalid argument\n");
        return false;
    }

    if (arg_len > 64) {
        fprintf(stderr, "error: Argument too long\n");
        return false;
    }

    if (strstr(arg, DIR_SEPARATOR)) {
        fprintf(stderr, "error: Invalid argument\n");
        return false;
    }

    if (strspn(arg, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "abcdefghijklmnopqrstuvwxyz"
                    "0123456789"
                    " _-.'()&+") != arg_len) {
        fprintf(stderr, "error: Invalid argument\n");
        return false;
                    }

    // Name with only spaces
    if (strspn(arg, " ") == arg_len) {
        fprintf(stderr, "error: Invalid argument\n");
        return false;
    }

    return true;
}
