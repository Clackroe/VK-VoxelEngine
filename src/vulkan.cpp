#include "Arena.h"
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

static void create_graphics_pipeline()
{
}

void init_vulkan(Arena* arr, VulkanContext* ctx, Window* window)
{
    create_instance(arr, ctx);
    setup_debug_messenger(ctx);
    create_surface(ctx, window);
    pick_physical_device(ctx);
    create_logical_device(ctx);
    create_swapchain(arr, ctx, window);
    create_image_views(ctx);
}

void cleanup_vulkan(VulkanContext* ctx, Window* window)
{
    for (auto v : ctx->sc_image_views) {
        vkDestroyImageView(ctx->device, v, nullptr);
    }

    vkDestroySwapchainKHR(ctx->device, ctx->swapchain, nullptr);

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
