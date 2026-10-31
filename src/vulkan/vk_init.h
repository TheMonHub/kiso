#ifndef VK_INIT_H
#define VK_INIT_H

#include <vulkan/vulkan_core.h>

#include "vk_convert.h"

#define CURRENT_VK_API_VERSION VK_API_VERSION_1_1

typedef struct PhysicalDeviceInformation {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceFeatures features;
    QueueFamilyInfo global_queue_family_info;
    uint64_t score;
    VkQueueFamilyProperties *queue_family_properties;
    uint32_t queue_family_count;
} PhysicalDeviceInformation;

[[nodiscard]]
VkResult CreateVulkanInstance(VkInstance* instance, const char** extensions, uint32_t extensions_count, VkInstanceCreateFlags flags);

[[nodiscard]]
VkResult GetVulkanPhysicalDevicesInfo(VkInstance instance, VkPhysicalDevice *physical_device, uint32_t device_count, PhysicalDeviceInformation **info, char** error_message);

[[nodiscard]]
VkResult CreateVulkanDevice(VkPhysicalDevice physical_device, VkDevice* vk_device, uint32_t queue_family_index, const char * const extensions[], uint32_t extension_count, char** error_message);

[[nodiscard]]
bool GetBestPhysicalDevice(const PhysicalDeviceInformation *properties, uint32_t device_count, uint32_t *best_device, VkQueueFlags required_flags) [[unsequenced]];

[[nodiscard]]
bool GetQueueFamilyIndexWithRequiredFlags(uint32_t queue_family_count, const VkQueueFamilyProperties *queue_family_info, VkQueueFlags required_flags, uint32_t *queue_family_index) [[unsequenced]];

#endif // VK_INIT_H
