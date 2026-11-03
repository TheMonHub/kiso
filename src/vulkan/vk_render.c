#include "vk_render.h"

#include "vk_helper.h"

[[nodiscard]]
VkResult RecordCommandBuffer(const VkCommandBuffer command_buffer, const VkRenderPass render_pass, const VkFramebuffer framebuffer,
                             const VkPipeline pipeline, const VkExtent2D *extent, char **error_message) {

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
        RETURN_ERROR("Failed to begin recording command buffer", result);
    }

    const VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = *extent
        },
        .clearValueCount = 1,
        .pClearValues = &(VkClearValue){.color.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)extent->width,
        .height = (float)extent->height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {.x = 0, .y = 0},
        .extent = *extent
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS) {
        RETURN_ERROR("Failed to record command buffer", result);
    }

    return VK_SUCCESS;
}

VkResult DrawFrame(const VkDevice device, const VkFence *in_flight_fence, const VkSemaphore *image_available_semaphore, VkSemaphore **render_finished_semaphores,
                   const VkSwapchainKHR swapchain, const VkCommandBuffer command_buffer, const VkRenderPass render_pass, const VkFramebuffer *framebuffers,
                   const VkPipeline pipeline, const VkExtent2D *extent, const VkQueue graphics_queue, char **error_message) {

    vkWaitForFences(device, 1, in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, in_flight_fence);
    uint32_t image_index;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, *image_available_semaphore, VK_NULL_HANDLE, &image_index);

    vkResetCommandBuffer(command_buffer, 0);
    VkResult vk_result = RecordCommandBuffer(command_buffer, render_pass, framebuffers[image_index], pipeline, extent, error_message);
    if (vk_result != VK_SUCCESS) {
        return vk_result;
    }

    VkSemaphore signal_semaphores[] = {(*render_finished_semaphores)[image_index]};

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = (VkSemaphore[]){*image_available_semaphore},
        .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores
    };

    vk_result = vkQueueSubmit(graphics_queue, 1, &submit_info, *in_flight_fence);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to submit draw command buffer", vk_result);
    }

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };

    vk_result = vkQueuePresentKHR(graphics_queue, &present_info);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to present swapchain image", vk_result);
    }

    return VK_SUCCESS;
}