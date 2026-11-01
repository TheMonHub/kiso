#include <stdio.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "error.h"
#include "logger.h"
#include "version.h"
#include "GLFW/glfw3.h"
#include "util/common_macro.h"
#include "vulkan/vk_init.h"

// For some reason the compiler can't see glfwCreateWindowSurface unless I declare it here.
GLFWAPI VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);

static void PrintProjectInfo() {
    log_info("========== KISO ===========");
    log_info("Version: " PROJECT_VERSION_STRING);
    log_info("Version number: %d", kProjectVersionNumber);
#if defined(PROJECT_GITHUB_URL)
    log_info("GitHub source code URL: " PROJECT_GITHUB_URL);
#endif
#if defined(PROJECT_GIT_COMMIT_HASH)
#if defined(PROJECT_GITHUB_URL)
    log_info("GitHub commit URL: " PROJECT_GITHUB_URL "/commit/" PROJECT_GIT_COMMIT_HASH);
#endif
    log_info("Git commit hash: " PROJECT_GIT_COMMIT_HASH);
#endif
#if defined(PROJECT_GIT_BRANCH)
    log_info("Git branch: " PROJECT_GIT_BRANCH);
#endif
    log_info(LINE_SEPARATOR);
}

int main() {
    log_set_max_level(DEFAULT, LOG_WARN);
    log_add_stream_handler(stderr, LOG_WARN, LOG_LEVEL_MAX, "stderr");

    PrintProjectInfo();

    log_info("Initializing Application");

    log_trace("Initializing GLFW");
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
    log_trace("Required Vulkan instance extensions count: %u", glfw_extensions_count);
    for (uint32_t i = 0; i < glfw_extensions_count; ++i) {
        log_trace("Required Vulkan instance extension: %s", glfw_extensions[i]);
    }

    const char *instance_extensions[glfw_extensions_count + 1];
    for (uint32_t i = 0; i < glfw_extensions_count; ++i) {
        instance_extensions[i] = glfw_extensions[i];
    }
    instance_extensions[glfw_extensions_count] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;

    log_info("Initializing Vulkan");
    log_trace("Creating Vulkan Instance");
    VkInstance instance;
    VkResult vk_result = CreateVulkanInstance(&instance, instance_extensions, glfw_extensions_count + 1, VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR);
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

    log_trace("Getting Vulkan physical devices and its information");
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
        ExitOnError("No Vulkan physical devices found", VK_ERROR_INCOMPATIBLE_DRIVER);
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
        ExitOnError("No Vulkan physical devices found", VK_ERROR_INCOMPATIBLE_DRIVER);
    }
    log_trace("Found %u Vulkan physical devices", physical_device_count);

    PhysicalDeviceInformation *physical_device_info;
    char *error_str;
    vk_result = GetVulkanPhysicalDevicesInfo(instance, physical_devices, physical_device_count, &physical_device_info, &error_str);
    if (vk_result != VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }

    log_trace("Finding the most suitable physical device");
    uint32_t best_physical_device_index;
    const bool has_physical_device = GetBestPhysicalDeviceWithSwapchainAndPresentationSupport(physical_device_info, physical_device_count, &best_physical_device_index, VK_QUEUE_GRAPHICS_BIT);
    if (!has_physical_device) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError("Failed to find a suitable physical device", 1);
    }

    log_trace("Found the most suitable physical device: %u", best_physical_device_index);

    log_trace("Getting the graphics queue family");
    uint32_t graphic_queue_family;
    const bool has_graphic_queue_family = GetQueueFamilyIndexWithRequiredFlags(
        physical_device_info[best_physical_device_index].queue_family_count,
        physical_device_info[best_physical_device_index].queue_family_properties,
        VK_QUEUE_GRAPHICS_BIT,
        &graphic_queue_family);
    if (!has_graphic_queue_family) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError("Failed to find a suitable graphics queue family", 1);
    }
    log_trace("Found the graphics queue family: %u", graphic_queue_family);

    const uint32_t presentation_queue_family = physical_device_info[best_physical_device_index].queue_family_surface_support_index;
    log_trace("Presentation queue family: %u", presentation_queue_family);

    log_trace("Creating Vulkan device");
    VkDevice device;
    const char * const device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    uint32_t queue_families[2];
    uint32_t queue_family_count = 1;
    queue_families[0] = graphic_queue_family;
    if (presentation_queue_family != graphic_queue_family) {
        log_trace("Presentation queue family is different from graphics queue family");
        queue_families[1] = presentation_queue_family;
        queue_family_count = 2;
    }

    vk_result = CreateVulkanDevice(physical_devices[best_physical_device_index], &device, queue_families, queue_family_count, device_extensions, ARRAY_SIZE(device_extensions), &error_str);
    if (vk_result != VK_SUCCESS) {
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
        ExitOnError(error_str, vk_result);
    }

    VkSwapchainKHR swapchain;
    

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    log_info("Clearing resources");
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();

    return 0;
}
