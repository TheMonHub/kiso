#ifndef VK_CLEANUP_H
#define VK_CLEANUP_H

#include "vk_init.h"
#include "vulkan/vulkan.h"

void CleanupPhysicalDeviceInformation(PhysicalDeviceInformation *physical_device_information);

void CleanupSwapchainSupportDetails(const SwapchainSupportDetails *swapchain_support_details);

void CleanupImageViews(VkDevice device, const VkImageView *image_views, uint32_t image_count);

void CleanupImages(VkDevice device, const VkImage *images, uint32_t image_count);

void CleanupFramebuffers(VkDevice device, const VkFramebuffer *framebuffers, uint32_t framebuffer_count);

void CleanupSemaphores(VkDevice device, const VkSemaphore *semaphores, uint32_t semaphore_count);

void CleanupFences(VkDevice device, const VkFence *fences, uint32_t fence_count);

#endif // VK_CLEANUP_H
