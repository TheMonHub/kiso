// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include "error.h"

#include <stdlib.h>

#include "logger.h"

#if defined(_WIN32)
#include <windows.h>
#include <winuser.h>
#endif

void InternalExitOnError(const char *msg, const int code, const char *file, const int line) {
    _log_message(LOG_FATAL, file, line, msg);

#if defined(_WIN32)
    MessageBox(NULL, msg, "kiso", MB_ICONERROR);
#endif
    exit(code);
}

void GlfwErrorCallback(const int error, const char *description) {
    log_error("%s (%d)", description, error);
}
