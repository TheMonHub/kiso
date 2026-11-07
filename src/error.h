// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#ifndef ERROR_H
#define ERROR_H

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#include <winuser.h>
#endif

#define ExitOnError(msg, code) InternalExitOnError(msg, code, __FILE__, __LINE__)

#define PrintSetAndGoto(msg, code, label) \
    _current_error_status = code; \
    _log_message(LOG_FATAL, __FILE__, __LINE__, msg); \
    goto label

#define PrintAndReturn(msg, code) \
    _log_message(LOG_FATAL, __FILE__, __LINE__, msg); \
    return code

[[noreturn]]
void InternalExitOnError(const char *msg, int code, const char *file, int line);

void GlfwErrorCallback(int error, const char *description);

#endif // ERROR_H
