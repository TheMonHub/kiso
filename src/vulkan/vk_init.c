#include "vk_init.h"

#include <limits.h>
#include <string.h>

#include "GLFW/glfw3.h"
#include "util/common_macro.h"
#include "version.h"

#include <vulkan/vulkan.h>

#include "error.h"
#include "logger.h"
#include "vk_convert.h"
#include "util/enum_to_str.h"

#define RETURN_ERROR(msg, code) \
    *error_message = msg; \
    return code

static constexpr uint64_t kMaxDeviceSizeScore = 1280;

VkResult CreateVulkanInstance(VkInstance* instance, const char** extensions, const uint32_t extensions_count, VkInstanceCreateFlags flags) {
    log_info(LINE_SEPARATOR);
    log_info(
        "Vulkan API version: %d.%d.%d.%d",
        VK_API_VERSION_VARIANT(CURRENT_VK_API_VERSION),
        VK_API_VERSION_MAJOR(CURRENT_VK_API_VERSION),
        VK_API_VERSION_MINOR(CURRENT_VK_API_VERSION),
        VK_API_VERSION_PATCH(CURRENT_VK_API_VERSION));
    log_info(LINE_SEPARATOR);

    log_trace("Instructed to create a Vulkan instance with %d extensions:", extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++) {
        log_trace("Vulkan instance extension: %s", extensions[i]);
    }
    log_trace("Vulkan instance flags: %08X", flags);

    static const VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "kiso",
        .applicationVersion = kProjectVersionNumber,
        .pEngineName = nullptr,
        .engineVersion = kProjectVersionNumber,
        .apiVersion = CURRENT_VK_API_VERSION
      };

    const VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = extensions_count,
        .ppEnabledExtensionNames = extensions
      };

    return vkCreateInstance(&instance_create_info, nullptr, instance);
}

static int GetScoreBasedOnHeapFlags(const VkFlags flags) {
    // NOLINTBEGIN(bugprone-signed-bitwise)
    if (flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
        return 10;
    }
    if (flags & VK_MEMORY_HEAP_TILE_MEMORY_BIT_QCOM) {
        return 9;
    }
    if (flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) {
        return 8;
    }
    return 0;
    // NOLINTEND(bugprone-signed-bitwise)
}

