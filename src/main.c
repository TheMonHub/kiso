#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "error.h"
#include "logger.h"
#include "version.h"
#include "GLFW/glfw3.h"
#include "util/common_macro.h"
#include "util/enum_to_str.h"
#include "vulkan/vk_cleanup.h"
#include "vulkan/vk_init.h"
#include "vulkan/vk_shader.h"

static void PrintProjectInfo() {
    log_info("========== KISO ===========");
    log_info("Version: " PROJECT_VERSION_STRING);
    log_info("Version number: %d", kProjectVersionNumber);
#if defined(PROJECT_GITHUB_URL)
    log_debug("GitHub source code URL: " PROJECT_GITHUB_URL);
#endif
#if defined(PROJECT_GIT_COMMIT_HASH)
#if defined(PROJECT_GITHUB_URL)
    log_debug("GitHub commit URL: " PROJECT_GITHUB_URL "/commit/" PROJECT_GIT_COMMIT_HASH);
#endif
    log_debug("Git commit hash: " PROJECT_GIT_COMMIT_HASH);
#endif
#if defined(PROJECT_GIT_BRANCH)
    log_debug("Git branch: " PROJECT_GIT_BRANCH);
#endif
    log_info(LINE_SEPARATOR);
}

// TODO: Refactor cleanup using goto.
int main(const int argc, char *argv[]) {
#if defined(NDEBUG)
    log_set_level(DEFAULT, LOG_INFO);

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            log_set_level(DEFAULT, LOG_DEBUG);
            break;
        }
    }
