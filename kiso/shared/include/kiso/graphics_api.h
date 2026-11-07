// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#ifndef GRAPHICS_API_H
#define GRAPHICS_API_H

#include <GLFW/glfw3.h>

typedef void *WindowData;
typedef void *ShaderData;

typedef enum PresentMode {
    kPresentModeImmediate = 0,
    kPresentModeMailbox = 1,
    kPresentModeFifo = 2,
} PresentMode;

typedef struct GlfwCallbackStruct {
    void *user_pointer;
    bool is_framebuffer_resized;
} GlfwCallbackStruct;

typedef struct WindowCreateInfo {
    int width;
    int height;
    uint32_t max_frame_in_flight;
    PresentMode present_mode;
    void *glfw_callback_usr_ptr;
} WindowCreateInfo;

typedef struct GraphicsApi {
    uint32_t (* const Init)(struct GraphicsApi *api);

    void (* const Fini)(struct GraphicsApi *api);

    uint32_t (* const CreateWindow)(struct GraphicsApi *api, GLFWwindow **window, WindowCreateInfo *create_info, WindowData *window_data);

    void (* const DestroyWindow)(struct GraphicsApi *api, WindowData window_data);

    uint32_t (* const BeginFrame)(struct GraphicsApi *api, WindowData window_data);

    uint32_t (* const EndFrame)(struct GraphicsApi *api, WindowData window_data);

    uint32_t (* const PresentFrame)(struct GraphicsApi *api, WindowData window_data);

    uint32_t (* const DrawTestTriangle)(struct GraphicsApi *api, WindowData window_data); // For testing

    uint32_t (* const CreateShader)(struct GraphicsApi *api, const unsigned char *code, size_t code_size, ShaderData *shader_data);

    uint32_t (* const SetShader)(struct GraphicsApi *api, WindowData window_data, ShaderData vertex, ShaderData fragment);

    void *backend_data;
} GraphicsApi;

#endif // GRAPHICS_API_H