// The caller of this function is responsible for freeing the memory allocated for the properties array.
// Being the PhysicalDeviceInformation array, queue_family_properties array inside each PhysicalDeviceInformation entry all from the returned array.
VkResult GetVulkanPhysicalDevicesInfo(const VkInstance instance, const VkPhysicalDevice *physical_device, const uint32_t device_count,
    PhysicalDeviceInformation **info, char** error_message) {
    log_trace("Instructed to get Vulkan physical devices information");

    *info = calloc(device_count, sizeof(PhysicalDeviceInformation));
    if (*info == nullptr) {
        RETURN_ERROR("Failed to allocate physical device properties", VK_ERROR_OUT_OF_HOST_MEMORY);
    }

    for (uint32_t i = 0; i < device_count; i++) {
        PhysicalDeviceInformation *device_information = &(*info)[i];

        vkGetPhysicalDeviceProperties(physical_device[i], &device_information->properties);
        VkPhysicalDeviceProperties *device_properties = &device_information->properties;

        log_trace(LINE_SEPARATOR);
        log_trace("Physical device %d", i, device_information->queue_family_count);
        log_trace(LINE_SEPARATOR);
        log_trace("Physical device %d name: %s", i, device_properties->deviceName);
        log_trace("Physical device %d type: %s", i, VulkanDeviceTypeToStr(device_properties->deviceType));
        log_trace("Physical device %d driver version: %u", i, device_properties->driverVersion);
        log_trace("Physical device %d API version: %d.%d.%d",
            i,
            VK_VERSION_MAJOR(device_properties->apiVersion),
            VK_VERSION_MINOR(device_properties->apiVersion),
            VK_VERSION_PATCH(device_properties->apiVersion));

        if (device_properties->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            device_information->score += 500;
        }

        device_information->swapchain_extension_supported = CheckVulkanPhysicalDeviceSwapchainExtensionSupport(physical_device[i]);
        log_trace("Physical device %d swapchain extension: %d", i, device_information->swapchain_extension_supported ? 1 : 0);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device[i], &device_information->queue_family_count, nullptr);
        uint32_t queue_family_count = device_information->queue_family_count;
        device_information->queue_family_properties = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
        if (device_information->queue_family_properties == nullptr) {
            for (uint64_t j = i; j > 0; j--) {
                free((*info)[j].queue_family_properties);
            }
            free(*info);
            RETURN_ERROR("Failed to allocate queue family properties", VK_ERROR_OUT_OF_HOST_MEMORY);
        }
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device[i], &queue_family_count, device_information->queue_family_properties);

        QueueFamilyInfo queue_family_info[queue_family_count];
        ConvertQueueFamilyProperties(device_information->queue_family_properties, queue_family_count, queue_family_info);

        log_trace(LINE_SEPARATOR);
        log_trace("Physical device %d has %d queue families", i, device_information->queue_family_count);
        log_trace(LINE_SEPARATOR);
        QueueFamilyInfo *global_queue_family_info = &device_information->global_queue_family_info;
        for (uint32_t j = 0; j < device_information->queue_family_count; j++) {
            global_queue_family_info->queue_flags = global_queue_family_info->queue_flags | queue_family_info[j].queue_flags;

            log_trace("Physical device %d queue family %d: graphics=%d, compute=%d, transfer=%d, sparse_binding=%d,"
                      " protected=%d, video_decode=%d, video_encode=%d, optical_flow=%d, data_graph=%d",
                      i, j,
                      queue_family_info[j].graphics_bit,
                      queue_family_info[j].compute_bit,
                      queue_family_info[j].transfer_bit,
                      queue_family_info[j].sparse_binding_bit,
                      queue_family_info[j].protected_bit,
                      queue_family_info[j].video_decode_bit,
                      queue_family_info[j].video_encode_bit,
                      queue_family_info[j].optical_flow_bit,
                      queue_family_info[j].data_graph_bit);

            const bool surface_support = glfwGetPhysicalDevicePresentationSupport(instance, physical_device[i], j);
            log_trace("Physical device %d queue family %d surface support: %d", i, j, surface_support);
            if (!device_information->queue_family_surface_support_present) {
                device_information->queue_family_surface_support_index = j;
                device_information->queue_family_surface_support_present = true;
                log_trace("Physical device %d queue family %d is chosen for presentation", i, j);
            }
        }
        if (global_queue_family_info->graphics_bit) {
            device_information->score += 100;
        }
        if (global_queue_family_info->compute_bit) {
            device_information->score += 100;
        }
        if (global_queue_family_info->transfer_bit) {
            device_information->score += 100;
        }
        if (global_queue_family_info->sparse_binding_bit) {
            device_information->score += 100;
        }
        if (global_queue_family_info->protected_bit) {
            device_information->score += 50;
        }
        if (global_queue_family_info->video_decode_bit) {
            device_information->score += 100;
        }
        if (global_queue_family_info->video_encode_bit) {
            device_information->score += 75;
        }
        if (global_queue_family_info->optical_flow_bit) {
            device_information->score += 5;
        }
        if (global_queue_family_info->data_graph_bit) {
            device_information->score += 5;
        }

        log_trace(LINE_SEPARATOR);
        log_trace("Getting physical device features");
        vkGetPhysicalDeviceFeatures(physical_device[i], &device_information->features);

        vkGetPhysicalDeviceMemoryProperties(physical_device[i], &device_information->memory_properties);
        log_trace("Physical device %d has %d memory heaps", i, device_information->memory_properties.memoryHeapCount);
        log_trace(LINE_SEPARATOR);
        for (uint32_t j = 0; j < device_information->memory_properties.memoryHeapCount; j++) {
            const int mult = GetScoreBasedOnHeapFlags(device_information->memory_properties.memoryHeaps[j].flags);
            const VkDeviceSize size_in_gib = device_information->memory_properties.memoryHeaps[j].size / (1024ull * 1024ull * 1024ull);
            uint64_t size_score = size_in_gib * mult;
            if (size_score > kMaxDeviceSizeScore) {
                size_score = kMaxDeviceSizeScore;
            }
            device_information->score += size_score;

            log_trace("Physical device %d memory heap %d size: %llu", i, j, device_information->memory_properties.memoryHeaps[j].size);
            log_trace("Physical device %d memory heap %d flags: %08X", i, j, device_information->memory_properties.memoryHeaps[j].flags);
            log_trace("Physical device %d memory heap %d score: %llu", i, j, size_score);
        }

        log_trace(LINE_SEPARATOR);
        log_trace("Physical device %d score: %llu", i, device_information->score);
        log_trace(LINE_SEPARATOR);
    }

    return VK_SUCCESS;
}

