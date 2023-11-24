#pragma once
#include "core.hpp"
#include "instance.hpp"
#include "device.hpp"
#include <span>

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

    struct PresentInfo
    {
        std::span<VkSemaphore> wait_semaphores;
    };
    struct Swapchain
    {
        public:
            VkSurfaceFormatKHR surface_format = {};
            VkPresentModeKHR present_mode = {};
            VkExtent2D surface_extent = {};

            Swapchain() = default;
            Swapchain(CreateSwapchainInfo const & info);
            auto acquire_next_image() -> ImageId;
            auto get_current_acquire_semaphore() -> VkSemaphore;
            auto get_current_present_semaphore() -> VkSemaphore;
            auto get_timeline_semaphore() -> VkSemaphore;
            auto get_timeline_cpu_value() -> u64;
            void present(PresentInfo const & inf);
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

            u32 current_acquired_image_index = {};
            u32 current_semaphore_index = {};
            u64 swapchain_cpu_timeline = {};
            VkSemaphore swapchain_timeline_semaphore = {};
            std::array<VkSemaphore, FRAMES_IN_FLIGHT> swapchain_acquire_semaphores = {};
            std::array<VkSemaphore, FRAMES_IN_FLIGHT> swapchain_present_semaphores = {};

            void create_surface();
    };
}
