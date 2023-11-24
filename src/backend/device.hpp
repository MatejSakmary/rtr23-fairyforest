#pragma once

#include <span>
#include <queue>

#include "core.hpp"
#include "instance.hpp"
#include "gpu_resource_table.hpp"
#include "../fairy_forest.hpp"

namespace ff
{
    struct TimelineSemaphoreInfo
    {
        VkSemaphore semaphore;
        u64 value;
    };
    struct SubmitInfo
    {
        std::span<VkCommandBuffer> command_buffers = {};
        std::span<VkSemaphore> wait_binary_semaphores = {};
        std::span<TimelineSemaphoreInfo> wait_timeline_semaphores = {};
        std::span<VkSemaphore> signal_binary_semaphores = {};
        std::span<TimelineSemaphoreInfo> signal_timeline_semaphores = {};
    };

    struct CommandBufferZombie
    {
        VkCommandPool pool = {};
        VkCommandBuffer buffer = {};
        u64 cpu_timeline_value = {};
    };
    struct Device
    {
        public:
            VkPhysicalDeviceProperties2 physical_device_properties = {};
            VkQueue main_queue = {};
            VkSemaphore main_gpu_semaphore = {};

            Device() = default;
            Device(std::shared_ptr<Instance> instance);
            void submit(SubmitInfo const & info);
            void cleanup_resources();
            ~Device();

        private:
            friend struct Swapchain;
            friend struct CommandBuffer;

            constexpr static u32 MAX_BUFFERS = 1000u;
            constexpr static u32 MAX_IMAGES = 1000u;
            constexpr static u32 MAX_SAMPLERS = 100u;
            std::shared_ptr<Instance> instance;

            std::unique_ptr<GpuResourceTable> resource_table = {};

            VkPhysicalDevice vulkan_physical_device = {};
            VkDevice vulkan_device = {};

            // Debug utils function pointers
            PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = {};
            PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = {};
            PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = {};

            std::queue<CommandBufferZombie> command_buffer_zombies = {};

            i32 main_queue_family_index = {};
            u64 main_cpu_timeline_value = {};

            auto create_swapchain_image(VkImage swapchain_image, CreateImageInfo const & info) -> ImageId;
            void destroy_swapchain_image(ImageId id);
            auto get_physical_device() -> VkPhysicalDevice;
    };
}
