#ifndef VULKAN_HPP_
#define VULKAN_HPP_

#include "Arena.h"
#include "window.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define VK_CHECK_RESULT(f)                                                                                                                 \
    {                                                                                                                                      \
        VkResult res = (f);                                                                                                                \
        if (res != VK_SUCCESS) {                                                                                                           \
            std::cout << "Fatal : VkResult is \"" << string_VkResult(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
            assert(res == VK_SUCCESS);                                                                                                     \
        }                                                                                                                                  \
    }

struct VulkanContext;

void init_vulkan(Arena* arr, VulkanContext* ctx, Window* window);
void cleanup_vulkan(VulkanContext* ctx, Window* window);

struct VulkanContext {

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;

    std::vector<VkImage> sc_images; // using vector for easier swapchain recreation (Should be fine as it shouldn't be recreated much)
    std::vector<VkImageView> sc_image_views;
    VkFormat sc_image_format;
    VkExtent2D sc_extent;

    static VulkanContext* Create(Arena* arr, Window* window)
    {
        VulkanContext* ctx = (VulkanContext*)arena_allocate(arr, sizeof(*ctx));
        memset(ctx, 0, sizeof(*ctx));
        init_vulkan(arr, ctx, window);
        return ctx;
    }
};

#endif // VULKAN_HPP_
