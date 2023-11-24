#pragma once

#include "core.hpp"
#include "instance.hpp"
#include "slotmap.hpp"
#include "../fairy_forest.hpp"

namespace ff
{
    struct CreateImageInfo
    {
        u32 dimensions = 2;
        VkFormat format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
        VkExtent3D extent = {0, 0, 0};
        u32 mip_level_count = 1;
        u32 array_layer_count = 1;
        u32 sample_count = 1;
        VkImageUsageFlags usage = {};
        std::string name = {};
    };

    struct Image 
    {
        public:
            CreateImageInfo image_info;
        private:
            friend struct Device;
            VkImage image;
            VkImageView image_view;
    };

    using ImageId = SlotMap<Image>::Id;
    using BufferId = SlotMap<VkBuffer>::Id;
    using SamplerId = SlotMap<VkSampler>::Id;
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

            VkPhysicalDevice vulkan_physical_device = {};
            VkDevice vulkan_device = {};

            // Debug utils function pointers
            PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = {};
            PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = {};
            PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = {};

            i32 main_queue_family_index = {};
            SlotMap<Image> images = {};
            SlotMap<VkBuffer> buffers = {};
            SlotMap<VkSampler> samplers = {};

            auto create_swapchain_image(VkImage swapchain_image, CreateImageInfo const & info) -> ImageId;
            void destroy_swapchain_image(ImageId id);
            auto get_physical_device() -> VkPhysicalDevice;
    };
}
