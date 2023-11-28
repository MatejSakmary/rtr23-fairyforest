#include "renderer.hpp"
#include "../shared/shared.inl"
namespace ff
{
    Renderer::Renderer(std::shared_ptr<Context> context)
        : context{context},
          triangle_pipeline({PipelineCreateInfo{
              .device = context->device,
              .vert_spirv_path = ".\\src\\shaders\\bin\\mesh_draw.vert.spv",
              .frag_spirv_path = ".\\src\\shaders\\bin\\mesh_draw.frag.spv",
              .attachments = {{.format = context->swapchain->surface_format.format}},
              .depth_test = DepthTestInfo{
                  .depth_attachment_format = VkFormat::VK_FORMAT_D32_SFLOAT,
                  .enable_depth_write = 1,
                  .depth_test_compare_op = VkCompareOp::VK_COMPARE_OP_GREATER,
              },
              .entry_point = "main",
              .push_constant_size = sizeof(DrawPc),
              .name = "mesh draw pipeline"}})
    {
        auto const swapchain_extent = context->swapchain->surface_extent;
        depth_buffer = context->device->create_image({
            .format = VkFormat::VK_FORMAT_D32_SFLOAT,
            .extent = {swapchain_extent.width, swapchain_extent.height, 1},
            .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
            .name = "depth buffer",
        });
        repeat_sampler = context->device->create_sampler({
            .address_mode_u = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .address_mode_v = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .name = "repeat sampler",
        });
    }

    void Renderer::resize()
    {
        context->swapchain->resize();
        context->device->destroy_image(depth_buffer);
        auto const swapchain_extent = context->swapchain->surface_extent;
        depth_buffer = context->device->create_image({
            .format = VkFormat::VK_FORMAT_D32_SFLOAT,
            .extent = {swapchain_extent.width, swapchain_extent.height, 1},
            .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
            .name = "depth buffer",
        });
    }

    void Renderer::draw_frame(SceneDrawCommands const & draw_commands, CameraInfo const & camera_info, f32 delta_time)
    {
        PreciseStopwatch stopwatch = {};
        static f32 accum = 0.0f;
        auto swapchain_image = context->swapchain->acquire_next_image();
        u32 const fif_index = frame_index % (FRAMES_IN_FLIGHT + 1);

        auto command_buffer = CommandBuffer(context->device);
        command_buffer.begin();

        command_buffer.cmd_image_memory_transition_barrier({
            .src_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .src_access = VK_ACCESS_2_NONE_KHR,
            .dst_stages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
            .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .image_id = swapchain_image,
        });

        command_buffer.cmd_image_memory_transition_barrier({
            .src_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .src_access = VK_ACCESS_2_NONE_KHR,
            .dst_stages = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dst_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
            .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
            .image_id = depth_buffer,
        });

        auto const & swapchain_extent = context->device->info_image(swapchain_image).extent;

        command_buffer.cmd_begin_renderpass({
            .color_attachments = {{
                .image_id = swapchain_image,
                .layout = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                .load_op = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                .clear_value = {.color = {.float32 = {0.002f, 0.002f, 0.002f, 1.0f}}},
            }},
            .depth_attachment = RenderingAttachmentInfo{
                .image_id = depth_buffer,
                .layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .load_op = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clear_value = {.depthStencil = {.depth = 0.0f, .stencil = 0}},
            },
            .render_area = VkRect2D{.offset = {.x = 0, .y = 0}, .extent = {.width = swapchain_extent.width, .height = swapchain_extent.height}},
        });
        command_buffer.cmd_set_raster_pipeline(triangle_pipeline);
        for (auto const & draw_command : draw_commands.draw_commands)
        {
            command_buffer.cmd_set_push_constant(DrawPc{
                .scene_descriptor = draw_commands.scene_descriptor,
                .view_proj = camera_info.viewproj,
                .mesh_index = draw_command.mesh_idx,
                .sampler_id = repeat_sampler.index,
                .sun_direction = glm::normalize(f32vec3(
                    std::cos(f32(accum) / 5.0f),
                    std::sin(f32(accum) / 5.0f),
                    1.0f))
            });
            command_buffer.cmd_draw({
                .vertex_count = draw_command.index_count,
                .instance_count = draw_command.instance_count,
            });
        }
        command_buffer.cmd_end_renderpass();

        command_buffer.cmd_image_memory_transition_barrier({
            .src_stages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .src_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dst_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .dst_access = VK_ACCESS_2_MEMORY_READ_BIT,
            .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .image_id = swapchain_image,
        });
        command_buffer.end();

        auto finished_command_buffer = command_buffer.get_recorded_command_buffer();
        auto acquire_semaphore = context->swapchain->get_current_acquire_semaphore();
        auto present_semaphore = context->swapchain->get_current_present_semaphore();
        auto swapchain_timeline_semaphore_info = TimelineSemaphoreInfo{
            .semaphore = context->swapchain->get_timeline_semaphore(),
            .value = context->swapchain->get_timeline_cpu_value(),
        };

        context->device->submit({
            .command_buffers = {&finished_command_buffer, 1},
            .wait_binary_semaphores = {&acquire_semaphore, 1},
            .signal_binary_semaphores = {&present_semaphore, 1},
            .signal_timeline_semaphores = {&swapchain_timeline_semaphore_info, 1},
        });

        context->swapchain->present({.wait_semaphores = {&present_semaphore, 1}});
        context->device->cleanup_resources();
        u32 elapsed_time = stopwatch.elapsed_time<unsigned int, std::chrono::microseconds>();
        // fmt::println("CPU frame time {}us FPS {}", elapsed_time, 1.0f / (elapsed_time * 0.000'001f));
        frame_index += 1;
        accum += delta_time;
    }

    Renderer::~Renderer()
    {
        context->device->destroy_image(depth_buffer);
        context->device->destroy_sampler(repeat_sampler);
    }
} // namespace ff