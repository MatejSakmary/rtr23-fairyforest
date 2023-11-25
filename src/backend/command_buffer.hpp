#pragma once

#include <variant>

#include "core.hpp"
#include "device.hpp"
#include "pipeline.hpp"


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

    struct RenderingAttachmentInfo
    {
        /// TODO: Pass ImageView instead of ImageId
        ImageId image_id = {};
        VkImageLayout layout = {};
        VkAttachmentLoadOp load_op = {};
        VkAttachmentStoreOp store_op = {};
        VkClearValue clear_value = {};
    };

    struct BeginRenderpassInfo
    {
        std::vector<RenderingAttachmentInfo> color_attachments = {};
        std::optional<RenderingAttachmentInfo> depth_attachment = {};
        std::optional<RenderingAttachmentInfo> stencil_attachment = {};
        VkRect2D render_area = {};
    };

    struct DrawInfo
    {
        u32 vertex_count = 0;
        u32 instance_count = 1;
        u32 first_vertex = 0;
        u32 first_instance = 0; 
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
            void cmd_image_clear(ImageClearInfo const & info);
            void cmd_set_raster_pipeline(Pipeline const & pipeline);
            void cmd_draw(DrawInfo const & info);
            void cmd_begin_renderpass(BeginRenderpassInfo const & info);
            void cmd_end_renderpass();
            auto get_recorded_command_buffer() -> VkCommandBuffer;
        
        private:
            bool recording = {};
            bool was_recorded = {};
            bool in_renderpass = {};
            std::shared_ptr<Device> device = {};
            VkCommandPool pool = {};
            VkCommandBuffer buffer = {};
    };
}