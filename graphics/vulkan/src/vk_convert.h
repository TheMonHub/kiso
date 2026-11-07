// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#ifndef VK_CONVERT_H
#define VK_CONVERT_H

#include <stdint.h>
#include <vulkan/vulkan_core.h>

typedef struct QueueFamilyInfo {
    VkQueueFlags queue_flags;
    uint32_t index;
    bool graphics_bit;
    bool compute_bit;
    bool transfer_bit;
    bool sparse_binding_bit;
    bool protected_bit;
    bool video_decode_bit;
    bool video_encode_bit;
    bool optical_flow_bit;
    bool data_graph_bit;
    bool present_support;
} QueueFamilyInfo;

void ConvertQueueFamilyProperties(const VkQueueFamilyProperties *properties, uint32_t count, QueueFamilyInfo output[]) [[unsequenced]];

void ConvertQueueFlags(const VkQueueFlags flags, QueueFamilyInfo *output) [[unsequenced]];

#endif // VK_CONVERT_H
