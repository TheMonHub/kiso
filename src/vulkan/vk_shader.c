#include "vk_shader.h"

#include "vulkan/vulkan.h"

#define RETURN_ERROR(msg, code) \
    *error_message = msg; \
    return code

VkResult CreateShaderModule(const unsigned char *code, const size_t code_size, const VkDevice device, VkShaderModule *shader_module, char **error_message) {
    const VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code_size,
        .pCode = (const uint32_t *)code
    };

    const VkResult result = vkCreateShaderModule(device, &create_info, nullptr, shader_module);
    if (result != VK_SUCCESS) {
        RETURN_ERROR("Failed to create shader module", result);
    }
    return VK_SUCCESS;
}