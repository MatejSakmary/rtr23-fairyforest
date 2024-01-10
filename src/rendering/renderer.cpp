#include "renderer.hpp"
#include "../shared/shared.inl"
#include <random>
namespace ff
{
    Renderer::Renderer(std::shared_ptr<Context> context)
        : context{context}
    {
        create_pipelines();
        create_resolution_indep_resources();
        create_resolution_dep_resources();
    }

    void Renderer::create_pipelines()
    {
        pipelines.prepass = RasterPipeline({RasterPipelineCreateInfo{
            .device = context->device,
            .vert_spirv_path = ".\\src\\shaders\\bin\\prepass.vert.spv",
            .frag_spirv_path = ".\\src\\shaders\\bin\\prepass.frag.spv",
            .attachments = {
                RenderAttachmentInfo{.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT},
            },
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
            .format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT,
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

        buffers.camera_info = context->device->create_buffer({
            .size = sizeof(CameraInfoBuf) * (FRAMES_IN_FLIGHT + 1),
            .name = "camera info buffer",
        });

        buffers.ssao_kernel = context->device->create_buffer({
            .size = sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT,
            .name = "SSAO kernel",
        });

        // SSAO RANDOM KERNEL
        std::mt19937 engine = std::mt19937(747474);
        std::uniform_real_distribution distribution = std::uniform_real_distribution<f32>(0.0, 1.0);
        BufferId ssao_kernel_staging = {};
        {
            DBG_ASSERT_TRUE_M(sizeof(SSAOKernel) == sizeof(f32vec3), "SSAO Kernel was changed from f32vec3 -> ssao_kernel vector needs to be updated too");
            std::vector<f32vec3> ssao_kernel = {};
            ssao_kernel.reserve(SSAO_KERNEL_SAMPLE_COUNT);
            for (i32 kernel_index = 0; kernel_index < SSAO_KERNEL_SAMPLE_COUNT; kernel_index++)
            {
                f32vec3 const random_sample = f32vec3(
                    distribution(engine) * 2.0f - 1.0f,
                    distribution(engine) * 2.0f - 1.0f,
                    distribution(engine));

                f32 const weight = static_cast<f32>(kernel_index) / static_cast<f32>(SSAO_KERNEL_SAMPLE_COUNT);
                // Push samples towards origin
                f32 const non_uniform_weight = 0.1f + (weight * weight) * 0.9f;
                f32vec3 const weighed_random_sample = non_uniform_weight * random_sample;
                ssao_kernel.emplace_back(weighed_random_sample);
            }

            ssao_kernel_staging = context->device->create_buffer({
                .size = sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT,
                .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .name = "SSAO kernel staging",
            });

            void * staging_ptr = context->device->get_buffer_host_pointer(ssao_kernel_staging);
            std::memcpy(staging_ptr, ssao_kernel.data(), sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT);
        }

        // SSAO KERNEL NOISE
        BufferId ssao_kernel_noise_staging = {};
        {
            std::vector<f32vec4> ssao_kernel_noise = {};
            ssao_kernel_noise.reserve(SSAO_KERNEL_NOISE_SIZE * SSAO_KERNEL_NOISE_SIZE);
            for (i32 noise_index = 0; noise_index < SSAO_KERNEL_NOISE_SIZE * SSAO_KERNEL_NOISE_SIZE; noise_index++)
            {
                ssao_kernel_noise.emplace_back(
                    distribution(engine) * 2.0f - 1.0f,
                    distribution(engine) * 2.0f - 1.0f,
                    distribution(engine) * 2.0f - 1.0f,
                    0.0f);
            }
            ssao_kernel_noise_staging = context->device->create_buffer({
                .size = sizeof(f32vec4) * SSAO_KERNEL_NOISE_SIZE * SSAO_KERNEL_NOISE_SIZE,
                .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .name = "SSAO kernel noise staging",
            });

            // TODO(msakmary) change this to be a buffer
            images.ssao_kernel_noise = context->device->create_image({
                .format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT,
                .extent = {SSAO_KERNEL_NOISE_SIZE, SSAO_KERNEL_NOISE_SIZE, 1},
                .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                         VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT,
                .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .name = "ssao kernel noise",
            });

            void * staging_ptr = context->device->get_buffer_host_pointer(ssao_kernel_noise_staging);
            std::memcpy(staging_ptr, ssao_kernel_noise.data(), sizeof(f32vec4) * SSAO_KERNEL_NOISE_SIZE * SSAO_KERNEL_NOISE_SIZE);
        }

        auto resource_update_command_buffer = CommandBuffer(context->device);
        resource_update_command_buffer.begin();
        // Fill kernel buffer
        {
            resource_update_command_buffer.cmd_copy_buffer_to_buffer({
                .src_buffer = ssao_kernel_staging,
                .dst_buffer = buffers.ssao_kernel,
                .size = static_cast<u32>(sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT),
            });
        }
        // Fill noise image
        {
            resource_update_command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .src_access = VK_ACCESS_2_NONE,
                .dst_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.ssao_kernel_noise,
            });
            resource_update_command_buffer.cmd_copy_buffer_to_image({
                .buffer_id = ssao_kernel_noise_staging,
                .image_id = images.ssao_kernel_noise,
                .image_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_offset = {0, 0, 0},
                .image_extent = {SSAO_KERNEL_NOISE_SIZE, SSAO_KERNEL_NOISE_SIZE, 1},
            });
            resource_update_command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .src_access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .dst_access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.ssao_kernel_noise,
            });
        }
        resource_update_command_buffer.end();
        auto recorded_command_buffer = resource_update_command_buffer.get_recorded_command_buffer();
        context->device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        context->device->destroy_buffer(ssao_kernel_staging);
        context->device->destroy_buffer(ssao_kernel_noise_staging);
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

