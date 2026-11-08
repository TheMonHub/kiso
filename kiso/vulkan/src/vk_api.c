// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

// ReSharper disable CppParameterMayBeConstPtrOrRef
#include "kiso/vk_api.h"

#include <stdlib.h>

#include "vk_init.h"
#include <string.h>
#include <GLFW/glfw3.h>

#include "logger.h"
#include "vk_shader.h"
#include "kiso/common.h"
#include "vk_list.h"
#include "vk_render.h"
#include "kiso/glfw/glfw_callbacks.h"
#include "kiso/error.h"

GLFWAPI VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow *window, const VkAllocationCallbacks *allocator, VkSurfaceKHR *surface);

typedef struct VulkanWindowData {
    struct VulkanWindowData *next;
    GLFWwindow *window;
    VkSurfaceKHR surface;
    VkExtent2D extent;
    SwapchainSupportDetails swapchain_support_details;
    VkSurfaceFormatKHR *surface_format;
    VkPresentModeKHR present_mode;
    VkSwapchainKHR swapchain;
    VkImage *images;
    VkImageView *image_views;
    uint32_t image_count;
    VulkanPipeline *pipeline;
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_finished_semaphores;
    VkFence *in_flight_fences;
    uint32_t max_frame_in_flight;
    uint32_t current_frame;
    GlfwCallbackStruct glfw_callback_struct;
} VulkanWindowData;

typedef struct VulkanBackendData {
    VkInstance instance;
    VkPhysicalDevice *physical_devices;
    uint32_t physical_device_count;
    PhysicalDeviceInformation *physical_device_info;
    uint32_t best_physical_device_index;
    uint32_t graphic_queue_family;
    uint32_t present_queue_family;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkDevice device;
    VulkanShaderModule *def_vert_shader;
    VulkanShaderModule *def_frag_shader;
    VulkanWindowData *window_data;
    VulkanWindowData *window_data_tail;
    VulkanShaderModuleList *shader_modules;
    VulkanPipelineList *pipelines;
} BackendData;

