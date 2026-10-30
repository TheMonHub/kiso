#ifndef ERROR_H
#define ERROR_H

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <winuser.h>
#endif

#define ExitOnError(msg, code) InternalExitOnError(msg, code, __FILE__, __LINE__)

[[noreturn]]
void InternalExitOnError(const char *msg, int code, const char *file, int line);

void GlfwErrorCallback(int error, const char *description);

#endif // ERROR_H
