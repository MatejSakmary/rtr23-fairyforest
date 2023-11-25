#include "command_buffer.hpp"
namespace  ff 
{
    CommandBuffer::CommandBuffer(std::shared_ptr<Device> device) :
        device{device},
        was_recorded{false},
        in_renderpass{false}
    {
        VkCommandPoolCreateInfo const command_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = static_cast<u32>(device->main_queue_family_index)
        };

        CHECK_VK_RESULT(vkCreateCommandPool(device->vulkan_device, &command_pool_create_info, nullptr, &pool));

        VkCommandBufferAllocateInfo const command_buffer_allocate_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        CHECK_VK_RESULT(vkAllocateCommandBuffers(device->vulkan_device, &command_buffer_allocate_info, &buffer));
    }

    void CommandBuffer::begin()
    {
        if(recording == true) 
        {
            BACKEND_LOG("[ERROR][CommandBuffer::begin()] begin() called twice");
            throw std::runtime_error("[ERROR][CommandBuffer::begin()] begin() called twice");
        }
        if(was_recorded == true)
        {
            BACKEND_LOG("[ERROR][CommandBuffer::begin()] buffer was already recorded");
            throw std::runtime_error("[ERROR][CommandBuffer::begin()] buffer was already recorded");
        }

        VkCommandBufferBeginInfo const command_buffer_begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = {},
        };
        CHECK_VK_RESULT(vkBeginCommandBuffer(buffer, &command_buffer_begin_info));
        recording = true;
    }

    void CommandBuffer::end()
    {
        if(recording != true) 
        {
            BACKEND_LOG("[ERROR][CommandBuffer::end()] Did not call begin()");
            throw std::runtime_error("[ERROR][CommandBuffer::end()] Did not call begin()");
        }
        CHECK_VK_RESULT(vkEndCommandBuffer(buffer));
        recording = false;
        was_recorded = true;
    }

    void CommandBuffer::cmd_image_memory_transition_barrier(ImageMemoryBarrierTransitionInfo const & info)
    {
        VkPipelineStageFlags2 src_stage = {};
        VkAccessFlags2 src_access = {};
        VkPipelineStageFlags2 dst_stage = {};
        VkAccessFlags2 dst_access = {};
        VkImageLayout src_layout = {};
        VkImageLayout dst_layout = {};
        u32 base_mip_level = 0;
        u32 level_count = 1;
        u32 base_array_layer = 0;
        u32 layer_count = 1;
        ImageId image_id = {};

        if(!device->resource_table->images.is_id_valid(info.image_id)) 
        { 
            BACKEND_LOG("[ERROR][CommandBuffer::cmd_image_memory_transition_barrier()] Received invalid image ID");
            throw std::runtime_error("[ERROR][CommandBuffer::cmd_image_memory_transition_barrier()] Received invalid image ID");
        }
        auto const & image = device->resource_table->images.slot(info.image_id);
        VkImageMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = info.src_stages,
            .srcAccessMask = info.src_access,
            .dstStageMask = info.dst_stages,
            .dstAccessMask = info.dst_access,
            .oldLayout = info.src_layout,
            .newLayout = info.dst_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image->image,
            .subresourceRange = {
                .aspectMask = info.aspect_mask,
                .baseMipLevel = info.base_mip_level,
                .levelCount = info.level_count,
                .baseArrayLayer = info.base_array_layer,
                .layerCount = info.layer_count
            }
        };

        VkDependencyInfo const dependency_info = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = {},
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };
        vkCmdPipelineBarrier2(buffer, &dependency_info);
    }

    void CommandBuffer::cmd_image_clear(ImageClearInfo const & info)
    {
        if(!device->resource_table->images.is_id_valid(info.image_id)) 
        { 
            BACKEND_LOG("[ERROR][CommandBuffer::cmd_image_clear()] Received invalid image ID");
            throw std::runtime_error("[ERROR][CommandBuffer::cmd_image_clear()] Received invalid image ID");
        }
        auto const & image = device->resource_table->images.slot(info.image_id);
        /// TODO: (msakmary) add support for clearing Depth/Stencil images
        VkImageSubresourceRange subresource_range = {
            .aspectMask = info.aspect_mask,
            .baseMipLevel = info.base_mip_level,
            .levelCount = info.level_count,
            .baseArrayLayer = info.base_array_layer,
            .layerCount = info.layer_count
        };
        vkCmdClearColorImage(buffer, image->image, info.layout, &info.clear_value, 1, &subresource_range);
    }

    auto CommandBuffer::get_recorded_command_buffer() -> VkCommandBuffer
    {
        if(was_recorded == false)
        { 
            BACKEND_LOG("[ERROR][CommandBuffer::get_recorded_command_buffer()] Buffer was not yet recorded");
            throw std::runtime_error("[ERROR][CommandBuffer::get_recorded_command_buffer()] Buffer was not yet recorded");
        }

        return buffer;
    }

    void CommandBuffer::cmd_set_raster_pipeline(Pipeline const & pipeline)
    {
        vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &device->resource_table->descriptor_set, 0, nullptr);
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    }

    void CommandBuffer::cmd_draw(DrawInfo const & info)
    {
        vkCmdDraw(buffer, info.vertex_count, info.instance_count, info.first_vertex, info.first_instance);
    }

    void CommandBuffer::cmd_begin_renderpass(BeginRenderpassInfo const & info)
    {
        auto fill_rendering_attachment_info = [&](RenderingAttachmentInfo const & in) -> VkRenderingAttachmentInfo 
        {
            return VkRenderingAttachmentInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = device->resource_table->images.slot(in.image_id)->image_view,
                .imageLayout = in.layout,
                .resolveMode = VkResolveModeFlagBits::VK_RESOLVE_MODE_NONE,
                .resolveImageView = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = in.load_op,
                .storeOp = in.store_op,
                .clearValue = in.clear_value,
            };
        };

        std::vector<VkRenderingAttachmentInfo> color_attachments = {};
        color_attachments.reserve(info.color_attachments.size());
        for (usize i = 0; i < info.color_attachments.size(); ++i)
        {
            if(!device->resource_table->images.is_id_valid(info.color_attachments.at(i).image_id))
            {
                BACKEND_LOG(fmt::format("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid image id in color attachment index {}", i));
                throw std::runtime_error("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid color image id");
            }
            color_attachments.push_back(fill_rendering_attachment_info(info.color_attachments.at(i)));
        }
        VkRenderingAttachmentInfo depth_attachment_info = {};
        if (info.depth_attachment.has_value())
        {
            if(!device->resource_table->images.is_id_valid(info.depth_attachment.value().image_id))
            {
                BACKEND_LOG(fmt::format("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid image id in depth attachment index"));
                throw std::runtime_error("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid depth image id");
            }
            depth_attachment_info = fill_rendering_attachment_info(info.depth_attachment.value());
        };
        VkRenderingAttachmentInfo stencil_attachment_info = {};
        if (info.stencil_attachment.has_value())
        {
            if(!device->resource_table->images.is_id_valid(info.stencil_attachment.value().image_id))
            {
                BACKEND_LOG(fmt::format("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid image id in stencil attachment index"));
                throw std::runtime_error("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid stencil image id");
            }
            stencil_attachment_info = fill_rendering_attachment_info(info.stencil_attachment.value());
        };

        VkRenderingInfo const rendering_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = {},
            .renderArea = info.render_area,
            .layerCount = 1,
            .viewMask = {},
            .colorAttachmentCount = static_cast<u32>(info.color_attachments.size()),
            .pColorAttachments = color_attachments.data(),
            .pDepthAttachment = info.depth_attachment.has_value() ? &depth_attachment_info : nullptr,
            .pStencilAttachment = info.stencil_attachment.has_value() ? &stencil_attachment_info : nullptr,
        };
        vkCmdSetScissor(buffer, 0, 1, &info.render_area);
        VkViewport const viewport = {
            .x = static_cast<f32>(info.render_area.offset.x),
            .y = static_cast<f32>(info.render_area.offset.y),
            .width = static_cast<f32>(info.render_area.extent.width),
            .height = static_cast<f32>(info.render_area.extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(buffer, 0, 1, &viewport);
        vkCmdBeginRendering(buffer, &rendering_info);
        in_renderpass = true;
    }

    void CommandBuffer::cmd_end_renderpass()
    {
        if(in_renderpass == false)
        {
            BACKEND_LOG(fmt::format("[ERROR][CommandBuffer::cmd_end_renderpass()] end_renderpass() called without calling begin_renderpass() first"));
            throw std::runtime_error("[ERROR][CommandBuffer::cmd_end_renderpass()] end_renderpass() called without calling begin_renderpass() first");
        }
        vkCmdEndRendering(buffer);
    }

    CommandBuffer::~CommandBuffer()
    {
        device->command_buffer_zombies.push({
            .pool = pool,
            .buffer = buffer,
            .cpu_timeline_value = device->main_cpu_timeline_value
        });
    }
}