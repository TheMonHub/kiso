#ifndef VK_SHADER_H
#define VK_SHADER_H

#include "vulkan/vulkan.h"

VkResult CreateShaderModule(const unsigned char *code, const size_t code_size, const VkDevice device, VkShaderModule *shader_module, char **error_message);

#endif // VK_SHADER_H
