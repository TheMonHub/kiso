#ifndef VK_LIST_H
#define VK_LIST_H

typedef struct VulkanPipeline {
    uint32_t vert_index;
    uint32_t frag_index;
    VkFormat format;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
} VulkanPipeline;

typedef struct VulkanPipelineList {
    uint32_t size;
    uint32_t count;
    struct VulkanPipelineList *next;
    VulkanPipeline *pipelines;
} VulkanPipelineList;

typedef struct VulkanPipelineList {
    uint32_t size;
    uint32_t count;
    struct VulkanPipelineList *next;
    VulkanPipeline *pipelines;
} VulkanPipelineList;

typedef struct VulkanShaderModule {
    uint32_t index;
    VkShaderModule shader_module;
} VulkanShaderModule;

typedef struct VulkanShaderModuleList {
    uint32_t size;
    uint32_t count;
    struct VulkanShaderModuleList *next;
    VulkanShaderModule *shader_modules;
} VulkanShaderModuleList;

typedef void (*VulkanPipelineCallback)(VulkanPipeline *pipeline, void *userdata, bool *do_break);

typedef void (*VulkanShaderModuleCallback)(VulkanShaderModule *module, void *userdata, bool *do_break);

constexpr uint32_t kVulkanPipelineListAllocationChunkSize = 64;
constexpr uint32_t kVulkanShaderModuleListAllocationChunkSize = 32;

static VulkanPipeline *VulkanPipelineListGet(const VulkanPipelineList *list, uint32_t index) {
    while (list) {
        if (index < list->count) {
            return &list->pipelines[index];
        }

        index -= list->count;
        list = list->next;
    }

    return NULL;
}

static bool VulkanPipelineListNew(VulkanPipelineList **list) {
    *list = malloc(sizeof(VulkanPipelineList));
    memset(*list, 0, sizeof(VulkanPipelineList));
    if (*list == nullptr) {
        return false;
    }
    (*list)->pipelines = malloc(kVulkanPipelineListAllocationChunkSize * sizeof(VulkanPipeline));
    if ((*list)->pipelines == nullptr) {
        free(*list);
        return false;
    }
    memset((*list)->pipelines, 0, kVulkanPipelineListAllocationChunkSize * sizeof(VulkanPipeline));
    (*list)->size = kVulkanPipelineListAllocationChunkSize;

    return true;
}

static bool VulkanPipelineListAdd(VulkanPipelineList *list, VulkanPipeline pipeline, VulkanPipeline **added) {
    while (list) {
        if (list->count < list->size) {
            list->pipelines[list->count++] = pipeline;
            *added = &list->pipelines[list->count - 1];
            return true;
        }

        if (!list->next) {
            VulkanPipelineList *new_list;
            if (!VulkanPipelineListNew(&new_list)) {
                return false;
            }
            list->next = new_list;
        }

        list = list->next;
    }

    return false;
}

static void VulkanPipelineListEnumerate(const VulkanPipelineList *list, VulkanPipelineCallback callback, void *userdata) {
    bool do_break = false;
    while (list) {
        for (uint32_t i = 0; i < list->count; i++) {
            callback(&list->pipelines[i], userdata, &do_break);
            if (do_break) {
                return;
            }
        }

        list = list->next;
    }
}

static void VulkanPipelineListDestroy(const VkDevice device, VulkanPipelineList *list) {
    VulkanPipelineList *current = list;

    while (current) {
        VulkanPipelineList *next = current->next;

        for (uint32_t i = 0; i < current->count; i++) {
            const VulkanPipeline *pipeline = &current->pipelines[i];

            if (pipeline->pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, pipeline->pipeline, NULL);
            }

            if (pipeline->pipeline_layout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, pipeline->pipeline_layout, NULL);
            }
        }

        free(current->pipelines);
        free(current);

        current = next;
    }
}

static VulkanShaderModule *VulkanShaderModuleListGet(const VulkanShaderModuleList *list, uint32_t index) {
    while (list) {
        if (index < list->count) {
            return &list->shader_modules[index];
        }

        index -= list->count;
        list = list->next;
    }

    return NULL;
}

static bool VulkanShaderModuleListNew(VulkanShaderModuleList **list) {
    *list = malloc(sizeof(VulkanShaderModuleList));
    memset(*list, 0, sizeof(VulkanShaderModuleList));
    if (*list == nullptr) {
        return false;
    }
    (*list)->shader_modules = malloc(kVulkanShaderModuleListAllocationChunkSize * sizeof(VulkanShaderModule));
    if ((*list)->shader_modules == nullptr) {
        free(*list);
        return false;
    }
    memset((*list)->shader_modules, 0, kVulkanShaderModuleListAllocationChunkSize * sizeof(VulkanShaderModule));
    (*list)->size = kVulkanShaderModuleListAllocationChunkSize;

    return true;
}

static bool VulkanShaderModuleListAdd(VulkanShaderModuleList *list, VkShaderModule shader_module, VulkanShaderModule **added) {
    uint32_t index = 0;
    while (list) {
        index += list->count;
        if (list->count < list->size) {
            const VulkanShaderModule module = {
                .index = index,
                .shader_module = shader_module
            };
            list->shader_modules[list->count++] = module;
            *added = &list->shader_modules[list->count - 1];
            return true;
        }

        if (!list->next) {
            VulkanShaderModuleList *new_list;
            if (!VulkanShaderModuleListNew(&new_list)) {
                return false;
            }
            list->next = new_list;
        }

        list = list->next;
    }

    return false;
}

static void VulkanShaderModuleListEnumerate(const VulkanShaderModuleList *list, VulkanShaderModuleCallback callback, void *userdata) {
    bool do_break = false;
    while (list) {
        for (uint32_t i = 0; i < list->count; i++) {
            callback(&list->shader_modules[i], userdata, &do_break);
            if (do_break) {
                return;
            }
        }

        list = list->next;
    }
}

static void VulkanShaderModuleListDestroy(const VkDevice device, VulkanShaderModuleList *list) {
    VulkanShaderModuleList *current = list;

    while (current) {
        VulkanShaderModuleList *next = current->next;

        for (uint32_t i = 0; i < current->count; i++) {
            const VulkanShaderModule *module = &current->shader_modules[i];

            if (module->shader_module != VK_NULL_HANDLE) {
                vkDestroyShaderModule(device, module->shader_module, NULL);
            }
        }

        free(current->shader_modules);
        free(current);

        current = next;
    }
}

#endif // VK_LIST_H
