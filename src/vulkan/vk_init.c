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
#include "vk_shader.h"
#include "util/enum_to_str.h"
#include "util/math.h"

#define RETURN_ERROR(msg, code) \
    *error_message = msg; \
    return code

static constexpr uint64_t kMaxDeviceSizeScore = 1280;

VkResult CreateVulkanInstance(VkInstance* instance, const char** extensions, const uint32_t extensions_count, const VkInstanceCreateFlags flags) {
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

#if !defined(NDEBUG)
    const char *actual_extensions[extensions_count + 1];
    memcpy((void*)actual_extensions, (const void*)extensions, extensions_count * sizeof(char*));
    actual_extensions[extensions_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    log_info("Enabling Vulkan validation layers");
    static const char *validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
#endif
    const VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pApplicationInfo = &application_info,
#if !defined(NDEBUG)
        .enabledLayerCount = ARRAY_SIZE(validation_layers),
        .ppEnabledLayerNames = validation_layers,
        .enabledExtensionCount = extensions_count + 1,
        .ppEnabledExtensionNames = actual_extensions
#else
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = extensions_count,
        .ppEnabledExtensionNames = extensions
#endif
      };

    return vkCreateInstance(&instance_create_info, nullptr, instance);
}

static int GetScoreBasedOnHeapFlags(const VkFlags flags) {
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
}

// The caller of this function is responsible for freeing the memory allocated for the properties array.
// Being the PhysicalDeviceInformation array, queue_family_properties array inside each PhysicalDeviceInformation entry all from the returned array.
VkResult GetVulkanPhysicalDevicesInfo(const VkInstance instance, const VkPhysicalDevice *physical_device, const uint32_t device_count,
    PhysicalDeviceInformation **info, const VkSurfaceKHR surface, char** error_message) {
    log_trace("Instructed to get Vulkan physical devices information");

    *info = calloc(device_count, sizeof(PhysicalDeviceInformation));
    if (*info == nullptr) {
        RETURN_ERROR("Failed to allocate physical device properties", VK_ERROR_OUT_OF_HOST_MEMORY);
    }

    for (uint32_t i = 0; i < device_count; i++) {
        PhysicalDeviceInformation *device_information = &(*info)[i];

        vkGetPhysicalDeviceProperties(physical_device[i], &device_information->properties);
        VkPhysicalDeviceProperties *device_properties = &device_information->properties;

        log_debug(LINE_SEPARATOR);
        log_debug("Physical device %d", i, device_information->queue_family_count);
        log_debug(LINE_SEPARATOR);
        log_debug("Physical device %d name: %s", i, device_properties->deviceName);
        log_debug("Physical device %d type: %s", i, VulkanDeviceTypeToStr(device_properties->deviceType));
        log_debug("Physical device %d driver version: %u", i, device_properties->driverVersion);
        log_debug("Physical device %d API version: %d.%d.%d",
            i,
            VK_VERSION_MAJOR(device_properties->apiVersion),
            VK_VERSION_MINOR(device_properties->apiVersion),
            VK_VERSION_PATCH(device_properties->apiVersion));

        if (device_properties->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            device_information->score += 50;
        }

        static const char * const swapchain_extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        device_information->swapchain_extension_supported = CheckVulkanPhysicalDeviceExtensionSupport(physical_device[i], swapchain_extensions, 1);
        log_debug("Physical device %d swapchain extension: %d", i, device_information->swapchain_extension_supported ? 1 : 0);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device[i], &device_information->queue_family_count, nullptr);
        uint32_t queue_family_count = device_information->queue_family_count;
        device_information->queue_family_properties = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
        if (device_information->queue_family_properties == nullptr) {
            for (uint32_t j = 0; j < i; j++) {
                free((*info)[j].queue_family_properties);
            }
            free(*info);
            RETURN_ERROR("Failed to allocate queue family properties", VK_ERROR_OUT_OF_HOST_MEMORY);
        }
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device[i], &queue_family_count, device_information->queue_family_properties);

        QueueFamilyInfo queue_family_info[queue_family_count];
        ConvertQueueFamilyProperties(device_information->queue_family_properties, queue_family_count, queue_family_info);

        log_debug(LINE_SEPARATOR);
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

            VkBool32 surface_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device[i], j, surface, &surface_support);
            log_trace("Physical device %d queue family %d surface support: %d", i, j, surface_support);
            if (!device_information->queue_family_surface_support_present && surface_support) {
                device_information->queue_family_surface_support_index = j;
                device_information->queue_family_surface_support_present = true;
                log_trace("Physical device %d queue family %d is chosen for presentation", i, j);
            }
        }

        ConvertQueueFlags(global_queue_family_info->queue_flags, &device_information->global_queue_family_info);

        if (global_queue_family_info->graphics_bit) {
            device_information->score += 50;
        }
        if (global_queue_family_info->compute_bit) {
            device_information->score += 50;
        }
        if (global_queue_family_info->transfer_bit) {
            device_information->score += 50;
        }
        if (global_queue_family_info->sparse_binding_bit) {
            device_information->score += 50;
        }
        if (global_queue_family_info->protected_bit) {
            device_information->score += 25;
        }
        if (global_queue_family_info->video_decode_bit) {
            device_information->score += 50;
        }
        if (global_queue_family_info->video_encode_bit) {
            device_information->score += 50;
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

        log_debug(LINE_SEPARATOR);
        log_debug("Physical device %d score: %llu", i, device_information->score);
        log_debug(LINE_SEPARATOR);
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
        if (properties[i].properties.apiVersion < CURRENT_VK_API_VERSION) {
            continue;
        }
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

bool CheckVulkanPhysicalDeviceExtensionSupport(const VkPhysicalDevice device, const char *const *extensions, const uint32_t extensions_count) {
    uint32_t extension_count;
    VkResult vk_result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    if (vk_result != VK_SUCCESS) {
        return false;
    }

    VkExtensionProperties available_extensions[extension_count];
    vk_result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions);
    if (vk_result != VK_SUCCESS) {
        return false;
    }

    uint32_t found_extensions_count = 0;

    for (uint32_t i = 0; i < extensions_count; i++) {
        for (uint32_t j = 0; j < extension_count; j++) {
            if (strcmp(available_extensions[j].extensionName, extensions[i]) == 0) {
                found_extensions_count++;
                break;
            }
        }
    }

    if (found_extensions_count == extensions_count) {
        return true;
    }

    return false;
}

