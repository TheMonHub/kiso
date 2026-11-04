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
#include "vulkan/vk_render.h"
#include "glfw/glfw_callbacks.h"

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

int main(const int argc, char *argv[]) {
#if defined(NDEBUG)
    log_set_level(DEFAULT, LOG_INFO);

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            log_set_level(DEFAULT, LOG_DEBUG);
            break;
        }
    }
#else
    log_set_level(DEFAULT, LOG_DEBUG);

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--trace") == 0) {
            log_set_level(DEFAULT, LOG_TRACE);
            break;
        }
    }
#endif

    int _current_error_status = 0;

    log_set_max_level(DEFAULT, LOG_INFO);
    log_add_stream_handler(stderr, LOG_WARN, LOG_LEVEL_MAX, "stderr");

    PrintProjectInfo();

    log_info("Initializing Application");

    log_debug("Initializing GLFW");
    if (!glfwInit()) {
        ExitOnError("Failed to initialize GLFW", 1);
    }
    glfwSetErrorCallback(GlfwErrorCallback);

    constexpr int desired_window_width = 640;
    constexpr int desired_window_height = 480;

    log_info("Creating a window");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(desired_window_width, desired_window_height, "KISO", NULL, NULL);
    if (window == nullptr)
    {
        PrintAndGotoIfError("Failed to create window", 1, glfw_cleanup);
    }

    int win_size_width, win_size_height;
    glfwGetWindowSize(window, &win_size_width, &win_size_height);

    log_debug("Desired window size: %d x %d", desired_window_width, desired_window_height);
    log_debug("Initial window size: %d x %d", win_size_width, win_size_height);

    if (win_size_width != desired_window_width || win_size_height != desired_window_height) {
        log_warn("Window size does not match desired size. Desired size: %d x %d Actual size: %d x %d", desired_window_width, desired_window_height, win_size_width, win_size_height);
        log_warn("Resizing window to desired size");

        log_debug("Resizing window since it does not match the desired size initially");
        glfwSetWindowSize(window, 640, 480);
        log_debug("Window size resized to: %d x %d", desired_window_width, desired_window_height);

        glfwGetWindowSize(window, &win_size_width, &win_size_height);
        log_debug("Actual window size after resize: %d x %d", win_size_width, win_size_height);

        if (win_size_width != desired_window_width || win_size_height != desired_window_height) {
            log_error("Window size does not match desired size after resize. Desired size: %d x %d Actual size: %d x %d", desired_window_width, desired_window_height, win_size_width, win_size_height);
        }
    }

    log_debug("Fetching required Vulkan instance extensions from GLFW");
    uint32_t glfw_extensions_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
    if (!glfw_extensions) {
        PrintAndGotoIfError("Failed to get required Vulkan instance extensions", 1, glfw_cleanup);
    }
    log_debug("Required Vulkan instance extensions count: %u", glfw_extensions_count);
    for (uint32_t i = 0; i < glfw_extensions_count; ++i) {
        log_debug("Required Vulkan instance extension: %s", glfw_extensions[i]);
    }

    log_info("Initializing Vulkan");
    log_debug("Creating Vulkan Instance");
    VkInstance instance;
    VkResult vk_result = CreateVulkanInstance(&instance, glfw_extensions, glfw_extensions_count, 0);
    if (vk_result != VK_SUCCESS) {

        PrintAndGotoIfError("Failed to create Vulkan instance", vk_result, glfw_cleanup);
    }

    log_debug("Creating window surface");
    VkSurfaceKHR surface;
    vk_result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (vk_result != VK_SUCCESS)
    {
        PrintAndGotoIfError("Failed to create window surface", vk_result, vkinstance_cleanup);
    }

    log_debug("Getting Vulkan physical devices and its information");
    uint32_t physical_device_count;

    vk_result = vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError("Failed to get Vulkan physical devices count", vk_result, vksurface_cleanup);
    }
    if (physical_device_count == 0) {
        PrintAndGotoIfError("No Vulkan physical devices found", 1, vksurface_cleanup);
    }

    auto physical_devices = (VkPhysicalDevice *) malloc(sizeof(VkPhysicalDevice) * physical_device_count);
    if (!physical_devices) {
        PrintAndGotoIfError("Failed to allocate memory for Vulkan physical devices", 1, vksurface_cleanup);
    }
    vk_result = vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError("Failed to get Vulkan physical devices", vk_result, physicaldevicesmalloc_cleanup);
    }
    if (physical_device_count == 0) {
        PrintAndGotoIfError("No Vulkan physical devices found", 1, physicaldevicesmalloc_cleanup);
    }
    log_debug("Found %u Vulkan physical devices", physical_device_count);

    PhysicalDeviceInformation *physical_device_info;
    char *error_str;
    vk_result = GetVulkanPhysicalDevicesInfo(instance, physical_devices, physical_device_count, &physical_device_info, surface, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, physicaldevicesmalloc_cleanup);
    }

    log_info("Finding the most suitable physical device");
    uint32_t best_physical_device_index;
    const bool has_physical_device = GetBestPhysicalDeviceWithSwapchainAndPresentationSupport(physical_device_info, physical_device_count, &best_physical_device_index, VK_QUEUE_GRAPHICS_BIT);
    if (!has_physical_device) {
        PrintAndGotoIfError("Failed to find a suitable physical device", 1, vkphysicaldeviceinfo_cleanup);
    }

    log_info("Suitable physical device found at index: %u: %s", best_physical_device_index, physical_device_info[best_physical_device_index].properties.deviceName);

    log_debug("Getting the graphics queue family");
    uint32_t graphic_queue_family;
    const bool has_graphic_queue_family = GetQueueFamilyIndexWithRequiredFlags(
        physical_device_info[best_physical_device_index].queue_family_count,
        physical_device_info[best_physical_device_index].queue_family_properties,
        VK_QUEUE_GRAPHICS_BIT,
        &graphic_queue_family);
    if (!has_graphic_queue_family) {
        PrintAndGotoIfError("Failed to find a suitable graphics queue family", 1, vkphysicaldeviceinfo_cleanup);
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
        PrintAndGotoIfError(error_str, vk_result, vkphysicaldeviceinfo_cleanup);
    }

    log_debug("Getting graphics queue");
    VkQueue graphics_queue;
    vkGetDeviceQueue(device, graphic_queue_family, 0, &graphics_queue);
    log_debug("Getting presentation queue");
    VkQueue present_queue;
    vkGetDeviceQueue(device, present_queue_family, 0, &present_queue);

    log_debug("Getting swapchain support details");
    SwapchainSupportDetails swapchain_support_details;
    vk_result = GetSwapchainSupportDetails(physical_devices[best_physical_device_index], surface, &swapchain_support_details, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, vkdevice_cleanup);
    }

    VkSurfaceFormatKHR *surface_format;
    VkPresentModeKHR present_mode;
    const bool is_preferred_surface_format = ChoosePreferredSurfaceFormat(swapchain_support_details.formats, swapchain_support_details.format_count, VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, &surface_format);
    const bool is_preferred_present_mode = ChoosePreferredPresentMode(swapchain_support_details.present_modes, swapchain_support_details.present_mode_count, VK_PRESENT_MODE_MAILBOX_KHR, &present_mode);

    if (!is_preferred_surface_format) {
        log_info("Preferred surface format not found, using first available format");
    }
    if (!is_preferred_present_mode) {
        log_info("Preferred present mode not found, using first available present mode");
    }

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    VkExtent2D extent = ChooseVulkanExtent(&swapchain_support_details.capabilities, framebuffer_width, framebuffer_height);

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

    log_debug("Creating Vulkan swapchain");
    vk_result = CreateVulkanSwapchain(&create_swapchain_info, &swapchain, &images, &image_count, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, swapchainsupportdetails_cleanup);
    }

    log_debug("Creating Vulkan image views");
    auto image_views = (VkImageView *) malloc(image_count * sizeof(VkImageView));
    if (!image_views) {
        PrintAndGotoIfError("Failed to allocate memory for image views", 1, vkswapchain_cleanup);
    }
    vk_result = CreateImageViews(device, images, image_count, image_views, surface_format->format, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, imageviewsmalloc_cleanup);
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

    log_debug("Creating Vulkan shader modules for vertex and fragment shaders");
    vk_result = CreateShaderModule(vertex_shader_code, vertex_shader_code_size, device, &vertex_shader_module, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, imageviews_cleanup);
    }
    vk_result = CreateShaderModule(fragment_shader_code, fragment_shader_code_size, device, &fragment_shader_module, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, vertshadermod_cleanup);
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
        PrintAndGotoIfError(error_str, vk_result, fragshadermod_cleanup);
    }

    auto framebuffers = (VkFramebuffer *) malloc(sizeof(VkFramebuffer) * image_count);
    if (!framebuffers) {
        PrintAndGotoIfError("Failed to allocate memory for framebuffers", 1, pipeline_cleanup);
    }

    vk_result = CreateFramebuffers(device, image_views, image_count, render_pass, framebuffers, &extent, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, pipeline_cleanup);
    }

    log_debug("Creating command pool");
    VkCommandPool command_pool;
    vk_result = CreateCommandPool(device, graphic_queue_family, &command_pool, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, framebuffers_cleanup);
    }

    log_debug("Creating command buffers");
    auto command_buffers = (VkCommandBuffer *) malloc(sizeof(VkCommandBuffer) * kMaxFramesInFlight);
    vk_result = CreateCommandBuffers(device, command_pool, kMaxFramesInFlight, command_buffers, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, commandpool_cleanup);
    }

    log_debug("Recording sync objects");
    auto image_available_semaphores = (VkSemaphore *) malloc(sizeof(VkSemaphore));
    auto render_finished_semaphores = (VkSemaphore *) malloc(sizeof(VkSemaphore) * image_count);
    auto in_flight_fence = (VkFence *) malloc(sizeof(VkFence));
    vk_result = CreateSyncObjects(device, &in_flight_fence, &image_available_semaphores, &render_finished_semaphores, kMaxFramesInFlight, image_count, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndGotoIfError(error_str, vk_result, commandbuffersmalloc_cleanup);
    }

    log_debug("Setting up GLFW window callbacks");
    bool is_framebuffer_resized = false;
    glfwSetWindowUserPointer(window, &is_framebuffer_resized);
    glfwSetFramebufferSizeCallback(window, (GLFWframebuffersizefun) GlfwCallbackSetPointerTrue);

    uint32_t current_frame = 0;

    log_info("Starting main loop");
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        RenderContext render_context = {
            .device = device,
            .graphics_queue = graphics_queue,
            .swapchain = &swapchain,
            .command_buffers = command_buffers,
            .render_pass = render_pass,
            .framebuffers = &framebuffers,
            .pipeline = pipeline,
            .in_flight_fences = in_flight_fence,
            .image_available_semaphores = image_available_semaphores,
            .render_finished_semaphores = render_finished_semaphores,
            .extent = &extent,
            .current_frame = &current_frame
        };

        SwapchainRecreationInfo swapchain_recreation_info = {
            .swapchain_support = &swapchain_support_details,
            .surface_format = surface_format,
            .present_mode = present_mode,
            .surface = &surface,
            .graphics_family_index = graphic_queue_family,
            .present_family_index = present_queue_family,
            .composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .image_count = image_count,
            .images = &images,
            .image_views = &image_views,
            .window = window
        };

        if (is_framebuffer_resized) {
            is_framebuffer_resized = false;
            vk_result = RecreateSwapchain(&render_context, &swapchain_recreation_info, &error_str);
            if (vk_result == VK_ERROR_UNKNOWN && error_str == nullptr) {
                break;
            }
            if (vk_result != VK_SUCCESS) {
                PrintAndGotoIfError(error_str, vk_result, syncobjects_cleanup);
            }
        }
        vk_result = DrawFrame(&render_context, &swapchain_recreation_info, &error_str);
        // this means window is closed. Do not error.
        if (vk_result == VK_ERROR_UNKNOWN && error_str == nullptr) {
            break;
        }
        if (vk_result != VK_SUCCESS) {
            PrintAndGotoIfError(error_str, vk_result, syncobjects_cleanup);
        }
    }

    log_info("Waiting for device to become idle before cleanup");
    vkDeviceWaitIdle(device);

    log_info("Clearing resources");

    syncobjects_cleanup:
    CleanupSemaphores(device, image_available_semaphores, kMaxFramesInFlight);
    CleanupSemaphores(device, render_finished_semaphores, image_count);
    CleanupFences(device, in_flight_fence, kMaxFramesInFlight);
    free((void *) image_available_semaphores);
    free((void *) render_finished_semaphores);
    free((void *) in_flight_fence);
    commandbuffersmalloc_cleanup:
    free((void *) command_buffers);
    commandpool_cleanup:
    vkDestroyCommandPool(device, command_pool, nullptr);
    framebuffers_cleanup:
    CleanupFramebuffers(device, framebuffers, image_count);
    framebuffersmalloc_cleanup:
    free((void *) framebuffers);
    pipeline_cleanup:
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    fragshadermod_cleanup:
    vkDestroyShaderModule(device, fragment_shader_module, nullptr);
    vertshadermod_cleanup:
    vkDestroyShaderModule(device, vertex_shader_module, nullptr);
    imageviews_cleanup:
    CleanupImageViews(device, image_views, image_count);
    imageviewsmalloc_cleanup:
    free((void *) image_views);
    vkswapchain_cleanup:
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    imagesmalloc_cleanup:
    free((void *) images);
    swapchainsupportdetails_cleanup:
    CleanupSwapchainSupportDetails(&swapchain_support_details);
    vkdevice_cleanup:
    vkDestroyDevice(device, nullptr);
    vkphysicaldeviceinfo_cleanup:
    CleanupPhysicalDeviceInformation(physical_device_info);
    physicaldevicesmalloc_cleanup:
    free((void *) physical_devices);
    vksurface_cleanup:
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkinstance_cleanup:
    vkDestroyInstance(instance, nullptr);
    glfw_cleanup:
    glfwTerminate();

    return _current_error_status;
}
