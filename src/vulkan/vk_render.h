#ifndef VK_RENDER_H
#define VK_RENDER_H

#include "vk_init.h"
#include "vulkan/vulkan.h"

typedef struct RenderContext {
    const VkDevice device;
    const VkQueue graphics_queue;
    VkSwapchainKHR *swapchain;
    const VkCommandBuffer *command_buffers;
    const VkRenderPass render_pass;
    VkFramebuffer **framebuffers;
    const VkPipeline pipeline;
    VkExtent2D *extent;
    const VkFence *in_flight_fences;
    const VkSemaphore *image_available_semaphores;
    const VkSemaphore *render_finished_semaphores;
    uint32_t *current_frame;
} RenderContext;

typedef struct SwapchainRecreationInfo {
    const SwapchainSupportDetails *swapchain_support;
    const VkSurfaceFormatKHR *surface_format;
    const VkPresentModeKHR present_mode;
    const VkSurfaceKHR *surface;
    const uint32_t graphics_family_index;
    const uint32_t present_family_index;
    const VkCompositeAlphaFlagBitsKHR composite_alpha;
    const uint32_t image_count;
    VkImage **images;
    VkImageView **image_views;
    GLFWwindow *window;
} SwapchainRecreationInfo;

[[nodiscard]]
VkResult RecordCommandBuffer(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffers,
                             VkPipeline pipeline, VkExtent2D extent, char **error_message);

[[nodiscard]]
VkResult RecreateSwapchain(const RenderContext *render_context, const SwapchainRecreationInfo *info, char **error_message);

[[nodiscard]]
VkResult DrawFrame(const RenderContext *render_context, const SwapchainRecreationInfo *swapchain_recreation_info, char **error_message);

#endif // VK_RENDER_H
