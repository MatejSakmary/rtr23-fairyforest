#include "command_buffer.hpp"
namespace  ff 
{
    CommandBuffer::CommandBuffer(std::shared_ptr<Device> device) :
        device{device},
        was_recorded{false}
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

    CommandBuffer::~CommandBuffer()
    {
        device->command_buffer_zombies.push({
            .pool = pool,
            .buffer = buffer,
            .cpu_timeline_value = device->main_cpu_timeline_value
        });
    }
}