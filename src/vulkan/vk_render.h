#ifndef VK_RENDER_H
#define VK_RENDER_H

#include "vulkan/vulkan.h"

[[nodiscard]]
VkResult RecordCommandBuffer(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer,
                             VkPipeline pipeline, const VkExtent2D *extent, char **error_message);

[[nodiscard]]
VkResult DrawFrame(VkDevice device, const VkFence *in_flight_fence, const VkSemaphore *image_available_semaphore, VkSemaphore **render_finished_semaphores,
                   VkSwapchainKHR swapchain, VkCommandBuffer command_buffer, VkRenderPass render_pass, const VkFramebuffer *framebuffers,
                   VkPipeline pipeline, const VkExtent2D *extent, VkQueue graphics_queue, char **error_message);

#endif // VK_RENDER_H
