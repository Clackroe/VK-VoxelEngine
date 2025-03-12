#include "Arena.h"
#include "shader.hpp"
#include <algorithm>
#include <climits>
#include <set>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint>
#include <vulkan.hpp>
#include <vulkan/vulkan_core.h>

// ===========================================
// -----------------CONFIG--------------------
// ===========================================

static const char* DeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static int NumDeviceExtensions = sizeof(DeviceExtensions) / sizeof(const char*);

static const char* ValidationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};
static int NumValidationLayers = sizeof(ValidationLayers) / sizeof(const char*);

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// ===========================================
// ===========================================

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << "\n"
                  << std::endl;
    }

    return VK_FALSE;
}
static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static const char** get_required_instance_extensions(Arena* arr, int* size)
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    *size = glfwExtensionCount;
    if (enableValidationLayers) {
        *size += 1;
    }
    const char** extensions = (const char**)arena_allocate(arr, (*size) * sizeof(const char*));

    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        extensions[i] = glfwExtensions[i];
    }
    if (enableValidationLayers) {
        extensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
    return extensions;
}

static bool check_validation_support()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (int n = 0; n < NumValidationLayers; ++n) {
        bool layer_found = false;
        for (int p = 0; p < layerCount; ++p) {
            if (strcmp(ValidationLayers[n], availableLayers[p].layerName) == 0) {
                layer_found = true;
                break;
            }
        };
        if (!layer_found) {
            return false;
        }
    }

    return true;
}

static void create_instance(Arena* arr, VulkanContext* ctx)
{
    if (!check_validation_support() & enableValidationLayers) {
        fprintf(stderr, "validationLayers not supported");
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "VeryCoolApp";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    ArenaMark m = arena_scratch(arr);

    int num_ext = 0;
    const char** extensions = get_required_instance_extensions(arr, &num_ext);
    instance_info.enabledExtensionCount = num_ext;
    instance_info.ppEnabledExtensionNames = extensions;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
    if (enableValidationLayers) {
        instance_info.enabledLayerCount = NumValidationLayers;
        instance_info.ppEnabledLayerNames = ValidationLayers;

        populateDebugMessengerCreateInfo(debugCreateInfo);
        instance_info.pNext = &debugCreateInfo;
    } else {
        instance_info.enabledLayerCount = 0;
        instance_info.pNext = nullptr;
    }

    VK_CHECK_RESULT(vkCreateInstance(&instance_info, nullptr, &ctx->instance));

    arena_pop_scratch(arr, m);
}

static void setup_debug_messenger(VulkanContext* ctx)
{
    if (!enableValidationLayers)
        return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo {};
    populateDebugMessengerCreateInfo(createInfo);

    VK_CHECK_RESULT(CreateDebugUtilsMessengerEXT(ctx->instance, &createInfo, nullptr, &ctx->debug_messenger));
}

struct QueueFamilyIndices {
    uint32_t graphics = -1;
    uint32_t present = -1;

    bool is_valid()
    {
        return graphics >= 0 && present >= 0;
    }
};

QueueFamilyIndices findQueueFamilies(VulkanContext* ctx, VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, ctx->surface, &presentSupport);

        if (presentSupport) {
            indices.present = i;
        }

        if (indices.is_valid()) {
            break;
        }
    }

    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice dev)
{
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extension_count, nullptr);

    VkExtensionProperties props[extension_count];
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extension_count, props);

    std::set<std::string> requiredExtensions(DeviceExtensions, DeviceExtensions + NumDeviceExtensions);

    for (int i = 0; i < extension_count; ++i) {
        requiredExtensions.erase(props[i].extensionName);
    }

    return requiredExtensions.empty();
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;

    VkSurfaceFormatKHR* formats;
    uint32_t num_formats;
    VkPresentModeKHR* present_modes;
    uint32_t num_present_modes;
};

