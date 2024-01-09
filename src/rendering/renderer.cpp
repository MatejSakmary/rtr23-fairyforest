#include "renderer.hpp"
#include "../shared/scene.inl"
#include "../shared/ssao.inl"
#include "../shared/particules.inl"
namespace ff
{
    Renderer::Renderer(std::shared_ptr<Context> context)
        : context{context}
    {
        create_pipelines();
        create_resolution_indep_resources();
        create_resolution_dep_resources();
        create_particules_resources(); // Loanie
    }

    void Renderer::create_pipelines()
    {
        pipelines.prepass = RasterPipeline({RasterPipelineCreateInfo{
            .device = context->device,
            .vert_spirv_path = ".\\src\\shaders\\bin\\prepass.vert.spv",
            .frag_spirv_path = ".\\src\\shaders\\bin\\prepass.frag.spv",
            .attachments = {RenderAttachmentInfo{.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT}},
            .depth_test = DepthTestInfo{
                .depth_attachment_format = VkFormat::VK_FORMAT_D32_SFLOAT,
                .enable_depth_write = 1,
                .depth_test_compare_op = VkCompareOp::VK_COMPARE_OP_GREATER,
            },
            .entry_point = "main",
            .push_constant_size = sizeof(DrawPc),
            .name = "prepass pipeline",
        }});

        pipelines.main_pass = RasterPipeline({RasterPipelineCreateInfo{
            .device = context->device,
            .vert_spirv_path = ".\\src\\shaders\\bin\\mesh_draw.vert.spv",
            .frag_spirv_path = ".\\src\\shaders\\bin\\mesh_draw.frag.spv",
            .attachments = {{.format = context->swapchain->surface_format.format}},
            .depth_test = DepthTestInfo{
                .depth_attachment_format = VkFormat::VK_FORMAT_D32_SFLOAT,
                .enable_depth_write = 0,
                .depth_test_compare_op = VkCompareOp::VK_COMPARE_OP_EQUAL,
            },
            .entry_point = "main",
            .push_constant_size = sizeof(DrawPc),
            .name = "mesh draw pipeline",
        }});

        pipelines.ssao_pass = ComputePipeline({ComputePipelineCreateInfo{
            .device = context->device,
            .comp_spirv_path = ".\\src\\shaders\\bin\\ssao.comp.spv",
            .entry_point = "main",
            .push_constant_size = sizeof(SSAOPC),
            .name = "compute test pipeline",
        }});

        // Loanie
        pipelines.particules_pass = ComputePipeline({ComputePipelineCreateInfo{
            .device = context->device,
            .comp_spirv_path = ".\\src\\shaders\\bin\\particules.comp.spv",
            .entry_point = "main",
            .push_constant_size = sizeof(ParticulesPC),
            .name = "compute particules pipeline",
        }});
    }

    void Renderer::create_resolution_dep_resources()
    {
        auto const swapchain_extent = context->swapchain->surface_extent;
        images.depth = context->device->create_image({
            .format = VkFormat::VK_FORMAT_D32_SFLOAT,
            .extent = {swapchain_extent.width, swapchain_extent.height, 1},
            .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                     VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT,
            .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
            .name = "depth buffer",
        });
        images.ss_normals = context->device->create_image({
            .format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
            .extent = {swapchain_extent.width, swapchain_extent.height, 1},
            .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT |
                     VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT |
                     VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .name = "ss normals",
        });
        images.ambient_occlusion = context->device->create_image({
            .format = VkFormat::VK_FORMAT_R16_SFLOAT,
            .extent = {swapchain_extent.width, swapchain_extent.height, 1},
            .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT |
                     VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT,
            .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .name = "ambient occlusion",
        });
    }

