#include "vk_render.h"

#include <assert.h>
#include <stdlib.h>

#include "logger.h"
#include "vk_cleanup.h"
#include "vk_helper.h"
#include "vk_init.h"

[[nodiscard]]
VkResult RecordCommandBuffer(const VkCommandBuffer command_buffer, const VkRenderPass render_pass, const VkFramebuffer framebuffers,
                             const VkPipeline pipeline, const VkExtent2D extent, char **error_message) {

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
        .framebuffer = framebuffers,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = extent
        },
        .clearValueCount = 1,
        .pClearValues = &(VkClearValue){.color.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)extent.width,
        .height = (float)extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {.x = 0, .y = 0},
        .extent = extent
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

VkResult RecreateSwapchain(const RenderContext *render_context, const SwapchainRecreationInfo *info, char **error_message) {
    log_trace("Instructed to recreate swapchain");
    int width = 0, height = 0;
    glfwGetFramebufferSize(info->window, &width, &height);
    while (width == 0 || height == 0) {
        log_trace("There is nothing to render! Waiting for a bigger framebuffer");
        glfwGetFramebufferSize(info->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(render_context->device);

    vkDestroySwapchainKHR(render_context->device, *render_context->swapchain, nullptr);
    CleanupImageViews(render_context->device, *info->image_views, info->image_count);
    CleanupFramebuffers(render_context->device, *render_context->framebuffers, info->image_count);

    const VkExtent2D extent_temp = ChooseVulkanExtent(&info->swapchain_support->capabilities, width, height);
    render_context->extent->width = extent_temp.width;
    render_context->extent->height = extent_temp.height;

    const CreateSwapchainInfo create_swapchain_info = {
        .device = render_context->device,
        .swapchain_support = info->swapchain_support,
        .surface_format = info->surface_format,
        .present_mode = info->present_mode,
        .extent = render_context->extent,
        .surface = info->surface,
        .image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .graphics_family_index = info->graphics_family_index,
        .present_family_index = info->present_family_index,
        .composite_alpha = info->composite_alpha
    };

    uint32_t new_image_count;

    VkResult vk_result = CreateVulkanSwapchain(&create_swapchain_info, render_context->swapchain, info->images, &new_image_count, error_message);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to recreate swapchain", vk_result);
    }

    vk_result = CreateImageViews(render_context->device, *info->images, new_image_count, *info->image_views, info->surface_format->format, error_message);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to create image views for new swapchain", vk_result);
    }

    vk_result = CreateFramebuffers(render_context->device, *info->image_views, new_image_count, render_context->render_pass, *render_context->framebuffers, render_context->extent, error_message);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to create framebuffers for new swapchain", vk_result);
    }

    return VK_SUCCESS;
}

VkResult DrawFrame(const RenderContext *render_context, const SwapchainRecreationInfo *swapchain_recreation_info, char **error_message) {
    VkResult vk_result = vkWaitForFences(render_context->device, 1, &render_context->in_flight_fences[*render_context->current_frame], VK_TRUE, UINT64_MAX);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to wait for in-flight fence", vk_result);
    }

    uint32_t image_index;
    vk_result = vkAcquireNextImageKHR(render_context->device, *render_context->swapchain, UINT64_MAX, render_context->image_available_semaphores[*render_context->current_frame], VK_NULL_HANDLE, &image_index);
    if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
        log_trace("Swapchain is out of date");
        char *error_result = nullptr;
        vk_result = RecreateSwapchain(render_context, swapchain_recreation_info, &error_result);
        if (vk_result != VK_SUCCESS) {
            log_error("Failed to recreate swapchain after out-of-date error");
            RETURN_ERROR(error_result, vk_result);
        }
    }
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to acquire swapchain image", vk_result);
    }

    vk_result = vkResetFences(render_context->device, 1, &render_context->in_flight_fences[*render_context->current_frame]);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to reset in-flight fence", vk_result);
    }

    vkResetCommandBuffer(render_context->command_buffers[*render_context->current_frame], 0);
    vk_result = RecordCommandBuffer(render_context->command_buffers[*render_context->current_frame], render_context->render_pass, (*render_context->framebuffers)[image_index], render_context->pipeline, *render_context->extent, error_message);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to record command buffer", vk_result);
    }

    VkSemaphore signal_semaphores[] = {render_context->render_finished_semaphores[image_index]};

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = (VkSemaphore[]){render_context->image_available_semaphores[*render_context->current_frame]},
        .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = &render_context->command_buffers[*render_context->current_frame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores
    };

    vk_result = vkQueueSubmit(render_context->graphics_queue, 1, &submit_info, render_context->in_flight_fences[*render_context->current_frame]);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to submit draw command buffer", vk_result);
    }

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = render_context->swapchain,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };

    vk_result = vkQueuePresentKHR(render_context->graphics_queue, &present_info);

    if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
        log_trace("Swapchain is out of date");
        char *error_result = nullptr;
        vk_result = RecreateSwapchain(render_context, swapchain_recreation_info, &error_result);
        if (vk_result != VK_SUCCESS) {
            log_error("Failed to recreate swapchain after out-of-date error or framebuffer resize");
            RETURN_ERROR(error_result, vk_result);
        }
    }
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to present swapchain image", vk_result);
    }

    *render_context->current_frame = (*render_context->current_frame + 1) % kMaxFramesInFlight;

    return VK_SUCCESS;
}