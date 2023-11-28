#pragma once

#include <variant>

#include "core.hpp"
#include "device.hpp"
#include "pipeline.hpp"

namespace ff
{
    struct CopyBufferToBufferInfo
    {
        BufferId src_buffer = {};
        u32 src_offset = {};
        BufferId dst_buffer = {};
        u32 dst_offset = {};
        u32 size = {};
    };

    struct CopyBufferToImageInfo
    {
        BufferId buffer_id = {};
        size_t buffer_offset = {};
        ImageId image_id = {};
        VkImageLayout image_layout = {};
        u32 base_mip_level = 0;
        u32 base_array_layer = 0;
        u32 layer_count = 1;
        VkImageAspectFlags aspect_mask = {};
        VkOffset3D image_offset = {};
        VkExtent3D image_extent = {};
    };

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

    struct MemoryBarrierInfo
    {
        VkPipelineStageFlags2 src_stages = {};
        VkAccessFlags2 src_access = {};
        VkPipelineStageFlagBits2 dst_stages = {};
        VkAccessFlags2 dst_access = {};
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
        void cmd_copy_buffer_to_buffer(CopyBufferToBufferInfo const & info);
        void cmd_copy_buffer_to_image(CopyBufferToImageInfo const & info);
        void cmd_image_memory_transition_barrier(ImageMemoryBarrierTransitionInfo const & info);
        void cmd_memory_barrier(MemoryBarrierInfo const & info);
        template <typename T>
        void cmd_set_push_constant(T const & push_constant) { cmd_set_push_constant_internal(&push_constant, sizeof(T)); };
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

        void cmd_set_push_constant_internal(void const * data, u32 size);
    };
} // namespace ff