#include "Arena.h"
#include "vulkan.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vulkan/vulkan_core.h>

static char* read_file(Arena* arr, const char* path, size_t* size)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        printf("Failed to open file: %s\n", path);
        return nullptr;
    }
    *size = file.tellg();

    char* out = (char*)arena_allocate(arr, *size * sizeof(*out));

    file.seekg(0);
    file.read(out, *size);
    file.close();

    return out;
}

VkShaderModule create_shader_module(Arena* arr, VulkanContext* ctx, const char* path)
{
    size_t size = 0;
    ArenaMark m = arena_scratch(arr);
    char* binary = read_file(arr, path, &size);

    VkShaderModuleCreateInfo c_info;
    c_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    c_info.codeSize = size;
    c_info.pCode = (uint32_t*)binary;
    c_info.flags = 0;
    c_info.pNext = nullptr;

    VkShaderModule module;
    VK_CHECK_RESULT(vkCreateShaderModule(ctx->device, &c_info, nullptr, &module));

    arena_pop_scratch(arr, m);
    return module;
}
