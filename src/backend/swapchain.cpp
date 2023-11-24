#include "swapchain.hpp"
namespace ff
{
    void Swapchain::create_surface()
    {
        VkWin32SurfaceCreateInfoKHR const surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .hinstance = GetModuleHandleA(nullptr),
            .hwnd = static_cast<HWND>(window_handle)
        };
        CHECK_VK_RESULT(vkCreateWin32SurfaceKHR(instance->vulkan_instance, &surface_create_info, nullptr, &surface));
        BACKEND_LOG("[INFO][Swaphcain::create_surface()] Surface creation successful")
    }

    void Swapchain::resize()
    {
        device->wait_idle();
        for(ImageId const & id : images)
        {
            device->destroy_swapchain_image(id);
        }
        VkSurfaceCapabilitiesKHR surface_capabilities;
        CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->vulkan_physical_device, surface, &surface_capabilities));
        surface_extent = {
            .width = surface_capabilities.currentExtent.width,
            .height = surface_capabilities.currentExtent.height
        };

        VkImageUsageFlags const usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        auto old_swapchain = swapchain;
        VkSwapchainCreateInfoKHR const swapchain_create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = surface,
            .minImageCount = MIN_IMAGE_COUNT,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = surface_extent,
            .imageArrayLayers = 1,
            .imageUsage = usage,
            .imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = reinterpret_cast<u32*>(&device->main_queue_family_index),
            .preTransform = VkSurfaceTransformFlagBitsKHR::VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = old_swapchain,
        };

        CHECK_VK_RESULT(vkCreateSwapchainKHR(device->vulkan_device, &swapchain_create_info, nullptr, &swapchain));
        BACKEND_LOG("[INFO][Swapchain::Swapchain()] Swapchain creation successful");

        vkDestroySwapchainKHR(device->vulkan_device, old_swapchain, nullptr);

        u32 vulkan_swapchain_image_count = 0;
        std::vector<VkImage> vulkan_swapchain_images = {};
        CHECK_VK_RESULT(vkGetSwapchainImagesKHR(device->vulkan_device, swapchain, &vulkan_swapchain_image_count, nullptr));
        vulkan_swapchain_images.resize(vulkan_swapchain_image_count);
        CHECK_VK_RESULT(vkGetSwapchainImagesKHR(device->vulkan_device, swapchain, &vulkan_swapchain_image_count, vulkan_swapchain_images.data()));

        images.resize(vulkan_swapchain_image_count);
        for(u32 swapchain_image_index = 0; swapchain_image_index < vulkan_swapchain_image_count; swapchain_image_index++)
        {
            CreateImageInfo image_info = {
                .format = surface_format.format,
                .extent = {surface_extent.width, surface_extent.height, 1},
                .usage = usage,
                .name = fmt::format("swapchain {}", swapchain_image_index)
            };
            images.at(swapchain_image_index) = device->create_swapchain_image(vulkan_swapchain_images.at(swapchain_image_index), image_info);
        }
        VkDebugUtilsObjectNameInfoEXT const swapchain_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
            .objectHandle = reinterpret_cast<u64>(swapchain),
            .pObjectName = "FF Swapchain",
        };
        CHECK_VK_RESULT(device->vkSetDebugUtilsObjectNameEXT(device->vulkan_device, &swapchain_name_info));
    }

    Swapchain::Swapchain(CreateSwapchainInfo const & info) :
        device{info.device},
        instance{info.instance},
        window_handle{info.window_handle},
        vkCreateWin32SurfaceKHR{reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance->vulkan_instance,"vkCreateWin32SurfaceKHR"))}
    {
        create_surface();

        u32 present_mode_count = 0;
        std::vector<VkPresentModeKHR> present_modes = {};
        CHECK_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device->vulkan_physical_device, surface, &present_mode_count, nullptr));
        present_modes.resize(present_mode_count);
        CHECK_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device->vulkan_physical_device, surface, &present_mode_count, present_modes.data()));

        auto present_mode_selector = [](VkPresentModeKHR const & present_mode)
        {
            switch(present_mode)
            {
                case VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR: return 100;
                case VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR: return 90;
                case VkPresentModeKHR::VK_PRESENT_MODE_FIFO_RELAXED_KHR: return 80;
                case VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR: return 70;
                default: return 0;
            }
        };

        auto present_mode_comparator = [&](VkPresentModeKHR const & a, VkPresentModeKHR const & b)
        {
            return present_mode_selector(a) < present_mode_selector(a); 
        };
        auto const best_present_mode_it = std::max_element(present_modes.begin(), present_modes.end(), present_mode_comparator);
        present_mode = *best_present_mode_it;
        if(present_mode_selector(present_mode) == 0) { BACKEND_LOG(fmt::format("[WARN][Swapchain::Swapchain()] Found only present mode which was not explicitly wanted")); }

        u32 format_count = 0;
        std::vector<VkSurfaceFormatKHR> formats = {};
        CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device->vulkan_physical_device, surface, &format_count, nullptr));
        formats.resize(format_count);
        CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device->vulkan_physical_device, surface, &format_count, formats.data()));

        if(format_count == 0) { throw std::runtime_error("[ERROR][Swaphcain::Swapchain()] Found no surface formats"); }

        auto format_selector = [](VkFormat const & format) -> i32
        {
            switch (format)
            {
                case VkFormat::VK_FORMAT_R8G8B8A8_SRGB: return 100;
                case VkFormat::VK_FORMAT_R8G8B8A8_UNORM: return 90;
                case VkFormat::VK_FORMAT_B8G8R8A8_SRGB: return 80;
                case VkFormat::VK_FORMAT_B8G8R8A8_UNORM: return 70;
                default: return 0;
            }
        };

        auto format_comparator = [&](VkSurfaceFormatKHR const & a, VkSurfaceFormatKHR const & b)
        {
            return format_selector(a.format) < format_selector(b.format);
        };

        auto const best_format_it = std::max_element(formats.begin(), formats.end(), format_comparator);
        surface_format = *best_format_it;
        if(format_selector(surface_format.format) == 0) { BACKEND_LOG(fmt::format("[Swapchain::Swapchain()][WARN] Found only format which was not explicitly wanted")); }
        /// TODO: (msakmary) Add support for resizing
        VkSurfaceCapabilitiesKHR surface_capabilities;
        CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->vulkan_physical_device, surface, &surface_capabilities));
        surface_extent = {
            .width = surface_capabilities.currentExtent.width,
            .height = surface_capabilities.currentExtent.height
        };

        VkImageUsageFlags const usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkSwapchainCreateInfoKHR const swapchain_create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = surface,
            .minImageCount = MIN_IMAGE_COUNT,
            .imageFormat = surface_format.format,
            .imageColorSpace = surface_format.colorSpace,
            .imageExtent = surface_extent,
            .imageArrayLayers = 1,
            .imageUsage = usage,
            .imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = reinterpret_cast<u32*>(&device->main_queue_family_index),
            .preTransform = VkSurfaceTransformFlagBitsKHR::VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr,
        };

        CHECK_VK_RESULT(vkCreateSwapchainKHR(device->vulkan_device, &swapchain_create_info, nullptr, &swapchain));
        BACKEND_LOG("[INFO][Swapchain::Swapchain()] Swapchain creation successful");

        u32 vulkan_swapchain_image_count = 0;
        std::vector<VkImage> vulkan_swapchain_images = {};
        CHECK_VK_RESULT(vkGetSwapchainImagesKHR(device->vulkan_device, swapchain, &vulkan_swapchain_image_count, nullptr));
        vulkan_swapchain_images.resize(vulkan_swapchain_image_count);
        CHECK_VK_RESULT(vkGetSwapchainImagesKHR(device->vulkan_device, swapchain, &vulkan_swapchain_image_count, vulkan_swapchain_images.data()));

        images.resize(vulkan_swapchain_image_count);
        for(u32 swapchain_image_index = 0; swapchain_image_index < vulkan_swapchain_image_count; swapchain_image_index++)
        {
            CreateImageInfo image_info = {
                .format = surface_format.format,
                .extent = {surface_extent.width, surface_extent.height, 1},
                .usage = usage,
                .name = fmt::format("swapchain {}", swapchain_image_index)
            };
            images.at(swapchain_image_index) = device->create_swapchain_image(vulkan_swapchain_images.at(swapchain_image_index), image_info);
        }
        VkDebugUtilsObjectNameInfoEXT const swapchain_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
            .objectHandle = reinterpret_cast<u64>(swapchain),
            .pObjectName = "FF Swapchain",
        };
        CHECK_VK_RESULT(device->vkSetDebugUtilsObjectNameEXT(device->vulkan_device, &swapchain_name_info));

        swapchain_cpu_timeline = 0;
        VkSemaphoreTypeCreateInfo timeline_semaphore_type_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = nullptr,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = swapchain_cpu_timeline
        };

        VkSemaphoreCreateInfo const timeline_semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &timeline_semaphore_type_create_info,
            .flags = {}
        };
        CHECK_VK_RESULT(vkCreateSemaphore(device->vulkan_device, &timeline_semaphore_create_info, nullptr, &swapchain_timeline_semaphore));
        BACKEND_LOG("[INFO][Swapchain::Swapchain()] Swapchain timeline semaphore creation successful");

        for(u32 swapchain_semaphore_idx = 0; swapchain_semaphore_idx < FRAMES_IN_FLIGHT; swapchain_semaphore_idx++)
        {
            auto & current_acquire_semaphore = swapchain_acquire_semaphores.at(swapchain_semaphore_idx);
            auto & current_present_semaphore = swapchain_present_semaphores.at(swapchain_semaphore_idx);
            VkSemaphoreCreateInfo const semaphore_create_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {}
            };
            CHECK_VK_RESULT(vkCreateSemaphore(device->vulkan_device, &semaphore_create_info, nullptr, &current_acquire_semaphore));
            CHECK_VK_RESULT(vkCreateSemaphore(device->vulkan_device, &semaphore_create_info, nullptr, &current_present_semaphore));
            auto const acquire_name = fmt::format("Swapchain acquire sema {}", swapchain_semaphore_idx);
            VkDebugUtilsObjectNameInfoEXT const acquire_name_info{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .pNext = nullptr,
                .objectType = VK_OBJECT_TYPE_SEMAPHORE,
                .objectHandle = reinterpret_cast<u64>(current_acquire_semaphore),
                .pObjectName = acquire_name.c_str(),
            };
            auto const present_name = fmt::format("Swapchain present sema {}", swapchain_semaphore_idx);
            VkDebugUtilsObjectNameInfoEXT const present_name_info{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .pNext = nullptr,
                .objectType = VK_OBJECT_TYPE_SEMAPHORE,
                .objectHandle = reinterpret_cast<u64>(current_present_semaphore),
                .pObjectName = present_name.c_str(),
            };
            CHECK_VK_RESULT(device->vkSetDebugUtilsObjectNameEXT(device->vulkan_device, &acquire_name_info));
            CHECK_VK_RESULT(device->vkSetDebugUtilsObjectNameEXT(device->vulkan_device, &present_name_info));
        }
        BACKEND_LOG("[INFO][Swapchain::Swapchain()] Swapchain acquire and present semaphores creation successful");
    }

    auto Swapchain::acquire_next_image() -> ImageId
    {
        const u64 waited_value = static_cast<u64>(std::max(static_cast<i64>(swapchain_cpu_timeline) - static_cast<i64>(FRAMES_IN_FLIGHT), 0ll));
        VkSemaphoreWaitInfo const semaphore_wait_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = {},
            .semaphoreCount = 1,
            .pSemaphores = &swapchain_timeline_semaphore,
            .pValues = &waited_value
        };
        CHECK_VK_RESULT(vkWaitSemaphores(device->vulkan_device, &semaphore_wait_info, std::numeric_limits<u32>::max()));
        current_semaphore_index = (swapchain_cpu_timeline) % FRAMES_IN_FLIGHT;
        VkSemaphore const & acquire_semaphore = swapchain_acquire_semaphores.at(current_semaphore_index);
        CHECK_VK_RESULT(vkAcquireNextImageKHR(device->vulkan_device, swapchain, std::numeric_limits<u64>::max(), acquire_semaphore, nullptr, &current_acquired_image_index));
        swapchain_cpu_timeline += 1;
        return images.at(current_acquired_image_index);
    }

    auto Swapchain::get_current_acquire_semaphore() -> VkSemaphore
    {
        return swapchain_acquire_semaphores.at(current_semaphore_index);
    }

    auto Swapchain::get_current_present_semaphore() -> VkSemaphore
    {
        return swapchain_present_semaphores.at(current_semaphore_index);
    }

    auto Swapchain::get_timeline_semaphore() -> VkSemaphore
    {
        return swapchain_timeline_semaphore;
    }

    auto Swapchain::get_timeline_cpu_value() -> u64
    {
        return swapchain_cpu_timeline;
    }

    void Swapchain::present(PresentInfo const & info)
    {
        VkPresentInfoKHR const present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<u32>(info.wait_semaphores.size()),
            .pWaitSemaphores = info.wait_semaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &current_acquired_image_index,
            .pResults = {}
        };

        CHECK_VK_RESULT(vkQueuePresentKHR(device->main_queue, &present_info));
    }

    Swapchain::~Swapchain()
    {
        vkDestroySemaphore(device->vulkan_device, swapchain_timeline_semaphore, nullptr);
        BACKEND_LOG("[INFO][Swapchain::~Swapchain] Swapchain timeline semaphore destroyed")
        for(auto const & swapchain_acquire_semaphore : swapchain_acquire_semaphores)
        {
            vkDestroySemaphore(device->vulkan_device, swapchain_acquire_semaphore, nullptr);
        }
        for(auto const & swapchain_present_semaphore : swapchain_present_semaphores)
        {
            vkDestroySemaphore(device->vulkan_device, swapchain_present_semaphore, nullptr);
        }
        BACKEND_LOG("[INFO][Swapchain::~Swapchain] Swapchain acquire and present semaphores destroyed")
        for(ImageId const & id : images)
        {
            device->destroy_swapchain_image(id);
        }
        vkDestroySwapchainKHR(device->vulkan_device, swapchain, nullptr);
        BACKEND_LOG("[INFO][Swapchain::~Swapchain] Surface destroyed")
        vkDestroySurfaceKHR(instance->vulkan_instance, surface, nullptr);
        BACKEND_LOG("[INFO][Swapchain::~Swapchain] Swapchain destroyed")
    }
}