#include <stdio.h>
#include <GLFW/glfw3.h>
#include "error.h"
#include "logger.h"
#include "vulkan/vk_instance.h"

int main() {
#if defined(NDEBUG)
    log_set_level(DEFAULT, LOG_INFO);
#endif

    log_trace("Initializing GLFW");
    if (!glfwInit()) {
        ExitOnError("Failed to initialize GLFW.", 1);
    }
    glfwSetErrorCallback(GlfwErrorCallback);

    uint32_t glfw_extensions_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
    if (!glfw_extensions) {
        glfwTerminate();
        ExitOnError("Failed to get required Vulkan instance extensions.", 1);
    }
    log_trace("Required Vulkan instance extensions count: %u", glfw_extensions_count);
    for (uint32_t i = 0; i < glfw_extensions_count; ++i) {
        log_trace("Required Vulkan instance extension: %s", glfw_extensions[i]);
    }

    log_trace("Creating GLFW window");
    GLFWwindow *window = glfwCreateWindow(640, 480, "KISO", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        ExitOnError("Failed to create GLFW window.", 1);
    }

    log_trace("Creating Vulkan Instance");
    VkInstance instance;
    const VkResult create_vk_instance_result = CreateVulkanInstance(&instance, glfw_extensions, glfw_extensions_count);
    if (create_vk_instance_result != VK_SUCCESS) {
        ExitOnError("Failed to create Vulkan instance.", create_vk_instance_result);
    }

    vkDestroyInstance(instance, nullptr);
    glfwTerminate();

    return 0;
}