    void Renderer::create_resolution_indep_resources()
    {
        repeat_sampler = context->device->create_sampler({
            .address_mode_u = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .address_mode_v = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .name = "repeat sampler",
        });

        DBG_ASSERT_TRUE_M(sizeof(SSAOKernel) == sizeof(f32vec3), "SSAO Kernel was changed from f32vec3 -> ssao_kernel vector needs to be updated too");
        std::vector<f32vec3> ssao_kernel = {};
        ssao_kernel.reserve(SSAO_KERNEL_SAMPLE_COUNT);
        for (i32 kernel_index = 0; kernel_index < SSAO_KERNEL_SAMPLE_COUNT; kernel_index++)
        {
            // TODO(msakmary) Generate proper kernel samples here
            ssao_kernel.emplace_back(kernel_index);
        }

        auto resource_update_command_buffer = CommandBuffer(context->device);
        auto ssao_kernel_staging = context->device->create_buffer({
            .size = sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT,
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "SSAO kernel staging",
        });

        buffers.ssao_kernel = context->device->create_buffer({
            .size = sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT,
            .name = "SSAO kernel",
        });

        void * staging_ptr = context->device->get_buffer_host_pointer(ssao_kernel_staging);
        std::memcpy(staging_ptr, ssao_kernel.data(), sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT);
        resource_update_command_buffer.begin();
        resource_update_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = ssao_kernel_staging,
            .src_offset = 0,
            .dst_buffer = buffers.ssao_kernel,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT),
        });
        resource_update_command_buffer.end();
        auto recorded_command_buffer = resource_update_command_buffer.get_recorded_command_buffer();
        context->device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        context->device->destroy_buffer(ssao_kernel_staging);
        context->device->wait_idle();
        context->device->cleanup_resources();
    }

