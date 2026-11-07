#ifndef GRAPHICS_API_H
#define GRAPHICS_API_H

#include <GLFW/glfw3.h>

typedef void* WindowData;
typedef void* ShaderData;

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
    uint32_t (*Init)(struct GraphicsApi* api);
    void (*Fini)(struct GraphicsApi* api);

    uint32_t (*CreateWindow)(struct GraphicsApi* api, GLFWwindow **window, WindowCreateInfo *create_info, WindowData *window_data);
    void (*DestroyWindow)(struct GraphicsApi* api, WindowData window_data);

    uint32_t (*BeginFrame)(struct GraphicsApi* api, WindowData window_data);
    uint32_t (*EndFrame)(struct GraphicsApi* api, WindowData window_data);
    uint32_t (*PresentFrame)(struct GraphicsApi* api, WindowData window_data);

    uint32_t (*DrawTestTriangle)(struct GraphicsApi* api, WindowData window_data); // For testing

    uint32_t (*CreateShader)(struct GraphicsApi* api, const unsigned char *code, size_t code_size, ShaderData *shader_data);
    uint32_t (*SetShader)(struct GraphicsApi* api, WindowData window_data, ShaderData vertex, ShaderData fragment);

    void* backend_data;
} GraphicsApi;

#endif // GRAPHICS_API_H
