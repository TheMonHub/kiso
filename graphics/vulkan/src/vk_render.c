// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include "vk_render.h"

#include <assert.h>

#include "logger.h"
#include "vk_cleanup.h"
#include "vk_helper.h"
#include "vk_init.h"

static void TransitionImageLayout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask = VK_ACCESS_2_NONE,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }

    const VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

[[nodiscard]]
VkResult RecordCommandBuffer(VkCommandBuffer command_buffer, VkImage image, VkImageView image_view, VkAttachmentLoadOp load_op,
                             VkPipeline pipeline, VkExtent2D extent, char **error_message) {
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

    TransitionImageLayout(command_buffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo color_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = nullptr,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = load_op,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = (VkClearValue){.color.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}
    };

    const VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = extent
        },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr
    };

    vkCmdBeginRendering(command_buffer, &rendering_info);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) extent.width,
        .height = (float) extent.height,
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
    vkCmdEndRendering(command_buffer);

    TransitionImageLayout(command_buffer, image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS) {
        RETURN_ERROR("Failed to record command buffer", result);
    }

    return VK_SUCCESS;
}

VkResult RecreateSwapchain(const RenderContext *render_context, const SwapchainRecreationInfo *info, char **error_message) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(info->window, &width, &height);
    while (width == 0 || height == 0) {
        log_trace("There is nothing to render! Waiting for a bigger framebuffer");
        glfwGetFramebufferSize(info->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(render_context->device);

    vkDestroySwapchainKHR(render_context->device, *render_context->swapchain, nullptr);
    CleanupImageViews(render_context->device, *render_context->image_views, info->image_count);

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

    vk_result = CreateImageViews(render_context->device, *info->images, new_image_count, *render_context->image_views, info->surface_format->format,
                                 error_message);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to create image views for new swapchain", vk_result);
    }

    return VK_SUCCESS;
}

VkResult DrawFrame(const RenderContext *render_context, const VkAttachmentLoadOp load_op, char **error_message) {
    VkResult vk_result = vkWaitForFences(render_context->device, 1, &render_context->in_flight_fences[*render_context->current_frame], VK_TRUE,
                                         UINT64_MAX);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to wait for in-flight fence", vk_result);
    }

    uint32_t image_index;
    vk_result = vkAcquireNextImageKHR(render_context->device, *render_context->swapchain, UINT64_MAX,
                                      render_context->image_available_semaphores[*render_context->current_frame], VK_NULL_HANDLE, &image_index);
    if (vk_result == VK_ERROR_OUT_OF_DATE_KHR) {
        log_trace("Swapchain is out of date");
        *render_context->is_framebuffer_resized = true;
        return VK_SUCCESS;
    }
    if (vk_result == VK_SUBOPTIMAL_KHR) {
        log_trace("Swapchain is suboptimal");
        *render_context->is_framebuffer_resized = true;
    } else if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to acquire swapchain image", vk_result);
    }

    vk_result = vkResetFences(render_context->device, 1, &render_context->in_flight_fences[*render_context->current_frame]);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to reset in-flight fence", vk_result);
    }

    vk_result = vkResetCommandBuffer(render_context->command_buffers[*render_context->current_frame], 0);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to reset command buffer", vk_result);
    }
    vk_result = RecordCommandBuffer(render_context->command_buffers[*render_context->current_frame], (*render_context->images)[image_index],
                                    (*render_context->image_views)[image_index], load_op, render_context->pipeline, *render_context->extent,
                                    error_message);
    if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to record command buffer", vk_result);
    }

    VkSemaphore signal_semaphores[] = {render_context->render_finished_semaphores[image_index]};

    const VkSemaphoreSubmitInfo wait_semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = render_context->image_available_semaphores[*render_context->current_frame],
        .value = 0,
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .deviceIndex = 0
    };

    const VkCommandBufferSubmitInfo command_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = render_context->command_buffers[*render_context->current_frame],
        .deviceMask = 0
    };

    const VkSemaphoreSubmitInfo signal_semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = render_context->render_finished_semaphores[image_index],
        .value = 0,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0
    };

    const VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &wait_semaphore_info,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &command_buffer_info,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signal_semaphore_info
    };

    vk_result = vkQueueSubmit2(render_context->graphics_queue, 1, &submit_info, render_context->in_flight_fences[*render_context->current_frame]);
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

    vk_result = vkQueuePresentKHR(render_context->present_queue, &present_info);

    if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
        log_trace("Swapchain is out of date or suboptimal");
        *render_context->is_framebuffer_resized = true;
    } else if (vk_result != VK_SUCCESS) {
        RETURN_ERROR("Failed to present swapchain image", vk_result);
    }

    *render_context->current_frame = (*render_context->current_frame + 1) % render_context->max_frame_in_flight;

    return VK_SUCCESS;
}
