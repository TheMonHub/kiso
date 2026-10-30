#include "vk_instance.h"

#include "GLFW/glfw3.h"
#include "macro/array_size.h"
#include "version.h"

#include <vulkan/vulkan.h>

VkResult CreateVulkanInstance(VkInstance* instance, const char** extensions, const uint32_t extensions_count) {
    static VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "kiso",
        .applicationVersion = kProjectVersionNumber,
        .pEngineName = nullptr,
        .engineVersion = kProjectVersionNumber,
        .apiVersion = VK_API_VERSION_1_1
      };

    static VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr
      };

    instance_create_info.enabledExtensionCount = extensions_count;
    instance_create_info.ppEnabledExtensionNames = extensions;

    return vkCreateInstance(&instance_create_info, nullptr, instance);
}