bool CheckVulkanInstanceExtensionSupport(const char *const *extensions, const uint32_t extensions_count) {
    uint32_t extension_count;
    VkResult vk_result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (vk_result != VK_SUCCESS) {
        return false;
    }

    VkExtensionProperties available_extensions[extension_count];
    vk_result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions);
    if (vk_result != VK_SUCCESS) {
        return false;
    }

    uint32_t found_extensions_count = 0;

    for (uint32_t i = 0; i < extensions_count; i++) {
        for (uint32_t j = 0; j < extension_count; j++) {
            if (strcmp(available_extensions[j].extensionName, extensions[i]) == 0) {
                found_extensions_count++;
                break;
            }
        }
    }

    if (found_extensions_count == extensions_count) {
        return true;
    }

    return false;
}

// present_modes and formats are the caller's responsibility to free.
VkResult GetSwapchainSupportDetails(const VkPhysicalDevice device, const VkSurfaceKHR surface, SwapchainSupportDetails *details, char** error_message) {
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult vk_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to get swapchain surface capabilities", vk_result);
    }

    uint32_t format_count;
    vk_result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to get swapchain surface formats", vk_result);
    }
    if (format_count == 0) {
        RETURN_ERROR("No swapchain surface formats found", VK_ERROR_UNKNOWN);
    }

    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    if (formats == nullptr) {
        RETURN_ERROR("Failed to allocate memory for swapchain surface formats", VK_ERROR_OUT_OF_HOST_MEMORY);
    }
    vk_result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats);
    if (vk_result != VK_SUCCESS) {
        free(formats);
        RETURN_ERROR("Failed to get swapchain surface formats", vk_result);
    }

    uint32_t present_mode_count;
    vk_result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
    if (vk_result != VK_SUCCESS) {
        free(formats);
        RETURN_ERROR("Failed to get swapchain present modes", vk_result);
    }
    if (present_mode_count == 0) {
        free(formats);
        RETURN_ERROR("No swapchain present modes found", VK_ERROR_UNKNOWN);
    }

    VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
    if (present_modes == nullptr) {
        free(formats);
        RETURN_ERROR("Failed to allocate memory for swapchain present modes", VK_ERROR_OUT_OF_HOST_MEMORY);
    }
    vk_result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, present_modes);
    if (vk_result != VK_SUCCESS) {
        free(formats);
        free(present_modes);
        RETURN_ERROR("Failed to get swapchain present modes", vk_result);
    }

    details->formats = formats;
    details->format_count = format_count;
    details->present_modes = present_modes;
    details->present_mode_count = present_mode_count;
    details->capabilities = capabilities;

    return VK_SUCCESS;
}

bool ChoosePreferredSurfaceFormat(const VkSurfaceFormatKHR* surface_formats, const uint32_t surface_format_count, const VkFormat preferred_format, const VkColorSpaceKHR preferred_color_space, VkSurfaceFormatKHR **surface_format) {
    for (uint32_t i = 0; i < surface_format_count; i++) {
        if (surface_formats[i].format == preferred_format && surface_formats[i].colorSpace == preferred_color_space) {
            *surface_format = (VkSurfaceFormatKHR *) &surface_formats[i];
            return true;
        }
    }
    *surface_format = (VkSurfaceFormatKHR *) &surface_formats[0];
    return false;
}

bool choosePreferredPresentMode(const VkPresentModeKHR* present_modes, const uint32_t present_mode_count, const VkPresentModeKHR preferred_present_mode, VkPresentModeKHR *present_mode) {
    for (uint32_t i = 0; i < present_mode_count; i++) {
        if (present_modes[i] == preferred_present_mode) {
            *present_mode = present_modes[i];
            return true;
        }
    }
    *present_mode = present_modes[0];
    return false;
}

