#include "error.h"

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