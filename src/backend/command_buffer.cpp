#include "command_buffer.hpp"
namespace ff
{
    CommandBuffer::CommandBuffer(std::shared_ptr<Device> device)
        : device{device},
          was_recorded{false},
          in_renderpass{false}
    {
        VkCommandPoolCreateInfo const command_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = static_cast<u32>(device->main_queue_family_index),
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
        if (recording == true)
        {
            BACKEND_LOG("[ERROR][CommandBuffer::begin()] begin() called twice");
            throw std::runtime_error("[ERROR][CommandBuffer::begin()] begin() called twice");
        }
        if (was_recorded == true)
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
        if (recording != true)
        {
            BACKEND_LOG("[ERROR][CommandBuffer::end()] Did not call begin()");
            throw std::runtime_error("[ERROR][CommandBuffer::end()] Did not call begin()");
        }
        CHECK_VK_RESULT(vkEndCommandBuffer(buffer));
        recording = false;
        was_recorded = true;
    }

    void CommandBuffer::cmd_memory_barrier(MemoryBarrierInfo const & info)
    {
        VkMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = info.src_stages,
            .srcAccessMask = info.src_access,
            .dstStageMask = info.dst_stages,
            .dstAccessMask = info.dst_access,
        };

        VkDependencyInfo const dependency_info = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = {},
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &barrier,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 0,
            .pImageMemoryBarriers = nullptr,
        };
        vkCmdPipelineBarrier2(buffer, &dependency_info);
    }

    void CommandBuffer::cmd_set_push_constant_internal(void const * data, u32 size)
    {
        u32 const layout_index = (size + sizeof(u32) - 1) / sizeof(u32);
        vkCmdPushConstants(buffer, device->resource_table->pipeline_layouts.at(layout_index), VK_SHADER_STAGE_ALL, 0, size, data);
    }

    void CommandBuffer::cmd_image_memory_transition_barrier(ImageMemoryBarrierTransitionInfo const & info)
    {
        if (!device->resource_table->images.is_id_valid(info.image_id))
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
                .layerCount = info.layer_count},
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
            .pImageMemoryBarriers = &barrier,
        };
        vkCmdPipelineBarrier2(buffer, &dependency_info);
    }

    void CommandBuffer::cmd_copy_buffer_to_buffer(CopyBufferToBufferInfo const & info)
    {
        auto const copy_info = VkBufferCopy{
            .srcOffset = info.src_offset,
            .dstOffset = info.dst_offset,
            .size = info.size,
        };

        VkBuffer src_buffer = device->resource_table->buffers.slot(info.src_buffer)->buffer;
        VkBuffer dst_buffer = device->resource_table->buffers.slot(info.dst_buffer)->buffer;
        vkCmdCopyBuffer(buffer, src_buffer, dst_buffer, 1, &copy_info);
    }

    void CommandBuffer::cmd_copy_buffer_to_image(CopyBufferToImageInfo const & info)
    {

        if (!device->resource_table->images.is_id_valid(info.image_id))
        {
            BACKEND_LOG("[ERROR][CommandBuffer::cmd_copy_buffer_to_image()] Received invalid image ID");
            throw std::runtime_error("[ERROR][CommandBuffer::cmd_copy_buffer_to_image()] Received invalid image ID");
        }
        if (!device->resource_table->buffers.is_id_valid(info.buffer_id))
        {
            BACKEND_LOG("[ERROR][CommandBuffer::cmd_copy_buffer_to_image()] Received invalid buffer ID");
            throw std::runtime_error("[ERROR][CommandBuffer::cmd_copy_buffer_to_image()] Received invalid buffer ID");
        }
        auto const & dst_image = device->resource_table->images.slot(info.image_id);
        auto const & src_buffer = device->resource_table->buffers.slot(info.buffer_id);
        VkBufferImageCopy const buffer_image_copy = {
            .bufferOffset = info.buffer_offset,
            .bufferRowLength = 0u,
            .bufferImageHeight = 0u,
            .imageSubresource = VkImageSubresourceLayers{
                .aspectMask = info.aspect_mask,
                .mipLevel = info.base_mip_level,
                .baseArrayLayer = info.base_array_layer,
                .layerCount = info.layer_count,
            },
            .imageOffset = info.image_offset,
            .imageExtent = info.image_extent
        };
        vkCmdCopyBufferToImage(buffer, src_buffer->buffer, dst_image->image, info.image_layout, 1, &buffer_image_copy);
    }

    void CommandBuffer::cmd_blit_image(BlitImageInfo const & info)
    {
        if (!device->resource_table->images.is_id_valid(info.src_image))
        {
            BACKEND_LOG("[ERROR][CommandBuffer::cmd_blit_image()] Received invalid src image ID");
            throw std::runtime_error("[ERROR][CommandBuffer::cmd_blit_image()] Received invalid src image ID");
        }
        if (!device->resource_table->images.is_id_valid(info.dst_image))
        {
            BACKEND_LOG("[ERROR][CommandBuffer::cmd_blit_image()] Received invalid dst image ID");
            throw std::runtime_error("[ERROR][CommandBuffer::cmd_blit_image()] Received invalid dst image ID");
        }
        auto const & src_image = device->resource_table->images.slot(info.src_image);
        auto const & dst_image = device->resource_table->images.slot(info.dst_image);

        // VkImageSubresourceLayers    srcSubresource;
        // VkOffset3D                  srcOffsets[2];
        // VkImageSubresourceLayers    dstSubresource;
        // VkOffset3D                  dstOffsets[2];
        auto const blit = VkImageBlit{
            .srcSubresource = VkImageSubresourceLayers{
                .aspectMask = info.src_aspect_mask,
                .mipLevel = info.src_mip_level,
                .baseArrayLayer = info.src_base_array_layer,
                .layerCount = info.src_layer_count
            },
            .srcOffsets = { info.src_start_offset, info.src_end_offset },
            .dstSubresource = VkImageSubresourceLayers {
                .aspectMask = info.dst_aspect_mask,
                .mipLevel = info.dst_mip_level,
                .baseArrayLayer = info.dst_base_array_layer,
                .layerCount = info.dst_layer_count
            },
            .dstOffsets = { info.dst_start_offset, info.dst_end_offset },
        };

        vkCmdBlitImage(buffer, src_image->image, info.src_layout, dst_image->image, info.dst_layout, 1, &blit, VkFilter::VK_FILTER_LINEAR);
    }

    void CommandBuffer::cmd_image_clear(ImageClearInfo const & info)
    {
        if (!device->resource_table->images.is_id_valid(info.image_id))
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
            .layerCount = info.layer_count,
        };
        vkCmdClearColorImage(buffer, image->image, info.layout, &info.clear_value, 1, &subresource_range);
    }

    auto CommandBuffer::get_recorded_command_buffer() -> VkCommandBuffer
    {
        if (was_recorded == false)
        {
            BACKEND_LOG("[ERROR][CommandBuffer::get_recorded_command_buffer()] Buffer was not yet recorded");
            throw std::runtime_error("[ERROR][CommandBuffer::get_recorded_command_buffer()] Buffer was not yet recorded");
        }

        return buffer;
    }

    void CommandBuffer::cmd_set_raster_pipeline(RasterPipeline const & pipeline)
    {
        vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &device->resource_table->descriptor_set, 0, nullptr);
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    }

    void CommandBuffer::cmd_set_compute_pipeline(ComputePipeline const & pipeline)
    {
        vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0, 1, &device->resource_table->descriptor_set, 0, nullptr);
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    }

    void CommandBuffer::cmd_dispatch(DispatchInfo const & info)
    {
        vkCmdDispatch(buffer, info.x, info.y, info.z);
    }

    void CommandBuffer::cmd_draw(DrawInfo const & info)
    {
        vkCmdDraw(buffer, info.vertex_count, info.instance_count, info.first_vertex, info.first_instance);
    }

    void CommandBuffer::cmd_draw_indexed(DrawIndexedInfo const & info)
    {
        vkCmdDrawIndexed(buffer, info.index_count, info.instance_count, info.first_index, info.vertex_offset, info.first_instance);
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
            if (!device->resource_table->images.is_id_valid(info.color_attachments.at(i).image_id))
            {
                BACKEND_LOG(fmt::format("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid image id in color attachment index {}", i));
                throw std::runtime_error("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid color image id");
            }
            color_attachments.push_back(fill_rendering_attachment_info(info.color_attachments.at(i)));
        }
        VkRenderingAttachmentInfo depth_attachment_info = {};
        if (info.depth_attachment.has_value())
        {
            if (!device->resource_table->images.is_id_valid(info.depth_attachment.value().image_id))
            {
                BACKEND_LOG(fmt::format("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid image id in depth attachment index"));
                throw std::runtime_error("[ERROR][CommandBuffer::cmd_begin_renderpass()] Invalid depth image id");
            }
            depth_attachment_info = fill_rendering_attachment_info(info.depth_attachment.value());
        };
        VkRenderingAttachmentInfo stencil_attachment_info = {};
        if (info.stencil_attachment.has_value())
        {
            if (!device->resource_table->images.is_id_valid(info.stencil_attachment.value().image_id))
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

    void CommandBuffer::cmd_set_viewport(VkViewport const & info)
    {
        vkCmdSetViewport(buffer, 0, 1, &info);
    };

    void CommandBuffer::cmd_set_index_buffer(SetIndexBufferInfo const & info)
    {
        if (!device->resource_table->buffers.is_id_valid(info.buffer_id))
        {
            BACKEND_LOG("[ERROR][CommandBuffer::cmd_set_index_buffer()] Received invalid buffer ID");
            throw std::runtime_error("[ERROR][CommandBuffer::cmd_set_index_buffer()] Received invalid buffer ID");
        }
        auto const & src_buffer = device->resource_table->buffers.slot(info.buffer_id);
        vkCmdBindIndexBuffer(buffer, src_buffer->buffer, info.offset, info.index_type);
    }

    void CommandBuffer::cmd_end_renderpass()
    {
        if (in_renderpass == false)
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
            .cpu_timeline_value = device->main_cpu_timeline_value,
        });
    }
} // namespace ff