static uint32_t Init(GraphicsApi *api) {
    log_info("Initializing Vulkan");

    api->backend_data = malloc(sizeof(BackendData));
    BackendData *data = api->backend_data;
    if (!data) {
        PrintAndReturn("Failed to allocated for backend data", 1);
    }
    memset(data, 0, sizeof(BackendData));

    log_debug("Fetching required Vulkan instance extensions from GLFW");
    uint32_t glfw_extensions_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
    if (!glfw_extensions) {
        PrintAndReturn("Failed to get required Vulkan instance extensions", 1);
    }
    log_debug("Required Vulkan instance extensions count: %u", glfw_extensions_count);
    for (uint32_t i = 0; i < glfw_extensions_count; ++i) {
        log_debug("Required Vulkan instance extension: %s", glfw_extensions[i]);
    }

    log_debug("Creating Vulkan Instance");
    VkResult vk_result = CreateVulkanInstance(&data->instance, glfw_extensions, glfw_extensions_count, 0);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn("Failed to create Vulkan instance", vk_result);
    }

    log_debug("Getting Vulkan physical devices and its information");

    vk_result = vkEnumeratePhysicalDevices(data->instance, &data->physical_device_count, nullptr);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn("Failed to get Vulkan physical devices count", vk_result);
    }
    if (data->physical_device_count == 0) {
        PrintAndReturn("No Vulkan physical devices found", 1);
    }

    data->physical_devices = (VkPhysicalDevice *) malloc(sizeof(VkPhysicalDevice) * data->physical_device_count);
    if (!data->physical_devices) {
        PrintAndReturn("Failed to allocate memory for Vulkan physical devices", 1);
    }
    vk_result = vkEnumeratePhysicalDevices(data->instance, &data->physical_device_count, data->physical_devices);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn("Failed to get Vulkan physical devices", vk_result);
    }
    if (data->physical_device_count == 0) {
        PrintAndReturn("No Vulkan physical devices found", 1);
    }
    log_debug("Found %u Vulkan physical devices", data->physical_device_count);

    char *error_str;
    vk_result = GetVulkanPhysicalDevicesInfo(data->instance, data->physical_devices, data->physical_device_count, &data->physical_device_info,
                                             &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    log_info("Finding the most suitable physical device");
    const bool has_physical_device = GetBestPhysicalDeviceWithSwapchainAndPresentationSupport(
        data->physical_device_info, data->physical_device_count, &data->best_physical_device_index, VK_QUEUE_GRAPHICS_BIT);
    if (!has_physical_device) {
        PrintAndReturn("Failed to find a suitable physical device", 1);
    }

    log_info("Suitable physical device found at index: %u: %s", data->best_physical_device_index,
             data->physical_device_info[data->best_physical_device_index].properties.deviceName);

    log_debug("Getting the graphics queue family");
    const bool has_graphic_queue_family = GetQueueFamilyIndexWithRequiredFlags(
        data->physical_device_info[data->best_physical_device_index].queue_family_count,
        data->physical_device_info[data->best_physical_device_index].queue_family_properties,
        VK_QUEUE_GRAPHICS_BIT,
        &data->graphic_queue_family);
    if (!has_graphic_queue_family) {
        PrintAndReturn("Failed to find a suitable graphics queue family", 1);
    }
    log_debug("Found the graphics queue family: %u", data->graphic_queue_family);

    data->present_queue_family = data->physical_device_info[data->best_physical_device_index].queue_family_surface_support_index;
    log_debug("Presentation queue family: %u", data->present_queue_family);

    log_debug("Creating Vulkan device");
    const char *const device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    uint32_t queue_families[2];
    uint32_t queue_family_count = 1;
    queue_families[0] = data->graphic_queue_family;
    if (data->present_queue_family != data->graphic_queue_family) {
        log_debug("Presentation queue family is different from graphics queue family");
        queue_families[1] = data->present_queue_family;
        queue_family_count = 2;
    }

    vk_result = CreateVulkanDevice(data->physical_devices[data->best_physical_device_index], &data->device, queue_families, queue_family_count,
                                   device_extensions, ARRAY_SIZE(device_extensions), &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    log_debug("Getting graphics queue");
    vkGetDeviceQueue(data->device, data->graphic_queue_family, 0, &data->graphics_queue);
    log_debug("Getting presentation queue");
    vkGetDeviceQueue(data->device, data->present_queue_family, 0, &data->present_queue);

    log_trace("Creating pipeline list");
    VulkanPipelineListNew(&data->pipelines);
    log_trace("Creating shader module list");
    VulkanShaderModuleListNew(&data->shader_modules);

    log_info("Preparing built-in shaders");

    constexpr unsigned char vertex_shader_code[] = {
#embed "shader.vert.spv"
    };
    constexpr unsigned char fragment_shader_code[] = {
#embed "shader.frag.spv"
    };
    constexpr size_t vertex_shader_code_size = sizeof(vertex_shader_code);
    constexpr size_t fragment_shader_code_size = sizeof(fragment_shader_code);

    log_debug("Creating Vulkan default shader modules for vertex and fragment shaders");
    log_trace("Creating default vertex shader");
    VulkanShaderModule *vert;
    vk_result = api->CreateShader(api, vertex_shader_code, vertex_shader_code_size, (ShaderData *) &vert);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }
    data->def_vert_shader = vert;
    log_trace("Creating default fragment shader");
    VulkanShaderModule *frag;
    vk_result = api->CreateShader(api, fragment_shader_code, fragment_shader_code_size, (ShaderData *) &frag);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }
    data->def_frag_shader = frag;
    return 0;
}

