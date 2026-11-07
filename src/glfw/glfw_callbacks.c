// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include "glfw_callbacks.h"
#include <GLFW/glfw3.h>

#include "graphics_api.h"

void GlfwCallbackSetFramebufferResized(GLFWwindow *window) {
    const auto bool_to_set = (GlfwCallbackStruct *) glfwGetWindowUserPointer(window);
    bool_to_set->is_framebuffer_resized = true;
}