#endif

    log_set_max_level(DEFAULT, LOG_INFO);
    log_add_stream_handler(stderr, LOG_WARN, LOG_LEVEL_MAX, "stderr");

    PrintProjectInfo();

    log_info("Initializing Application");

    log_debug("Initializing GLFW");
    if (!glfwInit()) {
        ExitOnError("Failed to initialize GLFW", 1);
    }
    glfwSetErrorCallback(GlfwErrorCallback);

    log_info("Creating a window");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "KISO", NULL, NULL);
    if (window == nullptr)
    {
        glfwTerminate();
        ExitOnError("Failed to create window", 1);
    }

    log_trace("Fetching required Vulkan instance extensions from GLFW");
    uint32_t glfw_extensions_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
    if (!glfw_extensions) {
        glfwTerminate();
        ExitOnError("Failed to get required Vulkan instance extensions", 1);
    }
    log_debug("Required Vulkan instance extensions count: %u", glfw_extensions_count);
    for (uint32_t i = 0; i < glfw_extensions_count; ++i) {
        log_debug("Required Vulkan instance extension: %s", glfw_extensions[i]);
    }

    log_info("Initializing Vulkan");
    log_trace("Creating Vulkan Instance");
    VkInstance instance;
    VkResult vk_result = CreateVulkanInstance(&instance, glfw_extensions, glfw_extensions_count, 0);
    if (vk_result != VK_SUCCESS) {
        glfwTerminate();
        ExitOnError("Failed to create Vulkan instance", vk_result);
    }

    log_trace("Creating window surface");
    VkSurfaceKHR surface;
    vk_result = glfwCreateWindowSurface(instance, window, NULL, &surface);
    if (vk_result != VK_SUCCESS)
    {
        glfwTerminate();
        ExitOnError("Failed to create window surface", vk_result);
    }

    log_debug("Getting Vulkan physical devices and its information");
    uint32_t physical_device_count;

    vk_result = vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    if (vk_result != VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError("Failed to get Vulkan physical devices count", vk_result);
    }
    if (physical_device_count == 0) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError("No Vulkan physical devices found", 1);
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    vk_result = vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);
    if (vk_result != VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError("Failed to get Vulkan physical devices", vk_result);
    }
    if (physical_device_count == 0) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError("No Vulkan physical devices found", 1);
    }
    log_debug("Found %u Vulkan physical devices", physical_device_count);

    PhysicalDeviceInformation *physical_device_info;
    char *error_str;
    vk_result = GetVulkanPhysicalDevicesInfo(instance, physical_devices, physical_device_count, &physical_device_info, surface, &error_str);
    if (vk_result != VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }

    log_info("Finding the most suitable physical device");
    uint32_t best_physical_device_index;
    const bool has_physical_device = GetBestPhysicalDeviceWithSwapchainAndPresentationSupport(physical_device_info, physical_device_count, &best_physical_device_index, VK_QUEUE_GRAPHICS_BIT);
    if (!has_physical_device) {
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError("Failed to find a suitable physical device", 1);
    }

    log_info("Suitable physical device found at index: %u: %s", best_physical_device_index, physical_device_info[best_physical_device_index].properties.deviceName);

    log_trace("Getting the graphics queue family");
    uint32_t graphic_queue_family;
    const bool has_graphic_queue_family = GetQueueFamilyIndexWithRequiredFlags(
        physical_device_info[best_physical_device_index].queue_family_count,
        physical_device_info[best_physical_device_index].queue_family_properties,
        VK_QUEUE_GRAPHICS_BIT,
        &graphic_queue_family);
    if (!has_graphic_queue_family) {
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError("Failed to find a suitable graphics queue family", 1);
    }
    log_debug("Found the graphics queue family: %u", graphic_queue_family);

    const uint32_t present_queue_family = physical_device_info[best_physical_device_index].queue_family_surface_support_index;
    log_debug("Presentation queue family: %u", present_queue_family);

    log_debug("Creating Vulkan device");
    VkDevice device;
    const char * const device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    uint32_t queue_families[2];
    uint32_t queue_family_count = 1;
    queue_families[0] = graphic_queue_family;
    if (present_queue_family != graphic_queue_family) {
        log_debug("Presentation queue family is different from graphics queue family");
        queue_families[1] = present_queue_family;
        queue_family_count = 2;
    }

    vk_result = CreateVulkanDevice(physical_devices[best_physical_device_index], &device, queue_families, queue_family_count, device_extensions, ARRAY_SIZE(device_extensions), &error_str);
    if (vk_result != VK_SUCCESS) {
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }

    SwapchainSupportDetails swapchain_support_details;
    vk_result = GetSwapchainSupportDetails(physical_devices[best_physical_device_index], surface, &swapchain_support_details, &error_str);
    if (vk_result != VK_SUCCESS) {
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }

    VkSurfaceFormatKHR *surface_format;
    VkPresentModeKHR present_mode;
    const bool is_preferred_surface_format = ChoosePreferredSurfaceFormat(swapchain_support_details.formats, swapchain_support_details.format_count, VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, &surface_format);
    const bool is_preferred_present_mode = choosePreferredPresentMode(swapchain_support_details.present_modes, swapchain_support_details.present_mode_count, VK_PRESENT_MODE_MAILBOX_KHR, &present_mode);

    if (!is_preferred_surface_format) {
        log_info("Preferred surface format not found, using first available format");
    }
    if (!is_preferred_present_mode) {
        log_info("Preferred present mode not found, using first available present mode");
    }

    const VkExtent2D extent = chooseVulkanExtent(&swapchain_support_details.capabilities, window);

    const CreateSwapchainInfo create_swapchain_info = {
        .device = device,
        .swapchain_support = &swapchain_support_details,
        .surface_format = surface_format,
        .present_mode = present_mode,
        .extent = &extent,
        .surface = &surface,
        .image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .graphics_family_index = graphic_queue_family,
        .present_family_index = present_queue_family,
        .composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    };

    VkSwapchainKHR swapchain;
    VkImage *images;
    uint32_t image_count;

    vk_result = CreateVulkanSwapchain(&create_swapchain_info, &swapchain, &images, &image_count, &error_str);
    if (vk_result != VK_SUCCESS) {
        CleanupSwapchainSupportDetails(&swapchain_support_details);
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }

    VkImageView image_views[image_count];
    vk_result = CreateImageViews(device, images, image_count, image_views, surface_format->format, &error_str);
    if (vk_result != VK_SUCCESS) {
        CleanupSwapchainSupportDetails(&swapchain_support_details);
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }

    constexpr unsigned char vertex_shader_code[] = {
#embed "shader.vert.spv"
    };
    constexpr unsigned char fragment_shader_code[] = {
#embed "shader.frag.spv"
    };
    constexpr size_t vertex_shader_code_size = sizeof(vertex_shader_code);
    constexpr size_t fragment_shader_code_size = sizeof(fragment_shader_code);
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;

    vk_result = CreateShaderModule(vertex_shader_code, vertex_shader_code_size, device, &vertex_shader_module, &error_str);
    if (vk_result != VK_SUCCESS) {
        CleanupSwapchainSupportDetails(&swapchain_support_details);
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }
    vk_result = CreateShaderModule(fragment_shader_code, fragment_shader_code_size, device, &fragment_shader_module, &error_str);
    if (vk_result != VK_SUCCESS) {
        vkDestroyShaderModule(device, vertex_shader_module, nullptr);
        CleanupSwapchainSupportDetails(&swapchain_support_details);
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }

    const VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    log_debug("Creating graphics pipeline");

    vk_result = CreateGraphicsPipeline(device,
        vertex_shader_module,
        fragment_shader_module,
        &vertex_input_info,
        &extent,
        &pipeline_layout,
        &render_pass,
        &pipeline,
        surface_format->format,
        &error_str);

    if (vk_result != VK_SUCCESS) {
        vkDestroyShaderModule(device, fragment_shader_module, nullptr);
        vkDestroyShaderModule(device, vertex_shader_module, nullptr);
        CleanupSwapchainSupportDetails(&swapchain_support_details);
        CleanupPhysicalDeviceInformation(physical_device_info);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        log_error("Failed to create graphics pipeline: %d", vk_result);
        ExitOnError(error_str, vk_result);
    }

    // while (!glfwWindowShouldClose(window)) {
    //     glfwPollEvents();
    // }

    log_info("Clearing resources");
    CleanupImageViews(device, image_views, image_count);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyShaderModule(device, fragment_shader_module, nullptr);
    vkDestroyShaderModule(device, vertex_shader_module, nullptr);
    CleanupSwapchainSupportDetails(&swapchain_support_details);
    CleanupPhysicalDeviceInformation(physical_device_info);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();

    return 0;
}