SwapChainSupportDetails query_swapchain_support(Arena* arr, VulkanContext* ctx, VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, ctx->surface, &details.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, ctx->surface, &details.num_formats, nullptr);
    if (details.num_formats != 0) {
        details.formats = (VkSurfaceFormatKHR*)arena_allocate(arr, details.num_formats * sizeof(*details.formats));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, ctx->surface, &details.num_formats, details.formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, ctx->surface, &details.num_present_modes, nullptr);
    if (details.num_present_modes != 0) {
        details.present_modes = (VkPresentModeKHR*)arena_allocate(arr, details.num_present_modes * sizeof(*details.present_modes));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, ctx->surface, &details.num_present_modes, details.present_modes);
    }

    return details;
}

static bool is_suitable_device(VulkanContext* ctx, VkPhysicalDevice dev)
{
    QueueFamilyIndices indices = findQueueFamilies(ctx, dev);

    bool extensions_supported = checkDeviceExtensionSupport(dev);

    Arena* temp_arena = create_arena(512);
    bool valid_swapchain = false;
    if (extensions_supported) {
        SwapChainSupportDetails swapChainSupport = query_swapchain_support(temp_arena, ctx, dev);
        valid_swapchain = swapChainSupport.num_formats > 0 && swapChainSupport.num_present_modes > 0;
    }

    arena_free(temp_arena);

    return indices.is_valid() && extensions_supported && valid_swapchain;
}

static void pick_physical_device(VulkanContext* ctx)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, nullptr);

    if (device_count == 0) {
        printf("No valid physical devices found");
        return;
    }
    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, devices);

    for (int i = 0; i < device_count; i++) {
        if (is_suitable_device(ctx, devices[i])) {
            ctx->physical_device = devices[i];
            break;
        }
    }

    if (ctx->physical_device == VK_NULL_HANDLE) {
        printf("No valid physical devices found");
        return;
    }
}

static void create_logical_device(VulkanContext* ctx)
{
    QueueFamilyIndices indices = findQueueFamilies(ctx, ctx->physical_device);

    std::set<uint32_t> uniqueQueueFamilies = { indices.graphics, indices.present };
    VkDeviceQueueCreateInfo queue_infos[uniqueQueueFamilies.size()];

    int i = 0;
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queue_infos[i] = queueCreateInfo;
        ++i;
    }

    VkPhysicalDeviceFeatures deviceFeatures {};

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.pQueueCreateInfos = queue_infos;
    createInfo.queueCreateInfoCount = uniqueQueueFamilies.size();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.ppEnabledExtensionNames = DeviceExtensions;
    createInfo.enabledExtensionCount = NumDeviceExtensions;

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = NumValidationLayers;
        createInfo.ppEnabledLayerNames = ValidationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK_RESULT(vkCreateDevice(ctx->physical_device, &createInfo, nullptr, &ctx->device));

    // Queues
    vkGetDeviceQueue(ctx->device, indices.graphics, 0, &ctx->graphics_queue);
    vkGetDeviceQueue(ctx->device, indices.present, 0, &ctx->present_queue);
}

static void create_surface(VulkanContext* ctx, Window* window)
{
    VK_CHECK_RESULT(glfwCreateWindowSurface(ctx->instance, window->window, nullptr, &ctx->surface));
}

