#ifndef VK_CLEANUP_H
#define VK_CLEANUP_H

#include "vk_init.h"
#include "vulkan/vulkan.h"

void CleanupPhysicalDeviceInformation(PhysicalDeviceInformation *physical_device_information);

void CleanupSwapchainSupportDetails(const SwapchainSupportDetails *swapchain_support_details);

void CleanupImageViews(VkDevice device, const VkImageView *image_views, uint32_t image_count);

#endif // VK_CLEANUP_H
