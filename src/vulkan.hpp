#ifndef VULKAN_HPP_
#define VULKAN_HPP_

#include "Arena.h"
#include "window.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

#include <shader.hpp>

#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

const int MAX_FRAMES_IN_FLIGHT = 2; // Should be configurable

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

void record_command_buffer(VulkanContext* ctx, VkCommandBuffer cmd_buffer, uint32_t image_index);
void recreate_swapchain(Arena* arr, VulkanContext* ctx, Window* window);

struct VulkanContext {

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;

    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;

    VkPipeline graphics_pipeline;

    std::vector<VkImage> sc_images; // using vector for easier swapchain recreation (Should be fine as it shouldn't be recreated much)
    std::vector<VkImageView> sc_image_views;
    std::vector<VkFramebuffer> sc_framebuffers;

    VkFormat sc_image_format;
    VkExtent2D sc_extent;

    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buffers[MAX_FRAMES_IN_FLIGHT];

    VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];

    uint32_t current_frame = 0;

    bool frame_buffer_resized = false;

    static VulkanContext* Create(Arena* arr, Window* window)
    {
        VulkanContext* ctx = (VulkanContext*)arena_allocate(arr, sizeof(*ctx));
        memset(ctx, 0, sizeof(*ctx));
        init_vulkan(arr, ctx, window);
        return ctx;
    }
};

#endif // VULKAN_HPP_
