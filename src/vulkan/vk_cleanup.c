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