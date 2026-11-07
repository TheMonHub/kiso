// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <string.h>
#include "kiso/error.h"
#include "logger.h"
#include "kiso/version.h"
#include "kiso/common.h"
#include "kiso/vk_api.h"

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

    for (int i = 1; i < argc; i++) {
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

    GraphicsApi graphics_api = GetVulkanApi();
    uint32_t graphics_api_status = graphics_api.Init(&graphics_api);
    if (graphics_api_status != 0) {
        PrintSetAndGoto("Cannot initialize graphics api", graphics_api_status, error_goto);
    }

    WindowCreateInfo window_create_info = {
        .width = desired_window_width,
        .height = desired_window_height,
        .max_frame_in_flight = kMaxFramesInFlight,
        .present_mode = kPresentModeMailbox,
        .glfw_callback_usr_ptr = nullptr
    };

    WindowData window_data;
    GLFWwindow *window;
    graphics_api_status = graphics_api.CreateWindow(&graphics_api, &window, &window_create_info, &window_data);
    if (graphics_api_status != 0) {
        PrintSetAndGoto("Cannot create a window", graphics_api_status, error_goto);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        graphics_api.DrawTestTriangle(&graphics_api, window_data);
    }

error_goto:
    graphics_api.Fini(&graphics_api);
    glfwTerminate();
    return _current_error_status;
}
