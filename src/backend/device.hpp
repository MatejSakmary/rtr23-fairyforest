#pragma once

#include "core.hpp"
#include "instance.hpp"
#include "gpu_resource_table.hpp"
#include "../fairy_forest.hpp"

namespace ff
{
    struct Device
    {
        public:
            VkPhysicalDeviceProperties2 physical_device_properties = {};
            VkQueue main_queue = {};
            VkSemaphore main_gpu_semaphore = {};

            Device() = default;
            Device(std::shared_ptr<Instance> instance);
            ~Device();

        private:
            friend struct Swapchain;

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

            i32 main_queue_family_index = {};

            auto create_swapchain_image(VkImage swapchain_image, CreateImageInfo const & info) -> ImageId;
            void destroy_swapchain_image(ImageId id);
            auto get_physical_device() -> VkPhysicalDevice;
    };
}