static VkSurfaceFormatKHR choose_swapchain_surface_format(VkSurfaceFormatKHR* formats, uint32_t size,
    VkFormat p_format = VK_FORMAT_B8G8R8A8_SRGB, VkColorSpaceKHR p_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
{
    for (int i = 0; i < size; ++i) {
        VkSurfaceFormatKHR format = formats[i];
        if (format.format == p_format && format.colorSpace == p_color_space) {
            return format;
        }
    }
    return formats[0];
}
static VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR* modes, uint32_t size, VkPresentModeKHR p_mode = VK_PRESENT_MODE_FIFO_KHR)
{
    for (int i = 0; i < size; ++i) {
        if (modes[i] == p_mode) {
            return modes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_swap_extent(Window* window, VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window->window, &width, &height);

        VkExtent2D actualExtent = {
            (uint32_t)width,
            (uint32_t)height
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

static void create_swapchain(Arena* arr, VulkanContext* ctx, Window* window)
{

    ArenaMark m = arena_scratch(arr);
    SwapChainSupportDetails support = query_swapchain_support(arr, ctx, ctx->physical_device);

    VkSurfaceFormatKHR surfaceFormat = choose_swapchain_surface_format(support.formats, support.num_formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(support.present_modes, support.num_present_modes);
    VkExtent2D extent = choose_swap_extent(window, support.capabilities);

    uint32_t imageCount = support.capabilities.minImageCount + 1; // ensure that we always have enough to swap buffers

    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = ctx->surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // Maybe use this if you want to render to a texture first (Should be configurable)
    // createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT

    QueueFamilyIndices indices = findQueueFamilies(ctx, ctx->physical_device);
    uint32_t queueFamilyIndices[] = { indices.graphics, indices.present };

    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    vkCreateSwapchainKHR(ctx->device, &createInfo, nullptr, &ctx->swapchain);
    arena_pop_scratch(arr, m);

    // Images

    uint32_t num_images = 0;
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &num_images, nullptr);
    ctx->sc_images.resize(num_images);
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &num_images, ctx->sc_images.data());

    ctx->sc_image_format = surfaceFormat.format;
    ctx->sc_extent = extent;
}
static void create_image_views(VulkanContext* ctx)
{
    ctx->sc_image_views.resize(ctx->sc_images.size());
    for (size_t i = 0; i < ctx->sc_images.size(); i++) {
        VkImageViewCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = ctx->sc_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = ctx->sc_image_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK_RESULT(vkCreateImageView(ctx->device, &createInfo, nullptr, &ctx->sc_image_views[i]));
    }
}

static void create_graphics_pipeline(Arena* arr, VulkanContext* ctx)
{
    VkShaderModule vertex_module = create_shader_module(arr, ctx, "shaders/triangle.vert.spv");
    VkShaderModule frag_module = create_shader_module(arr, ctx, "shaders/triangle.frag.spv");

    VkPipelineShaderStageCreateInfo vert_stage_info {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vertex_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_module;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vert_stage_info, frag_stage_info };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    uint32_t num_dynamic_states = sizeof(dynamic_states) / sizeof(dynamic_states[0]);

    VkPipelineDynamicStateCreateInfo dynamic_state_info {};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = num_dynamic_states;
    dynamic_state_info.pDynamicStates = dynamic_states;

    VkPipelineVertexInputStateCreateInfo v_input_info {};
    v_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    v_input_info.vertexBindingDescriptionCount = 0;
    v_input_info.pVertexBindingDescriptions = nullptr;
    v_input_info.vertexAttributeDescriptionCount = 0;
    v_input_info.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assemply_info {};
    input_assemply_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assemply_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assemply_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)ctx->sc_extent.width;
    viewport.height = (float)ctx->sc_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = ctx->sc_extent;

    VkPipelineViewportStateCreateInfo viewport_state {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    VK_CHECK_RESULT(vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, nullptr, &ctx->pipeline_layout));

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &v_input_info;
    pipelineInfo.pInputAssemblyState = &input_assemply_info;
    pipelineInfo.pViewportState = &viewport_state;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamic_state_info;

    pipelineInfo.layout = ctx->pipeline_layout;
    pipelineInfo.renderPass = ctx->render_pass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx->graphics_pipeline));

    vkDestroyShaderModule(ctx->device, frag_module, nullptr);
    vkDestroyShaderModule(ctx->device, vertex_module, nullptr);
}

static void create_renderpass(VulkanContext* ctx)
{
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = ctx->sc_image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VK_CHECK_RESULT(vkCreateRenderPass(ctx->device, &renderPassInfo, nullptr, &ctx->render_pass));
}

static void create_framebuffers(VulkanContext* ctx)
{
    ctx->sc_framebuffers.resize(ctx->sc_image_views.size());
    for (size_t i = 0; i < ctx->sc_image_views.size(); i++) {
        VkImageView attachments[] = {
            ctx->sc_image_views[i]
        };

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = ctx->render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = ctx->sc_extent.width;
        framebufferInfo.height = ctx->sc_extent.height;
        framebufferInfo.layers = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(ctx->device, &framebufferInfo, nullptr, &ctx->sc_framebuffers[i]))
    }
}
static void create_command_pool(VulkanContext* ctx)
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(ctx, ctx->physical_device);

    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
    VK_CHECK_RESULT(vkCreateCommandPool(ctx->device, &poolInfo, nullptr, &ctx->cmd_pool));
}