static void Fini(GraphicsApi *api) {
    BackendData *data = api->backend_data;

    if (data == nullptr) {
        return;
    }

    log_trace("Destroying all window");
    while (data->window_data) {
        api->DestroyWindow(api, data->window_data);
    }

    if (data->device != VK_NULL_HANDLE) {
        log_info("Waiting for device to become idle before cleanup");
        vkDeviceWaitIdle(data->device);
    }

    if (data->device != VK_NULL_HANDLE) {
        VulkanPipelineListDestroy(data->device, data->pipelines);
        VulkanShaderModuleListDestroy(data->device, data->shader_modules);
        vkDestroyDevice(data->device, NULL);
    }

    if (data->physical_devices != nullptr) {
        free((void *) data->physical_devices);
    }
    if (data->physical_device_info != nullptr) {
        free(data->physical_device_info);
    }

    if (data->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(data->instance, NULL);
    }

    free(data);
    api->backend_data = NULL;
}

static uint32_t CreateWindow(GraphicsApi *api, GLFWwindow **window, WindowCreateInfo *create_info, WindowData *window_data) {
    log_info("Creating a window");

    char *error_str;
    BackendData *data = api->backend_data;

    VulkanWindowData *prep_window_data = malloc(sizeof(VulkanWindowData));
    if (prep_window_data == nullptr) {
        PrintAndReturn("Failed to allocated for window data", 1);
    }
    memset(prep_window_data, 0, sizeof(VulkanWindowData));
    *window_data = prep_window_data;

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    *window = glfwCreateWindow(create_info->width, create_info->height, "KISO", NULL, NULL);
    if (*window == nullptr) {
        PrintAndReturn("Failed to create window", 1);
    }
    prep_window_data->window = *window;
    if (data->window_data_tail == nullptr) {
        data->window_data = prep_window_data;
        data->window_data_tail = prep_window_data;
    } else {
        data->window_data_tail->next = prep_window_data;
    }

    int win_size_width, win_size_height;
    glfwGetWindowSize(*window, &win_size_width, &win_size_height);

    log_info("Desired window size: %d x %d", create_info->width, create_info->height);

    if (win_size_width != create_info->width || win_size_height != create_info->height) {
        log_warn("Window size does not match desired size. Desired size: %d x %d Actual size: %d x %d", create_info->width, create_info->height,
                 win_size_width, win_size_height);
        log_warn("Resizing window to desired size");

        log_debug("Resizing window since it does not match the desired size initially");
        glfwSetWindowSize(*window, create_info->width, create_info->height);
        log_info("Window size resized to: %d x %d", create_info->width, create_info->height);

        glfwGetWindowSize(*window, &win_size_width, &win_size_height);

        if (win_size_width != create_info->width || win_size_height != create_info->height) {
            log_error("Window size does not match desired size after resize. Desired size: %d x %d Actual size: %d x %d", create_info->width,
                      create_info->height, win_size_width, win_size_height);
        }
    }

    log_debug("Creating window surface");
    VkResult vk_result = glfwCreateWindowSurface(data->instance, *window, nullptr, &prep_window_data->surface);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn("Failed to create window surface", vk_result);
    }

    log_debug("Getting swapchain support details");
    vk_result = GetSwapchainSupportDetails(data->physical_devices[data->best_physical_device_index], prep_window_data->surface,
                                           &prep_window_data->swapchain_support_details, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    const bool is_preferred_surface_format = ChoosePreferredSurfaceFormat(prep_window_data->swapchain_support_details.formats,
                                                                          prep_window_data->swapchain_support_details.format_count,
                                                                          VK_FORMAT_B8G8R8A8_SRGB,
                                                                          VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                                                                          &prep_window_data->surface_format);
    const bool is_preferred_present_mode = ChoosePreferredPresentMode(prep_window_data->swapchain_support_details.present_modes,
                                                                      prep_window_data->swapchain_support_details.present_mode_count,
                                                                      (VkPresentModeKHR) create_info->present_mode,
                                                                      // present_mode can be converted directly
                                                                      &prep_window_data->present_mode);

    if (!is_preferred_surface_format) {
        log_info("Preferred surface format not found, using first available format");
    }
    if (prep_window_data->surface_format->format == VK_FORMAT_UNDEFINED) {
        PrintAndReturn("Surface format returned undefined", 1);
    }
    if (!is_preferred_present_mode) {
        log_info("Preferred present mode not found, using first available present mode");
    }

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(*window, &framebuffer_width, &framebuffer_height);
    prep_window_data->extent = ChooseVulkanExtent(&prep_window_data->swapchain_support_details.capabilities, framebuffer_width, framebuffer_height);

    const CreateSwapchainInfo create_swapchain_info = {
        .device = data->device,
        .swapchain_support = &prep_window_data->swapchain_support_details,
        .surface_format = prep_window_data->surface_format,
        .present_mode = prep_window_data->present_mode,
        .extent = &prep_window_data->extent,
        .surface = &prep_window_data->surface,
        .image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .graphics_family_index = data->graphic_queue_family,
        .present_family_index = data->present_queue_family,
        .composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
    };

    log_debug("Creating Vulkan swapchain");
    vk_result = CreateVulkanSwapchain(&create_swapchain_info, &prep_window_data->swapchain, &prep_window_data->images, &prep_window_data->image_count,
                                      &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    log_debug("Creating Vulkan image views");
    prep_window_data->image_views = (VkImageView *) malloc(prep_window_data->image_count * sizeof(VkImageView));
    if (!prep_window_data->image_views) {
        PrintAndReturn("Failed to allocate memory for image views", 1);
    }
    vk_result = CreateImageViews(data->device, prep_window_data->images, prep_window_data->image_count, prep_window_data->image_views,
                                 prep_window_data->surface_format->format, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    const uint32_t api_status = api->SetShader(api, prep_window_data, data->def_vert_shader, data->def_frag_shader);
    if (api_status != 0) {
        PrintAndReturn("Cannot set shader", api_status);
    }

    log_debug("Creating command pool");
    vk_result = CreateCommandPool(data->device, data->graphic_queue_family, &prep_window_data->command_pool, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    log_debug("Creating command buffers");
    prep_window_data->command_buffers = (VkCommandBuffer *) malloc(sizeof(VkCommandBuffer) * create_info->max_frame_in_flight);
    vk_result = CreateCommandBuffers(data->device, prep_window_data->command_pool, create_info->max_frame_in_flight,
                                     prep_window_data->command_buffers, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    log_debug("Creating sync objects");
    prep_window_data->image_available_semaphores = (VkSemaphore *) malloc(sizeof(VkSemaphore) * create_info->max_frame_in_flight);
    prep_window_data->render_finished_semaphores = (VkSemaphore *) malloc(sizeof(VkSemaphore) * prep_window_data->image_count);
    prep_window_data->in_flight_fences = (VkFence *) malloc(sizeof(VkFence) * create_info->max_frame_in_flight);
    vk_result = CreateSyncObjects(data->device, &prep_window_data->in_flight_fences, &prep_window_data->image_available_semaphores,
                                  &prep_window_data->render_finished_semaphores, create_info->max_frame_in_flight, prep_window_data->image_count,
                                  &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    glfwSetWindowUserPointer(*window, &prep_window_data->glfw_callback_struct);
    glfwSetFramebufferSizeCallback(*window, (GLFWframebuffersizefun) GlfwCallbackSetFramebufferResized);

    prep_window_data->max_frame_in_flight = create_info->max_frame_in_flight;

    return 0;
}

static void DestroyWindow(GraphicsApi *api, WindowData window_data) {
    BackendData *backend = api->backend_data;
    VulkanWindowData *data = window_data;

    const VkDevice device = backend->device;
    const VkInstance instance = backend->instance;

    log_trace("Destroying a window");
    log_trace("Waiting for device to become idle before cleanup");
    vkDeviceWaitIdle(device);

    for (uint32_t i = 0; i < data->max_frame_in_flight; i++) {
        if (data->image_available_semaphores && data->image_available_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, data->image_available_semaphores[i], nullptr);
        }

        if (data->in_flight_fences && data->in_flight_fences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, data->in_flight_fences[i], nullptr);
        }
    }

    for (uint32_t i = 0; i < data->image_count; i++) {
        if (data->render_finished_semaphores && data->render_finished_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, data->render_finished_semaphores[i], nullptr);
        }
    }

    if (data->command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, data->command_pool, nullptr);
    }

    for (uint32_t i = 0; i < data->image_count; i++) {
        if (data->image_views &&
            data->image_views[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(device, data->image_views[i], nullptr);
        }
    }

    if (data->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, data->swapchain, nullptr);
    }

    if (data->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, data->surface, nullptr);
    }

    free((void *) data->images);
    free((void *) data->image_views);
    free((void *) data->command_buffers);
    free((void *) data->image_available_semaphores);
    free((void *) data->render_finished_semaphores);
    free((void *) data->in_flight_fences);

    if (data->window) {
        glfwDestroyWindow(data->window);
    }

    VulkanWindowData **current = &backend->window_data;

    while (*current && *current != data) {
        current = &(*current)->next;
    }

    if (*current) {
        *current = data->next;
    }

    if (backend->window_data_tail == data) {
        backend->window_data_tail = *current;
    }

    free(data);
}

