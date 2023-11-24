#pragma once
#include "core.hpp"
#include "device.hpp"
#include <variant>

namespace ff
{
    struct ImageMemoryBarrierTransitionInfo
    {
        VkPipelineStageFlags2 src_stages = {};
        VkAccessFlags2 src_access = {};
        VkPipelineStageFlags2 dst_stages = {};
        VkAccessFlags2 dst_access = {};
        VkImageLayout src_layout = {};
        VkImageLayout dst_layout = {};
        u32 base_mip_level = 0;
        u32 level_count = 1;
        u32 base_array_layer = 0;
        u32 layer_count = 1;
        VkImageAspectFlags aspect_mask = {};
        ImageId image_id = {};
    };

    struct ImageClearInfo
    {
        VkImageLayout layout = {};
        VkClearColorValue clear_value = {};
        ImageId image_id = {};
        u32 base_mip_level = 0;
        u32 level_count = 1;
        u32 base_array_layer = 0;
        u32 layer_count = 1;
        VkImageAspectFlags aspect_mask = {};
    };

    struct CommandBuffer
    {
        public:
            CommandBuffer() = default;
            CommandBuffer(std::shared_ptr<Device> device);
            ~CommandBuffer();

            void begin();
            void end();
            void cmd_image_memory_transition_barrier(ImageMemoryBarrierTransitionInfo const & info);
            void cmd_image_clear(ImageClearInfo const & info );
            auto get_recorded_command_buffer() -> VkCommandBuffer;
        
        private:
            bool recording = {};
            bool was_recorded = {};
            std::shared_ptr<Device> device = {};
            VkCommandPool pool = {};
            VkCommandBuffer buffer = {};
    };
}