static void create_command_buffers(VulkanContext* ctx)
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ctx->cmd_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(ctx->device, &allocInfo, ctx->cmd_buffers))
}

void record_command_buffer(VulkanContext* ctx, VkCommandBuffer cmd_buffer, uint32_t image_index)
{
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmd_buffer, &beginInfo));

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = ctx->render_pass;
    renderPassInfo.framebuffer = ctx->sc_framebuffers[image_index];

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = ctx->sc_extent;

    VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphics_pipeline);

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)ctx->sc_extent.width;
    viewport.height = (float)ctx->sc_extent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = ctx->sc_extent;
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

    vkCmdDraw(cmd_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd_buffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmd_buffer));
}

static void create_sync_objects(VulkanContext* ctx)
{
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VK_CHECK_RESULT(vkCreateSemaphore(ctx->device, &semaphoreInfo, nullptr, &ctx->image_available_semaphores[i]))
        VK_CHECK_RESULT(vkCreateSemaphore(ctx->device, &semaphoreInfo, nullptr, &ctx->render_finished_semaphores[i]))
        VK_CHECK_RESULT(vkCreateFence(ctx->device, &fenceInfo, nullptr, &ctx->in_flight_fences[i]))
    }
}

static void cleanup_swapchain(VulkanContext* ctx)
{
    for (size_t i = 0; i < ctx->sc_framebuffers.size(); i++) {
        vkDestroyFramebuffer(ctx->device, ctx->sc_framebuffers[i], nullptr);
    }

    for (size_t i = 0; i < ctx->sc_image_views.size(); i++) {
        vkDestroyImageView(ctx->device, ctx->sc_image_views[i], nullptr);
    }
    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, nullptr);
}

void recreate_swapchain(Arena* arr, VulkanContext* ctx, Window* window)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window->window, &width, &height);
    while (width == 0 || height == 0) {
        printf("WAITING\n");
        glfwGetFramebufferSize(window->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(ctx->device);

    cleanup_swapchain(ctx);

    create_swapchain(arr, ctx, window);
    create_image_views(ctx);
    create_framebuffers(ctx);
}

void init_vulkan(Arena* arr, VulkanContext* ctx, Window* window)
{

    glfwSetWindowUserPointer(window->window, ctx);

    create_instance(arr, ctx);
    setup_debug_messenger(ctx);
    create_surface(ctx, window);
    pick_physical_device(ctx);
    create_logical_device(ctx);
    create_swapchain(arr, ctx, window);
    create_image_views(ctx);
    create_renderpass(ctx);
    create_graphics_pipeline(arr, ctx);
    create_framebuffers(ctx);
    create_command_pool(ctx);
    create_command_buffers(ctx);
    create_sync_objects(ctx);
}

void cleanup_vulkan(VulkanContext* ctx, Window* window)
{
    cleanup_swapchain(ctx);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        vkDestroySemaphore(ctx->device, ctx->image_available_semaphores[i], nullptr);
        vkDestroySemaphore(ctx->device, ctx->render_finished_semaphores[i], nullptr);
        vkDestroyFence(ctx->device, ctx->in_flight_fences[i], nullptr);
    }
    vkDestroyCommandPool(ctx->device, ctx->cmd_pool, nullptr);

    vkDestroyPipeline(ctx->device, ctx->graphics_pipeline, nullptr);

    vkDestroyPipelineLayout(ctx->device, ctx->pipeline_layout, nullptr);
    vkDestroyRenderPass(ctx->device, ctx->render_pass, nullptr);

    vkDestroySurfaceKHR(ctx->instance, ctx->surface, nullptr);
    vkDestroyDevice(ctx->device, nullptr);
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(ctx->instance, ctx->debug_messenger, nullptr);
    }
    vkDestroyInstance(ctx->instance, nullptr);

    ctx->sc_images.clear(); // explicitely free the memory, since the vector destructor will not be called
    ctx->sc_image_views.clear();

    window->destroy();
}