// Loanie
// Idk what is useless for particules from ssao
void Renderer::create_particules_resources()
    {
        DBG_ASSERT_TRUE_M(sizeof(Particule) == sizeof(f32vec3) * 2, "Particule was changed from f32vec3 -> particule_kernel vector needs to be updated too");
        std::vector<f32vec3> particule_kernel = {};
        particule_kernel.reserve(PARTICULES_COUNT);

        auto resource_update_command_buffer = CommandBuffer(context->device);

        buffers.particule_SSBO_in = context->device->create_buffer({
            .size = sizeof(Particule) * PARTICULES_COUNT,
            // something to add VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            .name = "¨Particules SSBO In",
        });

        buffers.particule_SSBO_out = context->device->create_buffer({
            .size = sizeof(Particule) * PARTICULES_COUNT,
            .name = "¨Particules SSBO Out",
        });

        resource_update_command_buffer.end();
        auto recorded_command_buffer = resource_update_command_buffer.get_recorded_command_buffer();
        context->device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        context->device->wait_idle();
        context->device->cleanup_resources();
    }

    void Renderer::resize()
    {
        context->swapchain->resize();
        context->device->destroy_image(images.depth);
        context->device->destroy_image(images.ss_normals);
        context->device->destroy_image(images.ambient_occlusion);
        auto const swapchain_extent = context->swapchain->surface_extent;
        create_resolution_dep_resources();
    }

    void Renderer::draw_frame(SceneDrawCommands const & draw_commands, CameraInfo const & camera_info, f32 delta_time)
    {
        PreciseStopwatch stopwatch = {};
        static f32 accum = 0.0f;
        auto swapchain_image = context->swapchain->acquire_next_image();
        u32 const fif_index = frame_index % (FRAMES_IN_FLIGHT + 1);

        auto command_buffer = CommandBuffer(context->device);
        auto const & swapchain_extent = context->device->info_image(swapchain_image).extent;

        command_buffer.begin();
        // swapchain_image      UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
        // ss_normals           UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
        // ambient_occlusion    UNDEFINED -> GENERAL
        // depth                UNDEFINED -> DEPTH_ATTACHMENT_OPTIMAL
        {
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
                .dst_stages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.ss_normals,
            });

            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .src_access = VK_ACCESS_2_NONE_KHR,
                .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.ambient_occlusion,
            });

            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .src_access = VK_ACCESS_2_NONE_KHR,
                .dst_stages = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dst_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
                .image_id = images.depth,
            });
        }

        // PREPASS
        {
            command_buffer.cmd_begin_renderpass({
                .color_attachments = {RenderingAttachmentInfo{
                    .image_id = images.ss_normals,
                    .layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .load_op = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                    .clear_value = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 0.0f}}},
                }},
                .depth_attachment = RenderingAttachmentInfo{
                    .image_id = images.depth,
                    .layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .load_op = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                    .clear_value = {.depthStencil = {.depth = 0.0f, .stencil = 0}},
                },
                .render_area = VkRect2D{.offset = {.x = 0, .y = 0}, .extent = {.width = swapchain_extent.width, .height = swapchain_extent.height}},
            });
            command_buffer.cmd_set_raster_pipeline(pipelines.prepass);
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
                        1.0f))});
                command_buffer.cmd_draw({
                    .vertex_count = draw_command.index_count,
                    .instance_count = draw_command.instance_count,
                });
            }
            command_buffer.cmd_end_renderpass();
        }

        // ss_normals COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        // depth      DEPTH_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        {
            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .src_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_READ_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.ss_normals,
            });

            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .src_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_READ_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
                .image_id = images.depth,
            });
        }

        // SSAO
        {
            command_buffer.cmd_set_compute_pipeline(pipelines.ssao_pass);
            command_buffer.cmd_set_push_constant(SSAOPC{
                .SSAO_kernel = context->device->get_buffer_device_address(buffers.ssao_kernel),
                .ss_normals_index = images.ss_normals.index,
                .depth_index = images.depth.index,
                .ambient_occlusion_index = images.ambient_occlusion.index,
                .extent = {swapchain_extent.width, swapchain_extent.height},
            });
            command_buffer.cmd_dispatch({
                .x = (swapchain_extent.width + SSAO_X_TILE_SIZE - 1) / SSAO_X_TILE_SIZE,
                .y = (swapchain_extent.height + SSAO_Y_TILE_SIZE - 1) / SSAO_Y_TILE_SIZE,
                .z = 1,
            });
        }

        // depth                SHADER_READ_ONLY_OPTIMAL -> DEPTH_ATTACHMENT_OPTIMAL
        // ambient_occlusion    GENERAL                  -> SHADER_READ_ONLY_OPTIMAL
        {
            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .src_access = VK_ACCESS_2_SHADER_READ_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dst_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
                .image_id = images.depth,
            });

            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .src_access = VK_ACCESS_2_SHADER_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.ambient_occlusion,
            });
        }

        // COLOR PASS
        {
            command_buffer.cmd_begin_renderpass({
                .color_attachments = {{
                    .image_id = swapchain_image,
                    .layout = VkImageLayout::VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                    .load_op = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                    .clear_value = {.color = {.float32 = {0.5f, 0.7f, 1.0f, 1.0f}}},
                }},
                .depth_attachment = RenderingAttachmentInfo{
                    .image_id = images.depth,
                    .layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .load_op = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD,
                    .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .clear_value = {.depthStencil = {.depth = 0.0f, .stencil = 0}},
                },
                .render_area = VkRect2D{.offset = {.x = 0, .y = 0}, .extent = {.width = swapchain_extent.width, .height = swapchain_extent.height}},
            });
            command_buffer.cmd_set_raster_pipeline(pipelines.main_pass);
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
                        1.0f))});
                command_buffer.cmd_draw({
                    .vertex_count = draw_command.index_count,
                    .instance_count = draw_command.instance_count,
                });
            }
            command_buffer.cmd_end_renderpass();
        }

        // swapchain COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC
        {
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
        }
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
        context->device->destroy_buffer(buffers.ssao_kernel);
        context->device->destroy_buffer(buffers.particule_SSBO_in); // Loanie
        context->device->destroy_buffer(buffers.particule_SSBO_out); // Loanie
        context->device->destroy_image(images.depth);
        context->device->destroy_image(images.ambient_occlusion);
        context->device->destroy_image(images.ss_normals);
        context->device->destroy_sampler(repeat_sampler);
    }
} // namespace ff