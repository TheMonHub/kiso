// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#ifndef ENUM_TO_STR_H
#define ENUM_TO_STR_H

#include <vulkan/vulkan_core.h>

const char *VulkanDeviceTypeToStr(VkPhysicalDeviceType value) [[unsequenced]];

const char *VkPresentModeToStr(VkPresentModeKHR value) [[unsequenced]];

#endif // ENUM_TO_STR_H