VkResult CreateVulkanDevice(const VkPhysicalDevice physical_device, VkDevice* vk_device, const uint32_t queue_family_index[], const uint32_t queue_family_count, const char * const extensions[], const uint32_t extension_count, char** error_message) {
    log_trace("Instructed to create a Vulkan device with %d extensions:", extension_count);
    for (uint32_t i = 0; i < extension_count; i++) {
        log_trace("Vulkan device extension: %s", extensions[i]);
    }

    VkDeviceQueueCreateInfo queue_infos[queue_family_count];
    constexpr float queue_priorities[] = { 1.0f };
    for (uint32_t i = 0; i < queue_family_count; i++) {
        log_trace("Vulkan device queue family index: %u", queue_family_index[i]);
        queue_infos[i] = (VkDeviceQueueCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queue_family_index[i],
            .queueCount = 1,
            .pQueuePriorities = &queue_priorities[0]
        };
    }

    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = queue_family_count,
        .pQueueCreateInfos = queue_infos,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
        .pEnabledFeatures = nullptr
    };

    const VkResult vk_result = vkCreateDevice(physical_device, &device_info, nullptr, vk_device);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to create logical device", vk_result);
    }

    return VK_SUCCESS;
}

bool GetBestPhysicalDeviceWithSwapchainAndPresentationSupport(const PhysicalDeviceInformation *properties, const uint32_t device_count, uint32_t *best_device, const VkQueueFlags required_flags) {
    bool found_candidate = false;
    uint32_t device_candidate = 0;
    uint64_t current_best_score = 0;
    for (uint32_t i = 0; i < device_count; i++) {
        if (!properties[i].swapchain_extension_supported) {
            continue;
        }
        if (!properties[i].queue_family_surface_support_present) {
            continue;
        }

        if ((properties[i].global_queue_family_info.queue_flags & required_flags) == required_flags) {
            if (current_best_score < properties[i].score) {
                current_best_score = properties[i].score;
                device_candidate = i;
                found_candidate = true;
            }
        }
    }

    *best_device = device_candidate;
    return found_candidate;
}

bool GetQueueFamilyIndexWithRequiredFlags(const uint32_t queue_family_count, const VkQueueFamilyProperties *queue_family_info, const VkQueueFlags required_flags,uint32_t *queue_family_index) {
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if ((queue_family_info[i].queueFlags & required_flags) == required_flags) {
            *queue_family_index = i;
            return true;
        }
    }
    return false;
}

bool CheckVulkanPhysicalDeviceSwapchainExtensionSupport(const VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    VkExtensionProperties available_extensions[extensionCount];
    const VkResult vk_result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available_extensions);
    if (vk_result != VK_SUCCESS) {
        return false;
    }
    for (uint32_t i = 0; i < extensionCount; i++) {
        const char * const swapchain_extension[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        if (strcmp(available_extensions[i].extensionName, swapchain_extension[0]) == 0) {
            return true;
        }
    }

    return false;
}