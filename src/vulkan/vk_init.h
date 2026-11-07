#ifndef VK_INIT_H
#define VK_INIT_H

#include "vk_convert.h"

#define CURRENT_VK_API_VERSION VK_API_VERSION_1_3

typedef struct PhysicalDeviceInformation {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceFeatures features;
    QueueFamilyInfo global_queue_family_info;
    uint64_t score;
    VkQueueFamilyProperties *queue_family_properties;
    uint32_t queue_family_count;
    uint32_t queue_family_surface_support_index;
    bool queue_family_surface_support_present;
    bool swapchain_extension_supported;
} PhysicalDeviceInformation;

typedef struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t format_count;
    VkSurfaceFormatKHR *formats;
    uint32_t present_mode_count;
    VkPresentModeKHR *present_modes;
} SwapchainSupportDetails;

typedef struct CreateSwapchainInfo {
    const VkDevice device;
    const SwapchainSupportDetails *swapchain_support;
    const VkSurfaceFormatKHR *surface_format;
    const VkPresentModeKHR present_mode;
    const VkExtent2D *extent;
    const VkSurfaceKHR *surface;
    const VkImageUsageFlags image_usage;
    const uint32_t graphics_family_index;
    const uint32_t present_family_index;
    const VkCompositeAlphaFlagBitsKHR composite_alpha;
} CreateSwapchainInfo;

[[nodiscard]]
VkResult CreateVulkanInstance(VkInstance* instance, const char** extensions, uint32_t extensions_count, VkInstanceCreateFlags flags);

[[nodiscard]]
VkResult GetVulkanPhysicalDevicesInfo(VkInstance instance, const VkPhysicalDevice *physical_device, uint32_t device_count, PhysicalDeviceInformation **info, char **error_message);

void GetVulkanPhysicalDevicesSurfaceSupport(const VkPhysicalDevice *physical_device, uint32_t device_count, PhysicalDeviceInformation *info, VkSurfaceKHR surface);

[[nodiscard]]
VkResult CreateVulkanDevice(VkPhysicalDevice physical_device, VkDevice* vk_device, const uint32_t queue_family_index[], uint32_t queue_family_count, const char * const extensions[], uint32_t extension_count, char** error_message);

[[nodiscard]]
bool GetBestPhysicalDeviceWithSwapchainAndPresentationSupport(const PhysicalDeviceInformation *properties, uint32_t device_count, uint32_t *best_device, VkQueueFlags required_flags) [[unsequenced]];

[[nodiscard]]
bool GetQueueFamilyIndexWithRequiredFlags(uint32_t queue_family_count, const VkQueueFamilyProperties *queue_family_info, VkQueueFlags required_flags, uint32_t *queue_family_index) [[unsequenced]];

bool CheckVulkanPhysicalDeviceExtensionSupport(VkPhysicalDevice device, const char *const *extensions, uint32_t extensions_count);

bool CheckVulkanInstanceLayerSupport(const char *const *extensions, uint32_t extensions_count);

bool CheckVulkanInstanceExtensionSupport(const char *const *extensions, uint32_t extensions_count);

[[nodiscard]]
VkResult GetSwapchainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface, SwapchainSupportDetails *details, char** error_message);

bool ChoosePreferredSurfaceFormat(const VkSurfaceFormatKHR* surface_formats, uint32_t surface_format_count, VkFormat preferred_format, VkColorSpaceKHR preferred_color_space, VkSurfaceFormatKHR **surface_format) [[unsequenced]];

bool ChoosePreferredPresentMode(const VkPresentModeKHR* present_modes, uint32_t present_mode_count, VkPresentModeKHR preferred_present_mode, VkPresentModeKHR *present_mode) [[unsequenced]];

VkExtent2D ChooseVulkanExtent(const VkSurfaceCapabilitiesKHR *capabilities, int width, int height);

[[nodiscard]]
VkResult CreateVulkanSwapchain(const CreateSwapchainInfo *info, VkSwapchainKHR *swapchain, VkImage **images, uint32_t *image_count, char** error_message);

[[nodiscard]]
VkResult CreateImageViews(VkDevice device, const VkImage *images, uint32_t image_count, VkImageView *image_views, VkFormat swapchain_img_format, char** error_message);

[[nodiscard]]
VkResult CreateGraphicsPipeline(VkDevice device, VkShaderModule vertex_shader_module, VkShaderModule fragment_shader_module,
                                VkPipelineLayout *pipeline_layout,
                                VkPipeline *pipeline, VkFormat image_format, char **error_message);

[[nodiscard]]
VkResult CreateCommandPool(VkDevice device, uint32_t queue_family_index, VkCommandPool *command_pool, char **error_message);

[[nodiscard]]
VkResult CreateCommandBuffers(VkDevice device, VkCommandPool command_pool, uint32_t command_buffer_count, VkCommandBuffer *command_buffers, char **error_message);

[[nodiscard]]
VkResult CreateSyncObjects(
    VkDevice device,
    VkFence **in_flight_fence,
    VkSemaphore **image_available_semaphore,
    VkSemaphore **render_finished_semaphores,
    uint32_t command_buffer_count,
    uint32_t image_count,
    char **error_message
);

#endif // VK_INIT_H