VkExtent2D chooseVulkanExtent(const VkSurfaceCapabilitiesKHR *capabilities, GLFWwindow *window) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return (VkExtent2D) {
        .width = UnsignedClamp((uint32_t) width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width),
        .height = UnsignedClamp((uint32_t) height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height),
    };
}

// images are the caller's responsibility to free.
VkResult CreateVulkanSwapchain(const CreateSwapchainInfo *info, VkSwapchainKHR *swapchain, VkImage **images, uint32_t *image_count, char** error_message) {
    log_trace("Instructed to create a Vulkan swapchain");
    log_trace("Using surface format: format=%08X, color_space=%08X", info->surface_format->format, info->surface_format->colorSpace);
    log_trace("Using present mode: %s", VkPresentModeToStr(info->present_mode));

    uint32_t image_request_count = info->swapchain_support->capabilities.minImageCount + 1;
    if (info->swapchain_support->capabilities.maxImageCount > 0 && image_request_count > info->swapchain_support->capabilities.maxImageCount) {
        image_request_count = info->swapchain_support->capabilities.maxImageCount;
    }
    log_trace("Using image count: %u", image_request_count);

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = *info->surface,
        .minImageCount = image_request_count,
        .imageFormat = info->surface_format->format,
        .imageColorSpace = info->surface_format->colorSpace,
        .imageExtent = *info->extent,
        .imageArrayLayers = 1,
        .imageUsage = info->image_usage,
        .preTransform = info->swapchain_support->capabilities.currentTransform,
        .compositeAlpha = info->composite_alpha,
        .presentMode = info->present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if (info->graphics_family_index != info->present_family_index) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = (uint32_t[2]) {info->graphics_family_index, info->present_family_index};
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    VkResult vk_result = vkCreateSwapchainKHR(info->device, &create_info, nullptr, swapchain);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to create Vulkan swapchain", vk_result);
    }

    vk_result = vkGetSwapchainImagesKHR(info->device, *swapchain, image_count, nullptr);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to get swapchain images", vk_result);
    }

    *images = (VkImage *) malloc(sizeof(VkImage) * *image_count);
    if (images == nullptr) {
        RETURN_ERROR("Failed to allocate memory for swapchain images", VK_ERROR_OUT_OF_HOST_MEMORY);
    }

    vk_result = vkGetSwapchainImagesKHR(info->device, *swapchain, image_count, *images);
    if (vk_result != VK_SUCCESS) {
        free((void *)*images);
        RETURN_ERROR("Failed to get swapchain images", vk_result);
    }
    return VK_SUCCESS;
}

VkResult CreateImageViews(const VkDevice device, const VkImage *images, const uint32_t image_count, VkImageView *image_views, const VkFormat swapchain_img_format, char** error_message) {
    for (uint32_t i = 0; i < image_count; ++i) {
        const VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain_img_format,
            .components = (VkComponentMapping) {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = (VkImageSubresourceRange) {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        const VkResult vk_result = vkCreateImageView(device, &create_info, nullptr, &image_views[i]);
        if (vk_result != VK_SUCCESS) {
            RETURN_ERROR("Failed to create image views", vk_result);
        }
    }

    return VK_SUCCESS;
}

VkResult CreateGraphicsPipeline(const VkDevice device, const VkShaderModule vertex_shader_module, const VkShaderModule fragment_shader_module,
    const VkPipelineVertexInputStateCreateInfo *vertex_input_info, const VkExtent2D *extent, VkPipelineLayout *pipeline_layout, VkRenderPass *render_pass,
    VkPipeline *pipeline, VkFormat image_format, char **error_message) {
    log_trace("Instructed to create a Vulkan graphics pipeline");
    const VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex_shader_module,
        .pName = "main"
    };
    const VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment_shader_module,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = ARRAY_SIZE(dynamicStates),
        .pDynamicStates = dynamicStates
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)extent->width,
        .height = (float)extent->height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {.x = 0, .y = 0},
        .extent = *extent
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    // VkPipelineDepthStencilStateCreateInfo depth_stencil

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
    };

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    log_trace("Creating pipeline layout");
    VkResult vk_result = vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, pipeline_layout);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to create pipeline layout", vk_result);
    }

    VkAttachmentDescription color_attachment = {
        .flags = 0,
        .format = image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
        .pDependencies = nullptr
    };

    log_trace("Creating render pass");
    vk_result = vkCreateRenderPass(device, &render_pass_info, nullptr, render_pass);
    if (vk_result != VK_SUCCESS) {
        vkDestroyPipelineLayout(device, *pipeline_layout, nullptr);
        RETURN_ERROR("Failed to create render pass", vk_result);
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = ARRAY_SIZE(shader_stages),
        .pStages = shader_stages,
        .pVertexInputState = vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = *pipeline_layout,
        .renderPass = *render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    log_trace("Creating graphics pipeline");
    vk_result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, pipeline);
    if (vk_result != VK_SUCCESS) {
        vkDestroyPipelineLayout(device, *pipeline_layout, nullptr);
        vkDestroyRenderPass(device, *render_pass, nullptr);
        RETURN_ERROR("Failed to create graphics pipeline", vk_result);
    }

    return VK_SUCCESS;
}