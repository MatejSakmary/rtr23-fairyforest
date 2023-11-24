#include "renderer.hpp"

namespace ff
{
    Renderer::Renderer(std::shared_ptr<Context> context) :
        context{context}
    {
    }

    void Renderer::resize()
    {
        context->swapchain->resize();
    }

    void Renderer::draw_frame()
    {
        auto swapchain_image = context->swapchain->acquire_next_image();
        auto command_buffer = CommandBuffer(context->device);
        command_buffer.begin();
        command_buffer.cmd_image_memory_transition_barrier({
            .src_stages = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .src_access = VK_ACCESS_2_NONE,
            .dst_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
            .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .image_id = swapchain_image
        });

        command_buffer.cmd_image_clear({
            .layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .clear_value = VkClearColorValue{{0.0, 0.0, 1.0, 1.0}},
            .image_id = swapchain_image,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT
        });

        command_buffer.cmd_image_memory_transition_barrier({
            .src_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .src_access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dst_stages = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dst_access = VK_ACCESS_2_MEMORY_READ_BIT,
            .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .image_id = swapchain_image
        });

        command_buffer.end();
        auto finished_command_buffer = command_buffer.get_recorded_command_buffer();
        auto acquire_semaphore = context->swapchain->get_current_acquire_semaphore();
        auto present_semaphore = context->swapchain->get_current_present_semaphore();
        auto swapchain_timeline_semaphore_info = TimelineSemaphoreInfo{
            .semaphore = context->swapchain->get_timeline_semaphore(),
            .value = context->swapchain->get_timeline_cpu_value()
        };

        context->device->submit({
            .command_buffers = {&finished_command_buffer, 1},
            .wait_binary_semaphores = {&acquire_semaphore, 1},
            .signal_binary_semaphores = {&present_semaphore, 1},
            .signal_timeline_semaphores = {&swapchain_timeline_semaphore_info, 1}
        });

        context->swapchain->present({ .wait_semaphores = {&present_semaphore, 1} });
        context->device->cleanup_resources();
    }
}