        auto camera_info_staging_buffer = context->device->create_buffer({
            .size = sizeof(CameraInfoBuf),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "camera info staging",
        });
        void * staging_memory = context->device->get_buffer_host_pointer(camera_info_staging_buffer);
        CameraInfoBuf curr_frame_camera = {
            .view = camera_info.view,
            .inverse_view = glm::inverse(camera_info.view),
            .projection = camera_info.proj,
            .inverse_projection = glm::inverse(camera_info.proj),
            .view_projection = camera_info.viewproj,
            .inverse_view_projection = glm::inverse(camera_info.viewproj),
        };
        std::memcpy(staging_memory, &curr_frame_camera, sizeof(CameraInfoBuf));
        command_buffer.begin();
        // COPY CAMERA INFO
        {
            command_buffer.cmd_copy_buffer_to_buffer({
                .src_buffer = camera_info_staging_buffer,
                .src_offset = 0,
                .dst_buffer = buffers.camera_info,
                .dst_offset = static_cast<u32>(sizeof(CameraInfoBuf) * fif_index),
                .size = static_cast<u32>(sizeof(CameraInfoBuf)),
            });
            command_buffer.cmd_memory_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .src_access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .dst_access = VK_ACCESS_2_SHADER_READ_BIT,
            });
        }
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
                    .camera_info = context->device->get_buffer_device_address(buffers.camera_info),
                    .ss_normals_index = images.ss_normals.index,
                    .fif_index = fif_index,
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
                .camera_info = context->device->get_buffer_device_address(buffers.camera_info),
                .fif_index = fif_index,
                .ss_normals_index = images.ss_normals.index,
                .kernel_noise_index = images.ssao_kernel_noise.index,
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
                    .camera_info = context->device->get_buffer_device_address(buffers.camera_info),
                    .ss_normals_index = images.ss_normals.index,
                    .fif_index = fif_index,
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

        context->device->destroy_buffer(camera_info_staging_buffer);
        context->swapchain->present({.wait_semaphores = {&present_semaphore, 1}});
        context->device->cleanup_resources();
        u32 elapsed_time = stopwatch.elapsed_time<unsigned int, std::chrono::microseconds>();
        // fmt::println("CPU frame time {}us FPS {}", elapsed_time, 1.0f / (elapsed_time * 0.000'001f));
        frame_index += 1;
        accum += delta_time;
    }

    Renderer::~Renderer()
    {
        context->device->destroy_buffer(buffers.camera_info);
        context->device->destroy_buffer(buffers.ssao_kernel);
        context->device->destroy_image(images.ssao_kernel_noise);
        context->device->destroy_image(images.depth);
        context->device->destroy_image(images.ambient_occlusion);
        context->device->destroy_image(images.ss_normals);
        context->device->destroy_sampler(repeat_sampler);
    }
} // namespace ff