// TODO: Implement these
static uint32_t BeginFrame(GraphicsApi *api, WindowData window_data) {
    return 0;
}

static uint32_t EndFrame(GraphicsApi *api, WindowData window_data) {
    return 0;
}

static uint32_t PresentFrame(GraphicsApi *api, WindowData window_data) {
    return 0;
}

static uint32_t DrawTestTriangle(GraphicsApi *api, WindowData window_data) {
    const BackendData *backend = api->backend_data;
    VulkanWindowData *data = window_data;

    const SwapchainRecreationInfo swapchain_recreation_info = {
        .swapchain_support = &data->swapchain_support_details,
        .surface_format = data->surface_format,
        .present_mode = data->present_mode,
        .surface = &data->surface,
        .graphics_family_index = backend->graphic_queue_family,
        .present_family_index = backend->present_queue_family,
        .composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .image_count = data->image_count,
        .images = &data->images,
        .window = data->window
    };

    const RenderContext render_context = {
        .device = backend->device,
        .graphics_queue = backend->graphics_queue,
        .present_queue = backend->present_queue,
        .swapchain = &data->swapchain,
        .command_buffers = data->command_buffers,
        .pipeline = data->pipeline->pipeline,
        .in_flight_fences = data->in_flight_fences,
        .image_available_semaphores = data->image_available_semaphores,
        .render_finished_semaphores = data->render_finished_semaphores,
        .extent = &data->extent,
        .current_frame = &data->current_frame,
        .max_frame_in_flight = data->max_frame_in_flight,
        .image_views = &data->image_views,
        .images = &data->images,
        .is_framebuffer_resized = &data->glfw_callback_struct.is_framebuffer_resized
    };

    char *error_str;
    VkResult vk_result = DrawFrame(&render_context, VK_ATTACHMENT_LOAD_OP_CLEAR, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }
    if (data->glfw_callback_struct.is_framebuffer_resized) {
        data->glfw_callback_struct.is_framebuffer_resized = false;
        vk_result = RecreateSwapchain(&render_context, &swapchain_recreation_info, &error_str);
        if (vk_result != VK_SUCCESS) {
            PrintAndReturn(error_str, vk_result);
        }
    }

    return 0;
}

