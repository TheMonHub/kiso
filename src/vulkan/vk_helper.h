#ifndef VK_HELPER_H
#define VK_HELPER_H

#define RETURN_ERROR(msg, code) \
    *error_message = msg; \
    return code

#endif // VK_HELPER_H
