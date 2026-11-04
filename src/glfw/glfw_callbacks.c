#include "glfw_callbacks.h"
#include "GLFW/glfw3.h"

void GlfwCallbackSetPointerTrue(GLFWwindow *window) {
    const auto bool_to_set = (bool *)glfwGetWindowUserPointer(window);
    *bool_to_set = true;
}