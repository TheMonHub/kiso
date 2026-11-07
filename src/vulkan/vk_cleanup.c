// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include "vk_cleanup.h"

#include <stdlib.h>

void CleanupPhysicalDeviceInformation(PhysicalDeviceInformation *physical_device_information) {
    free(physical_device_information->queue_family_properties);
    free(physical_device_information);
}

void CleanupSwapchainSupportDetails(const SwapchainSupportDetails *swapchain_support_details) {
    free(swapchain_support_details->formats);
    free(swapchain_support_details->present_modes);
}

void CleanupImageViews(const VkDevice device, const VkImageView *image_views, const uint32_t image_count) {
    for (uint32_t i = 0; i < image_count; ++i) {
        vkDestroyImageView(device, image_views[i], nullptr);
    }
}

void CleanupImages(const VkDevice device, const VkImage *images, const uint32_t image_count) {
    for (uint32_t i = 0; i < image_count; ++i) {
        vkDestroyImage(device, images[i], nullptr);
    }
}

void CleanupFramebuffers(const VkDevice device, const VkFramebuffer *framebuffers, const uint32_t framebuffer_count) {
    for (uint32_t i = 0; i < framebuffer_count; ++i) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
    }
}

void CleanupSemaphores(const VkDevice device, const VkSemaphore *semaphores, const uint32_t semaphore_count) {
    for (uint32_t i = 0; i < semaphore_count; ++i) {
        vkDestroySemaphore(device, semaphores[i], nullptr);
    }
}

void CleanupFences(const VkDevice device, const VkFence *fences, const uint32_t fence_count) {
    for (uint32_t i = 0; i < fence_count; ++i) {
        vkDestroyFence(device, fences[i], nullptr);
    }
}
