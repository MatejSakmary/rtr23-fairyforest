#pragma once
#include "core.hpp"
#include "instance.hpp"
#include "device.hpp"

namespace ff
{
    struct CreateSwapchainInfo
    {
        std::shared_ptr<Instance> instance = {};
        std::shared_ptr<Device> device = {};
        void * window_handle = {};
    };

    constexpr u32 FRAMES_IN_FLIGHT = 2;
    constexpr u32 MIN_IMAGE_COUNT = FRAMES_IN_FLIGHT + 1;
    struct Swapchain
    {
        public:
            VkSurfaceFormatKHR surface_format = {};
            VkPresentModeKHR present_mode = {};
            VkExtent2D surface_extent = {};

            Swapchain() = default;
            Swapchain(CreateSwapchainInfo const & info);
            ~Swapchain();

        private:
            friend struct Device;

            std::shared_ptr<Device> device = {};
            std::shared_ptr<Instance> instance = {};
            std::vector<ImageId> images = {};
            VkSwapchainKHR swapchain = {};

            void * window_handle = {};
            VkSurfaceKHR surface = {};
            PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = {};

            void create_surface();
    };
}
