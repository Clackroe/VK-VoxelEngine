#ifndef VE_SHADER
#define VE_SHADER

#include <Arena.h>
#include <vulkan/vulkan.h>

struct VulkanContext;

VkShaderModule create_shader_module(Arena* arr, VulkanContext* ctx, const char* path);

#endif // VE_SHADER
