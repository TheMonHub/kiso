// Copyright 2026 Podchara Phanthakhot
// SPDX-License-Identifier: Apache-2.0

#include "vk_convert.h"

#include <vulkan/vulkan_core.h>

void ConvertQueueFamilyProperties(const VkQueueFamilyProperties *properties, const uint32_t count, QueueFamilyInfo output[]) {
    for (uint32_t i = 0; i < count; i++) {
        output[i].index = i;
        output[i].queue_flags = properties[i].queueFlags;
        output[i].graphics_bit = properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        output[i].compute_bit = properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        output[i].transfer_bit = properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
        output[i].sparse_binding_bit = properties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
        output[i].protected_bit = properties[i].queueFlags & VK_QUEUE_PROTECTED_BIT;
        output[i].video_decode_bit = properties[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR;
        output[i].video_encode_bit = properties[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
        output[i].optical_flow_bit = properties[i].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV;
        output[i].data_graph_bit = properties[i].queueFlags & VK_QUEUE_DATA_GRAPH_BIT_ARM;
    }
}

void ConvertQueueFlags(const VkQueueFlags flags, QueueFamilyInfo *output) {
    output->queue_flags = flags;
    output->graphics_bit = flags & VK_QUEUE_GRAPHICS_BIT;
    output->compute_bit = flags & VK_QUEUE_COMPUTE_BIT;
    output->transfer_bit = flags & VK_QUEUE_TRANSFER_BIT;
    output->sparse_binding_bit = flags & VK_QUEUE_SPARSE_BINDING_BIT;
    output->protected_bit = flags & VK_QUEUE_PROTECTED_BIT;
    output->video_decode_bit = flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR;
    output->video_encode_bit = flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
    output->optical_flow_bit = flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV;
    output->data_graph_bit = flags & VK_QUEUE_DATA_GRAPH_BIT_ARM;
}