static uint32_t CreateShader(GraphicsApi *api, const unsigned char *code, const size_t code_size, ShaderData *shader_data) {
    char *error_str;
    const BackendData *data = api->backend_data;
    VkShaderModule module;
    const VkResult vk_result = CreateShaderModule(code, code_size, data->device, &module, &error_str);
    if (vk_result != VK_SUCCESS) {
        PrintAndReturn(error_str, vk_result);
    }

    VulkanShaderModule *added;
    if (!VulkanShaderModuleListAdd(data->shader_modules, module, &added)) {
        PrintAndReturn("Cannot add a new shader module", 1);
    }
    *shader_data = added;
    return 0;
}

typedef struct PipelineListFindMatchData {
    uint32_t vert_index;
    uint32_t frag_index;
    VkFormat format;
    VulkanPipeline *match;
} PipelineListFindMatchData;

static void PipelineListFindMatch(VulkanPipeline *pipeline, void *userdata, bool *do_break) {
    PipelineListFindMatchData *data = userdata;
    if (pipeline->vert_index == data->vert_index && pipeline->frag_index == data->frag_index && pipeline->format == data->format) {
        data->match = pipeline;
        *do_break = true;
    }
}

static uint32_t SetShader(GraphicsApi *api, WindowData window_data, ShaderData vertex, ShaderData fragment) {
    const BackendData *data = api->backend_data;

    PipelineListFindMatchData match_data = {
        .vert_index = ((VulkanShaderModule *) vertex)->index,
        .frag_index = ((VulkanShaderModule *) fragment)->index,
        .format = ((VulkanWindowData *) window_data)->surface_format->format,
        .match = nullptr
    };
    VulkanPipelineListEnumerate(data->pipelines, &PipelineListFindMatch, &match_data);

    if (match_data.match) {
        ((VulkanWindowData *) window_data)->pipeline = match_data.match;
        return 0;
    }

    VulkanPipeline pipeline = {
        .vert_index = ((VulkanShaderModule *) vertex)->index,
        .frag_index = ((VulkanShaderModule *) fragment)->index,
        .format = ((VulkanWindowData *) window_data)->surface_format->format
    };
    char *error_str;

    const VulkanShaderModule *vertex_shader = VulkanShaderModuleListGet(data->shader_modules, ((VulkanShaderModule *) vertex)->index);
    if (vertex_shader == nullptr) {
        PrintAndReturn("Vertex shader not exist", 1);
    }
    const VulkanShaderModule *fragment_shader = VulkanShaderModuleListGet(data->shader_modules, ((VulkanShaderModule *) fragment)->index);
    if (fragment_shader == nullptr) {
        PrintAndReturn("Fragment shader not exist", 1);
    }

    const VkResult vk_result = CreateGraphicsPipeline(data->device,
                                                      vertex_shader->shader_module,
                                                      fragment_shader->shader_module,
                                                      &pipeline.pipeline_layout,
                                                      &pipeline.pipeline,
                                                      ((VulkanWindowData *) window_data)->surface_format->format,
                                                      &error_str);
    if (vk_result != VK_SUCCESS) {
        log_fatal("%u", vk_result);
        PrintAndReturn(error_str, vk_result);
    }

    VulkanPipeline *added;
    if (!VulkanPipelineListAdd(data->pipelines, pipeline, &added)) {
        PrintAndReturn("Cannot add a new graphics pipeline", 1);
    }
    ((VulkanWindowData *) window_data)->pipeline = added;

    return 0;
}

GraphicsApi GetVulkanApi() {
    return (GraphicsApi){
        .Init = Init,
        .Fini = Fini,
        .CreateWindow = CreateWindow,
        .DestroyWindow = DestroyWindow,
        .BeginFrame = BeginFrame,
        .EndFrame = EndFrame,
        .PresentFrame = PresentFrame,
        .DrawTestTriangle = DrawTestTriangle,
        .CreateShader = CreateShader,
        .SetShader = SetShader,
        .backend_data = nullptr
    };
}
