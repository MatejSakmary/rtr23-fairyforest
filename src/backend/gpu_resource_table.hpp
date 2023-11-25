#pragma once
#include "core.hpp"
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
            friend struct GpuResourceTable;
            friend struct CommandBuffer;
            VkImage image;
            VkImageView image_view;
    };

    using ImageId = SlotMap<Image>::Id;
    using BufferId = SlotMap<VkBuffer>::Id;
    using SamplerId = SlotMap<VkSampler>::Id;

    struct CreateGpuResourceTableInfo
    {
        u32 max_buffer_slots = {};
        u32 max_image_slots = {};
        u32 max_sampler_slots = {};
        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = {};
        VkDevice vulkan_device = {};
    };

    static inline constexpr u32 MAX_PUSH_CONSTANT_WORD_SIZE = {32};
    static inline constexpr u32 MAX_PUSH_CONSTANT_BYTE_SIZE = {MAX_PUSH_CONSTANT_WORD_SIZE * 4};
    static inline constexpr u32 PIPELINE_LAYOUT_COUNT = {MAX_PUSH_CONSTANT_WORD_SIZE + 1};

    static inline constexpr u32 BUFFER_BINDING = 0;
    static inline constexpr u32 STORAGE_IMAGE_BINDING = 1;
    static inline constexpr u32 SAMPLED_IMAGE_BINDING = 2;
    static inline constexpr u32 SAMPLER_BINDING = 3;

    struct GpuResourceTable
    {
        public:
            SlotMap<Image> images = {};
            SlotMap<VkBuffer> buffers = {};
            SlotMap<VkSampler> samplers = {};

            GpuResourceTable(CreateGpuResourceTableInfo const & info);
            void write_descriptor_set_image(ImageId id);
            void write_descriptor_set_buffer(BufferId id);
            void write_descriptor_set_sampler(SamplerId id);
            ~GpuResourceTable();

        private:
            friend struct Pipeline;
            friend struct CommandBuffer;
            CreateGpuResourceTableInfo info = {};
            std::array<VkPipelineLayout, PIPELINE_LAYOUT_COUNT> pipeline_layouts = {};
            VkDevice vulkan_device = {};
            VkDescriptorSetLayout descriptor_set_layout = {};
            VkDescriptorSet descriptor_set = {};
            VkDescriptorPool descriptor_pool = {};
    };
}