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
            .raster_info = RasterInfo{.face_culling = VK_CULL_MODE_BACK_BIT, .front_face_winding = VK_FRONT_FACE_COUNTER_CLOCKWISE},
            .entry_point = "main",
            .push_constant_size = sizeof(DrawPc),
            .name = "prepass pipeline",
        }});

        pipelines.prepass_discard = RasterPipeline({RasterPipelineCreateInfo{
            .device = context->device,
            .vert_spirv_path = ".\\src\\shaders\\bin\\prepass.vert.spv",
            .frag_spirv_path = ".\\src\\shaders\\bin\\prepass_discard.frag.spv",
            .attachments = {
                RenderAttachmentInfo{.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT},
            },
            .depth_test = DepthTestInfo{
                .depth_attachment_format = VkFormat::VK_FORMAT_D32_SFLOAT,
                .enable_depth_write = 1,
                .depth_test_compare_op = VkCompareOp::VK_COMPARE_OP_GREATER,
            },
            .raster_info = RasterInfo{.face_culling = VK_CULL_MODE_BACK_BIT, .front_face_winding = VK_FRONT_FACE_COUNTER_CLOCKWISE},
            .entry_point = "main",
            .push_constant_size = sizeof(DrawPc),
            .name = "prepass pipeline",
        }});

        pipelines.shadowmap_pass = RasterPipeline({RasterPipelineCreateInfo{
            .device = context->device,
            .vert_spirv_path = ".\\src\\shaders\\bin\\shadow_pass.vert.spv",
            .frag_spirv_path = ".\\src\\shaders\\bin\\shadow_pass.frag.spv",
            .depth_test = DepthTestInfo{
                .depth_attachment_format = VkFormat::VK_FORMAT_D32_SFLOAT,
                .enable_depth_write = 1,
                .depth_test_compare_op = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL,
            },
            .raster_info = RasterInfo{
                .face_culling = VK_CULL_MODE_BACK_BIT,
                .front_face_winding = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .depth_clamp_enable = true,
            },
            .entry_point = "main",
            .push_constant_size = sizeof(ShadowPC),
            .name = "shadwo pass pipeline",
        }});

        pipelines.shadowmap_pass_discard = RasterPipeline({RasterPipelineCreateInfo{
            .device = context->device,
            .vert_spirv_path = ".\\src\\shaders\\bin\\shadow_pass.vert.spv",
            .frag_spirv_path = ".\\src\\shaders\\bin\\shadow_pass_discard.frag.spv",
            .depth_test = DepthTestInfo{
                .depth_attachment_format = VkFormat::VK_FORMAT_D32_SFLOAT,
                .enable_depth_write = 1,
                .depth_test_compare_op = VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL,
            },
            .raster_info = RasterInfo{
                .face_culling = VK_CULL_MODE_BACK_BIT,
                .front_face_winding = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .depth_clamp_enable = true,
            },
            .entry_point = "main",
            .push_constant_size = sizeof(ShadowPC),
            .name = "shadwo pass pipeline",
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
            .raster_info = RasterInfo{
                .face_culling = VK_CULL_MODE_BACK_BIT,
                .front_face_winding = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
            .name = "ssao pipeline",
        }});

        pipelines.first_depth_pass = ComputePipeline({ComputePipelineCreateInfo{
            .device = context->device,
            .comp_spirv_path = ".\\src\\shaders\\bin\\minmax_first_pass.comp.spv",
            .entry_point = "main",
            .push_constant_size = sizeof(AnalyzeDepthPC),
            .name = "first depth pass pipeline",
        }});

        pipelines.subseq_depth_pass = ComputePipeline({ComputePipelineCreateInfo{
            .device = context->device,
            .comp_spirv_path = ".\\src\\shaders\\bin\\minmax_subseq_pass.comp.spv",
            .entry_point = "main",
            .push_constant_size = sizeof(AnalyzeDepthPC),
            .name = "subsequent depth pass pipeline",
        }});

        pipelines.write_shadow_matrices = ComputePipeline({ComputePipelineCreateInfo{
            .device = context->device,
            .comp_spirv_path = ".\\src\\shaders\\bin\\write_shadow_matrices.comp.spv",
            .entry_point = "main",
            .push_constant_size = sizeof(WriteShadowMatricesPC),
            .name = "write shadow matrices pipeline",
        }});

        pipelines.first_esm_pass = ComputePipeline({ComputePipelineCreateInfo{
            .device = context->device,
            .comp_spirv_path = ".\\src\\shaders\\bin\\esm_first_pass.comp.spv",
            .entry_point = "main",
            .push_constant_size = sizeof(ESMShadowPC),
            .name = "first esm pass pipeline",
        }});

        pipelines.second_esm_pass = ComputePipeline({ComputePipelineCreateInfo{
            .device = context->device,
            .comp_spirv_path = ".\\src\\shaders\\bin\\esm_second_pass.comp.spv",
            .entry_point = "main",
            .push_constant_size = sizeof(ESMShadowPC),
            .name = "second esm pass pipeline",
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
            .format = VkFormat::VK_FORMAT_R16_UINT,
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

        u32vec2 limits_size;
        u32vec2 wg_size = DEPTH_PASS_WG_READS_PER_AXIS;
        limits_size.x = (swapchain_extent.width + wg_size.x - 1) / wg_size.x;
        limits_size.y = (swapchain_extent.height + wg_size.y - 1) / wg_size.y;
        buffers.depth_limits = context->device->create_buffer({
            .size = static_cast<u32>(sizeof(DepthLimits) * limits_size.x * limits_size.y),
            .name = "depth limits",
        });
    }

    void Renderer::create_resolution_indep_resources()
    {
        repeat_sampler = context->device->create_sampler({
            .address_mode_u = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .address_mode_v = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mip_lod_bias = -1.0f,
            .enable_anisotropy = true,
            .max_anisotropy = 16.0f,
            .name = "repeat sampler",
        });

        clamp_sampler = context->device->create_sampler({
            .address_mode_u = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_mode_v = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .name = "linear sampler",
        });

        no_mip_sampler = context->device->create_sampler({
            .mip_lod_bias = 0.0,
            .name = "no mip sampler",
        });

        buffers.camera_info = context->device->create_buffer({
            .size = sizeof(CameraInfoBuf) * (FRAMES_IN_FLIGHT + 1),
            .name = "camera info buffer",
        });

        buffers.ssao_kernel = context->device->create_buffer({
            .size = sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT,
            .name = "SSAO kernel",
        });

        buffers.cascade_data = context->device->create_buffer({
            .size = sizeof(ShadowmapCascadeData) * NUM_CASCADES,
            .name = "shadowmap cascade data",
        });

        // Shadowmap textures
        DBG_ASSERT_TRUE_M(NUM_CASCADES <= 8, "[ERROR][Renderer::create_resolution_indep_resources()] More than 8 cascades not supported");
        auto const resolution_multiplier = resolution_table[NUM_CASCADES - 1];
        images.shadowmap_cascades = context->device->create_image({
            .format = VkFormat::VK_FORMAT_D32_SFLOAT,
            .extent = {
                SHADOWMAP_RESOLUTION * resolution_multiplier.x,
                SHADOWMAP_RESOLUTION * resolution_multiplier.y, 1},
            .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT,
            .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
            .name = "shadowmap",
        });

        images.esm_cascades = context->device->create_image({
            .format = VkFormat::VK_FORMAT_R16_UNORM,
            .extent = {SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, 1},
            .array_layer_count = NUM_CASCADES,
            .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT |
                     VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT,
            .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .name = "esm shadowmap",
        });

        images.esm_tmp_cascades = context->device->create_image({
            .format = VkFormat::VK_FORMAT_R16_UNORM,
            .extent = {SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, 1},
            .array_layer_count = NUM_CASCADES,
            .usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT |
                     VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT,
            .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .name = "esm tmp shadowmap",
        });

        // SSAO RANDOM KERNEL
        std::mt19937 engine = std::mt19937(747474);
        std::uniform_real_distribution distribution = std::uniform_real_distribution<f32>(0.0, 1.0);
        std::uniform_real_distribution distribution_z = std::uniform_real_distribution<f32>(0.2, 1.0);
        BufferId ssao_kernel_staging = {};
        {
            DBG_ASSERT_TRUE_M(sizeof(SSAOKernel) == sizeof(f32vec3), "SSAO Kernel was changed from f32vec3 -> ssao_kernel vector needs to be updated too");
            std::vector<f32vec3> ssao_kernel = {};
            ssao_kernel.reserve(SSAO_KERNEL_SAMPLE_COUNT);
            auto lerp = [](f32 a, f32 b, f32 f) -> f32
            { return a + f * (b - a); };
            for (i32 kernel_index = 0; kernel_index < SSAO_KERNEL_SAMPLE_COUNT; kernel_index++)
            {
                f32vec3 const random_sample = f32vec3(
                    distribution(engine) * 2.0f - 1.0f,
                    distribution(engine) * 2.0f - 1.0f,
                    distribution_z(engine));

                f32 const weight = static_cast<f32>(kernel_index) / static_cast<f32>(SSAO_KERNEL_SAMPLE_COUNT);
                // Push samples towards origin
                f32 const non_uniform_weight = lerp(0.1, 1.0, weight * weight);
                f32vec3 const weighed_random_sample = non_uniform_weight * random_sample;
                ssao_kernel.emplace_back(weighed_random_sample);
            }

            ssao_kernel_staging = context->device->create_buffer({
                .size = sizeof(SSAOKernel) * SSAO_KERNEL_SAMPLE_COUNT,
                .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
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
        f32vec3 const sun_direction = glm::normalize(f32vec3(
            std::cos(f32(20.0f) / 5.0f),
            std::sin(f32(20.0f) / 5.0f),
            1.0f));
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
            .position = camera_info.pos,
            .frust_right_offset = camera_info.frust_right_offset,
            .frust_top_offset = camera_info.frust_top_offset,
            .frust_front = camera_info.frust_front,
            .view = camera_info.view,
            .inverse_view = glm::inverse(camera_info.view),
            .projection = camera_info.proj,
            .inverse_projection = glm::inverse(camera_info.proj),
            .view_projection = camera_info.viewproj,
            .inverse_view_projection = glm::inverse(camera_info.viewproj),
        };
        std::memcpy(staging_memory, &curr_frame_camera, sizeof(CameraInfoBuf));

        auto record_mesh_draw_commands = [&](auto const & commands)
        {
            for (auto const & draw_command : commands)
            {
                command_buffer.cmd_set_push_constant(DrawPc{
                    .scene_descriptor = draw_commands.scene_descriptor,
                    .camera_info = context->device->get_buffer_device_address(buffers.camera_info),
                    .ss_normals_index = images.ss_normals.index,
                    .fif_index = fif_index,
                    .mesh_index = draw_command.mesh_idx,
                    .sampler_id = repeat_sampler.index,
                });

                command_buffer.cmd_draw_indexed({
                    .index_count = draw_command.index_count,
                    .instance_count = draw_command.instance_count,
                    .first_index = draw_command.index_offset,
                    .vertex_offset = 0,
                    .first_instance = 0,
                });
            }
        };
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
        // shadowmap_cascades   UNDEFINED -> DEPTH_ATTACHMENT_OPTIMAL
        // esm_shadowmap        UNDEFINED -> GENERAL
        // esm_cascades         UNDEFINED -> GENERAL
        // esm_tmp_cascades     UNDEFINED -> GENERAL
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

            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .src_access = VK_ACCESS_2_NONE_KHR,
                .dst_stages = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dst_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
                .image_id = images.shadowmap_cascades,
            });

            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .src_access = VK_ACCESS_2_NONE_KHR,
                .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                .layer_count = NUM_CASCADES,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.esm_cascades,
            });

            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .src_access = VK_ACCESS_2_NONE_KHR,
                .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                .layer_count = NUM_CASCADES,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.esm_tmp_cascades,
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
            command_buffer.cmd_set_index_buffer({
                .buffer_id = draw_commands.index_buffer_id,
                .offset = 0,
                .index_type = VkIndexType::VK_INDEX_TYPE_UINT32,
            });
            record_mesh_draw_commands(draw_commands.draw_commands);
            command_buffer.cmd_set_raster_pipeline(pipelines.prepass_discard);
            record_mesh_draw_commands(draw_commands.alpha_discard_commands);
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

        // Depth passes
        {
            u32vec2 const first_pass_dispatch_size = u32vec2{
                (swapchain_extent.width + DEPTH_PASS_WG_READS_PER_AXIS.x - 1) / DEPTH_PASS_WG_READS_PER_AXIS.x,
                (swapchain_extent.height + DEPTH_PASS_WG_READS_PER_AXIS.y - 1) / DEPTH_PASS_WG_READS_PER_AXIS.y};

            command_buffer.cmd_set_push_constant(AnalyzeDepthPC{
                .depth_limits = context->device->get_buffer_device_address(buffers.depth_limits),
                .depth_dimensions = {swapchain_extent.width, swapchain_extent.height},
                .sampler_id = clamp_sampler.index,
                .prev_thread_count = 0, // Unused in the first pass
                .depth_index = images.depth.index,
            });
            command_buffer.cmd_set_compute_pipeline(pipelines.first_depth_pass);
            command_buffer.cmd_dispatch({first_pass_dispatch_size.x, first_pass_dispatch_size.y, 1});

            u32 const threads_in_block = DEPTH_PASS_WG_SIZE.x * DEPTH_PASS_WG_SIZE.y;
            u32 const wg_total_reads = threads_in_block * DEPTH_PASS_THREAD_READ_COUNT.x * DEPTH_PASS_THREAD_READ_COUNT.y;
            u32 prev_pass_num_threads = first_pass_dispatch_size.x * first_pass_dispatch_size.y;
            u32 second_pass_dispatch_size = 0;
            while (second_pass_dispatch_size != 1)
            {
                second_pass_dispatch_size = (prev_pass_num_threads + wg_total_reads - 1) / wg_total_reads;
                command_buffer.cmd_memory_barrier({
                    .src_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .src_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
                    .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                    .dst_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
                });
                command_buffer.cmd_set_push_constant(AnalyzeDepthPC{
                    .depth_limits = context->device->get_buffer_device_address(buffers.depth_limits),
                    .depth_dimensions = {swapchain_extent.width, swapchain_extent.height},
                    .sampler_id = clamp_sampler.index,
                    .prev_thread_count = prev_pass_num_threads,
                    .depth_index = images.depth.index});
                command_buffer.cmd_set_compute_pipeline(pipelines.subseq_depth_pass);
                command_buffer.cmd_dispatch({second_pass_dispatch_size, 1, 1});
            }
        }

        // Shadowmap matrices
        {
            command_buffer.cmd_memory_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .src_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
            });
            command_buffer.cmd_set_push_constant(WriteShadowMatricesPC{
                .camera_info = context->device->get_buffer_device_address(buffers.camera_info),
                .fif_index = fif_index,
                .depth_limits = context->device->get_buffer_device_address(buffers.depth_limits),
                .cascade_data = context->device->get_buffer_device_address(buffers.cascade_data),
                .sun_direction = sun_direction,
            });
            command_buffer.cmd_set_compute_pipeline(pipelines.write_shadow_matrices);
            command_buffer.cmd_dispatch({1, 1, 1});
        }

        // Draw shadows
        {
            command_buffer.cmd_memory_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .src_access = VK_ACCESS_2_SHADER_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_READ_BIT,
            });
            auto const resolution_multiplier = resolution_table[NUM_CASCADES - 1];

            command_buffer.cmd_begin_renderpass({
                .depth_attachment = RenderingAttachmentInfo{
                    .image_id = images.shadowmap_cascades,
                    .layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .load_op = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .store_op = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                    .clear_value = {.depthStencil = {.depth = 1.0f, .stencil = 0}},
                },
                .render_area = VkRect2D{
                    .offset = {.x = 0, .y = 0},
                    .extent = {
                        .width = SHADOWMAP_RESOLUTION * resolution_multiplier.x,
                        .height = SHADOWMAP_RESOLUTION * resolution_multiplier.y,
                    },
                },
            });
            command_buffer.cmd_set_raster_pipeline(pipelines.shadowmap_pass);
            command_buffer.cmd_set_index_buffer({
                .buffer_id = draw_commands.index_buffer_id,
                .offset = 0,
                .index_type = VkIndexType::VK_INDEX_TYPE_UINT32,
            });
            for (u32 cascade = 0; cascade < NUM_CASCADES; cascade++)
            {
                u32vec2 offset;
                offset.x = cascade % resolution_multiplier.x;
                offset.y = cascade / resolution_multiplier.x;
                command_buffer.cmd_set_viewport({
                    .x = static_cast<f32>(offset.x * SHADOWMAP_RESOLUTION),
                    .y = static_cast<f32>(offset.y * SHADOWMAP_RESOLUTION),
                    .width = SHADOWMAP_RESOLUTION,
                    .height = SHADOWMAP_RESOLUTION,
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                });
                for (auto const & draw_command : draw_commands.draw_commands)
                {
                    command_buffer.cmd_set_push_constant(ShadowPC{
                        .scene_descriptor = draw_commands.scene_descriptor,
                        .cascade_data = context->device->get_buffer_device_address(buffers.cascade_data),
                        .mesh_index = draw_command.mesh_idx,
                        .sampler_id = no_mip_sampler.index,
                        .cascade_index = cascade,
                    });
                    command_buffer.cmd_draw_indexed({
                        .index_count = draw_command.index_count,
                        .instance_count = draw_command.instance_count,
                        .first_index = draw_command.index_offset,
                        .vertex_offset = 0,
                        .first_instance = 0,
                    });
                }
            }

            command_buffer.cmd_set_raster_pipeline(pipelines.shadowmap_pass_discard);
            for (u32 cascade = 0; cascade < NUM_CASCADES; cascade++)
            {
                u32vec2 offset;
                offset.x = cascade % resolution_multiplier.x;
                offset.y = cascade / resolution_multiplier.x;
                command_buffer.cmd_set_viewport({
                    .x = static_cast<f32>(offset.x * SHADOWMAP_RESOLUTION),
                    .y = static_cast<f32>(offset.y * SHADOWMAP_RESOLUTION),
                    .width = SHADOWMAP_RESOLUTION,
                    .height = SHADOWMAP_RESOLUTION,
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                });
                for (auto const & draw_command : draw_commands.alpha_discard_commands)
                {
                    command_buffer.cmd_set_push_constant(ShadowPC{
                        .scene_descriptor = draw_commands.scene_descriptor,
                        .cascade_data = context->device->get_buffer_device_address(buffers.cascade_data),
                        .mesh_index = draw_command.mesh_idx,
                        .sampler_id = no_mip_sampler.index,
                        .cascade_index = cascade,
                    });
                    command_buffer.cmd_draw_indexed({
                        .index_count = draw_command.index_count,
                        .instance_count = draw_command.instance_count,
                        .first_index = draw_command.index_offset,
                        .vertex_offset = 0,
                        .first_instance = 0,
                    });
                }
            }
            command_buffer.cmd_end_renderpass();
        }

        // shadowmap_cascades DEPTH_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        {
            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .src_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_READ_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
                .image_id = images.shadowmap_cascades,
            });
        }

        // ESM blur first pass
        {
            auto const resolution_multiplier = resolution_table[NUM_CASCADES - 1];
            command_buffer.cmd_set_compute_pipeline(pipelines.first_esm_pass);
            for (u32 cascade = 0; cascade < NUM_CASCADES; cascade++)
            {
                u32vec2 const offset = {
                    cascade % resolution_multiplier.x,
                    cascade / resolution_multiplier.x,
                };
                command_buffer.cmd_set_push_constant(ESMShadowPC{
                    .tmp_esm_index = images.esm_tmp_cascades.index,
                    .esm_index = images.esm_cascades.index,
                    .shadowmap_index = images.shadowmap_cascades.index,
                    .cascade_index = cascade,
                    .offset = {
                        offset.x * SHADOWMAP_RESOLUTION,
                        offset.y * SHADOWMAP_RESOLUTION,
                    },
                });
                command_buffer.cmd_dispatch({SHADOWMAP_RESOLUTION / ESM_BLUR_WORKGROUP_SIZE, SHADOWMAP_RESOLUTION, 1});
            }
        }
        // esm_tmp_cascades     GENERAL -> SHADER_READ_ONLY_OPTIMAL
        {
            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .src_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .layer_count = NUM_CASCADES,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.esm_tmp_cascades,
            });
        }

        // ESM blur second pass
        {
            command_buffer.cmd_set_compute_pipeline(pipelines.second_esm_pass);
            for (u32 cascade = 0; cascade < NUM_CASCADES; cascade++)
            {
                command_buffer.cmd_set_push_constant(ESMShadowPC{
                    .tmp_esm_index = images.esm_tmp_cascades.index,
                    .esm_index = images.esm_cascades.index,
                    .shadowmap_index = images.shadowmap_cascades.index,
                    .cascade_index = cascade,
                    .offset = {},
                });
                command_buffer.cmd_dispatch({SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION / ESM_BLUR_WORKGROUP_SIZE, 1});
            }
        }

        // depth                SHADER_READ_ONLY_OPTIMAL -> DEPTH_ATTACHMENT_OPTIMAL
        // ambient_occlusion    GENERAL                  -> SHADER_READ_ONLY_OPTIMAL
        // esm_cascades         GENERAL                  -> SHADER_READ_ONLY_OPTIMAL
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
                .dst_access = VK_ACCESS_2_SHADER_READ_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.ambient_occlusion,
            });

            command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                .src_access = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dst_access = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .layer_count = NUM_CASCADES,
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = images.esm_cascades,
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
                    .clear_value = {.color = {.float32 = {SKY_COLOR.x, SKY_COLOR.y, SKY_COLOR.z, 1.0f}}},
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
            command_buffer.cmd_set_index_buffer({
                .buffer_id = draw_commands.index_buffer_id,
                .offset = 0,
                .index_type = VkIndexType::VK_INDEX_TYPE_UINT32,
            });
            for (auto const & draw_command : draw_commands.draw_commands)
            {
                command_buffer.cmd_set_push_constant(DrawPc{
                    .scene_descriptor = draw_commands.scene_descriptor,
                    .camera_info = context->device->get_buffer_device_address(buffers.camera_info),
                    .cascade_data = context->device->get_buffer_device_address(buffers.cascade_data),
                    .ss_normals_index = images.ss_normals.index,
                    .ssao_index = images.ambient_occlusion.index,
                    .esm_shadowmap_index = images.esm_cascades.index,
                    .fif_index = fif_index,
                    .mesh_index = draw_command.mesh_idx,
                    .sampler_id = repeat_sampler.index,
                    .shadow_sampler_id = clamp_sampler.index,
                    .sun_direction = sun_direction,
                    .enable_ao = static_cast<u32>(draw_commands.enable_ao),
                });
                command_buffer.cmd_draw_indexed({
                    .index_count = draw_command.index_count,
                    .instance_count = draw_command.instance_count,
                    .first_index = draw_command.index_offset,
                    .vertex_offset = 0,
                    .first_instance = 0,
                });
            }
            for (auto const & draw_command : draw_commands.alpha_discard_commands)
            {
                command_buffer.cmd_set_push_constant(DrawPc{
                    .scene_descriptor = draw_commands.scene_descriptor,
                    .camera_info = context->device->get_buffer_device_address(buffers.camera_info),
                    .cascade_data = context->device->get_buffer_device_address(buffers.cascade_data),
                    .ss_normals_index = images.ss_normals.index,
                    .ssao_index = images.ambient_occlusion.index,
                    .esm_shadowmap_index = images.esm_cascades.index,
                    .fif_index = fif_index,
                    .mesh_index = draw_command.mesh_idx,
                    .sampler_id = repeat_sampler.index,
                    .shadow_sampler_id = clamp_sampler.index,
                    .sun_direction = sun_direction,
                    .enable_ao = static_cast<u32>(draw_commands.enable_ao),
                });
                command_buffer.cmd_draw_indexed({
                    .index_count = draw_command.index_count,
                    .instance_count = draw_command.instance_count,
                    .first_index = draw_command.index_offset,
                    .vertex_offset = 0,
                    .first_instance = 0,
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
        context->device->destroy_buffer(buffers.cascade_data);
        context->device->destroy_buffer(buffers.depth_limits);
        context->device->destroy_image(images.ssao_kernel_noise);
        context->device->destroy_image(images.depth);
        context->device->destroy_image(images.ambient_occlusion);
        context->device->destroy_image(images.ss_normals);
        context->device->destroy_image(images.esm_cascades);
        context->device->destroy_image(images.esm_tmp_cascades);
        context->device->destroy_image(images.shadowmap_cascades);
        context->device->destroy_sampler(repeat_sampler);
        context->device->destroy_sampler(clamp_sampler);
        context->device->destroy_sampler(no_mip_sampler);
    }
} // namespace ff