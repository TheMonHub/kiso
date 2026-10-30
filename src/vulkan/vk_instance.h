#ifndef VK_INSTANCE_H
#define VK_INSTANCE_H

#include <vulkan/vulkan_core.h>

[[nodiscard]]
VkResult CreateVulkanInstance(VkInstance* instance, const char** extensions, const uint32_t extensions_count);

#endif // VK_INSTANCE_H
