#include "pipeline.hpp"
#include <fstream>

namespace ff
{
    RasterPipeline::RasterPipeline(RasterPipelineCreateInfo const & info) : device{info.device}
    {
        std::vector<VkShaderModule> shader_modules = {};
        std::vector<std::string> entry_point_names = {};
        // COPE so we don't dangle when pushing into
        entry_point_names.reserve(10);
        std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos = {};

        auto create_shader_module = [&](std::vector<u32> const & bytecode, std::string entry_point, VkShaderStageFlagBits shader_stage)
        {
            VkShaderModule shader_module = nullptr;
            VkShaderModuleCreateInfo const shader_module_create_info{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .codeSize = static_cast<u32>(bytecode.size() * sizeof(u32)),
                .pCode = bytecode.data(),
            };
            CHECK_VK_RESULT(vkCreateShaderModule(device->vulkan_device, &shader_module_create_info, nullptr, &shader_module));
            shader_modules.push_back(shader_module);
            entry_point_names.push_back(entry_point);
            VkPipelineShaderStageCreateInfo const vk_pipeline_shader_stage_create_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .stage = shader_stage,
                .module = shader_module,
                .pName = entry_point_names.back().c_str(),
                .pSpecializationInfo = nullptr,
            };
            pipeline_shader_stage_create_infos.push_back(vk_pipeline_shader_stage_create_info);
        };

        auto read_spirv_from_file = [](std::filesystem::path filepath) -> std::vector<u32>
        {
            std::ifstream ifs{filepath, std::ios::binary};
            if (!ifs)
            {
                BACKEND_LOG(fmt::format("[ERROR][Pipeline::Pipeline()] Shader spirv path invalid {}", filepath.string()));
                throw std::runtime_error("[ERROR][Pipeline::Pipeline()] Shader spirv path invalid");
            }
            ifs.seekg(0, ifs.end);
            const i32 filesize = ifs.tellg();
            ifs.seekg(0, ifs.beg);
            std::vector<u32> raw(filesize / 4);
            if (!ifs.read(reinterpret_cast<char *>(raw.data()), filesize))
            {
                BACKEND_LOG(fmt::format("[ERROR][Pipeline::Pipeline()] Failed to read entire shader spirv"));
                throw std::runtime_error("[ERROR][Pipeline::Pipeline()] Failed to read entire shader spirv");
            }
            return raw;
        };

        if (info.vert_spirv_path.has_value())
        {
            auto const spirv = read_spirv_from_file(info.vert_spirv_path.value());
            create_shader_module(spirv, info.entry_point, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
        }
        if (info.frag_spirv_path.has_value())
        {
            auto const spirv = read_spirv_from_file(info.frag_spirv_path.value());
            create_shader_module(spirv, info.entry_point, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        layout = device->resource_table->pipeline_layouts.at((info.push_constant_size + 3) / 4);
        constexpr VkPipelineVertexInputStateCreateInfo vertex_input_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
        };

        VkPipelineInputAssemblyStateCreateInfo const input_assembly_state{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .topology = info.raster_info.primitive_topology,
            .primitiveRestartEnable = static_cast<VkBool32>(info.raster_info.primitive_restart_enable),
        };

        constexpr VkPipelineMultisampleStateCreateInfo multisample_state{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = {},
            .alphaToCoverageEnable = {},
            .alphaToOneEnable = {},
        };

        VkPipelineRasterizationStateCreateInfo vk_raster_state{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .depthClampEnable = info.raster_info.depth_clamp_enable,
            .rasterizerDiscardEnable = static_cast<VkBool32>(info.raster_info.rasterizer_discard_enable),
            .polygonMode = info.raster_info.polygon_mode,
            .cullMode = info.raster_info.face_culling,
            .frontFace = info.raster_info.front_face_winding,
            .depthBiasEnable = static_cast<VkBool32>(info.raster_info.depth_bias_enable),
            .depthBiasConstantFactor = info.raster_info.depth_bias_constant_factor,
            .depthBiasClamp = info.raster_info.depth_bias_clamp,
            .depthBiasSlopeFactor = info.raster_info.depth_bias_slope_factor,
            .lineWidth = info.raster_info.line_width,
        };
        DepthTestInfo no_depth = {};
        VkPipelineDepthStencilStateCreateInfo const depth_stencil_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .depthTestEnable = static_cast<VkBool32>(info.depth_test.has_value()),
            .depthWriteEnable = static_cast<VkBool32>(info.depth_test.value_or(no_depth).enable_depth_write),
            .depthCompareOp = info.depth_test.value_or(no_depth).depth_test_compare_op,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {},
            .back = {},
            .minDepthBounds = info.depth_test.value_or(no_depth).min_depth_bounds,
            .maxDepthBounds = info.depth_test.value_or(no_depth).max_depth_bounds,
        };

        std::vector<VkPipelineColorBlendAttachmentState> pipeline_color_blend_attachment_blend_states = {};
        auto no_blend = BlendInfo{};
        for (u32 attachment_index = 0; attachment_index < info.attachments.size(); ++attachment_index)
        {
            pipeline_color_blend_attachment_blend_states.push_back({
                .blendEnable = static_cast<VkBool32>(info.attachments.at(attachment_index).blend_info.has_value()),
                .srcColorBlendFactor = info.attachments.at(attachment_index).blend_info.value_or(no_blend).src_color_blend_factor,
                .dstColorBlendFactor = info.attachments.at(attachment_index).blend_info.value_or(no_blend).dst_color_blend_factor,
                .colorBlendOp = info.attachments.at(attachment_index).blend_info.value_or(no_blend).color_blend_op,
                .srcAlphaBlendFactor = info.attachments.at(attachment_index).blend_info.value_or(no_blend).src_alpha_blend_factor,
                .dstAlphaBlendFactor = info.attachments.at(attachment_index).blend_info.value_or(no_blend).dst_alpha_blend_factor,
                .alphaBlendOp = info.attachments.at(attachment_index).blend_info.value_or(no_blend).alpha_blend_op,
                .colorWriteMask = info.attachments.at(attachment_index).blend_info.value_or(no_blend).color_write_mask,
            });
        }
        std::vector<VkFormat> pipeline_color_attachment_formats = {};
        for (u32 attachment_index = 0; attachment_index < info.attachments.size(); ++attachment_index)
        {
            pipeline_color_attachment_formats.push_back(info.attachments.at(attachment_index).format);
        }
        VkPipelineColorBlendStateCreateInfo const color_blend_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .logicOpEnable = VK_FALSE,
            .logicOp = {},
            .attachmentCount = static_cast<u32>(info.attachments.size()),
            .pAttachments = pipeline_color_blend_attachment_blend_states.data(),
            .blendConstants = {1.0f, 1.0f, 1.0f, 1.0f},
        };
        constexpr VkViewport DEFAULT_VIEWPORT{.x = 0, .y = 0, .width = 1, .height = 1, .minDepth = 0, .maxDepth = 0};
        constexpr VkRect2D DEFAULT_SCISSOR{.offset = {0, 0}, .extent = {1, 1}};
        VkPipelineViewportStateCreateInfo const viewport_state{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .viewportCount = 1,
            .pViewports = &DEFAULT_VIEWPORT,
            .scissorCount = 1,
            .pScissors = &DEFAULT_SCISSOR,
        };
        auto dynamic_states = std::array{
            VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
            VkDynamicState::VK_DYNAMIC_STATE_SCISSOR,
            VkDynamicState::VK_DYNAMIC_STATE_DEPTH_BIAS,
        };
        VkPipelineDynamicStateCreateInfo const dynamic_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
            .pDynamicStates = dynamic_states.data(),
        };
        VkPipelineRenderingCreateInfo pipeline_rendering = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = nullptr,
            .viewMask = {},
            .colorAttachmentCount = static_cast<u32>(info.attachments.size()),
            .pColorAttachmentFormats = pipeline_color_attachment_formats.data(),
            .depthAttachmentFormat = info.depth_test.value_or(no_depth).depth_attachment_format,
            .stencilAttachmentFormat = {},
        };
        VkGraphicsPipelineCreateInfo const graphics_pipeline_create_info{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &pipeline_rendering,
            .flags = {},
            .stageCount = static_cast<u32>(pipeline_shader_stage_create_infos.size()),
            .pStages = pipeline_shader_stage_create_infos.data(),
            .pVertexInputState = &vertex_input_state,
            .pInputAssemblyState = &input_assembly_state,
            .pTessellationState = nullptr,
            .pViewportState = &viewport_state,
            .pRasterizationState = &vk_raster_state,
            .pMultisampleState = &multisample_state,
            .pDepthStencilState = &depth_stencil_state,
            .pColorBlendState = &color_blend_state,
            .pDynamicState = &dynamic_state,
            .layout = layout,
            .renderPass = nullptr,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
        };

        CHECK_VK_RESULT(vkCreateGraphicsPipelines(device->vulkan_device, VK_NULL_HANDLE, 1u, &graphics_pipeline_create_info, nullptr, &pipeline));
        for (auto & shader_module : shader_modules)
        {
            vkDestroyShaderModule(device->vulkan_device, shader_module, nullptr);
        }
        {
            VkDebugUtilsObjectNameInfoEXT const name_info{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .pNext = nullptr,
                .objectType = VK_OBJECT_TYPE_PIPELINE,
                .objectHandle = std::bit_cast<u64>(pipeline),
                .pObjectName = info.name.c_str(),
            };
            CHECK_VK_RESULT(device->vkSetDebugUtilsObjectNameEXT(device->vulkan_device, &name_info));
        }
        BACKEND_LOG(fmt::format("[INFO][Pipeline::Pipeline()] Pipeline {} creation successful", info.name))
    }

    RasterPipeline::~RasterPipeline()
    {
        if (pipeline != VK_NULL_HANDLE)
        {
            device->pipeline_zombies.push({
                .pipeline = pipeline,
                .cpu_timeline_value = device->main_cpu_timeline_value,
            });
        }
    }

    ComputePipeline::ComputePipeline(ComputePipelineCreateInfo const & info) : device{info.device}
    {
        auto read_spirv_from_file = [](std::filesystem::path filepath) -> std::vector<u32>
        {
            std::ifstream ifs{filepath, std::ios::binary};
            if (!ifs)
            {
                BACKEND_LOG(fmt::format("[ERROR][Pipeline::Pipeline()] Shader spirv path invalid {}", filepath.string()));
                throw std::runtime_error("[ERROR][Pipeline::Pipeline()] Shader spirv path invalid");
            }
            ifs.seekg(0, ifs.end);
            const i32 filesize = ifs.tellg();
            ifs.seekg(0, ifs.beg);
            std::vector<u32> raw(filesize / 4);
            if (!ifs.read(reinterpret_cast<char *>(raw.data()), filesize))
            {
                BACKEND_LOG(fmt::format("[ERROR][Pipeline::Pipeline()] Failed to read entire shader spirv"));
                throw std::runtime_error("[ERROR][Pipeline::Pipeline()] Failed to read entire shader spirv");
            }
            return raw;
        };
        auto const spirv = read_spirv_from_file(info.comp_spirv_path);
        VkShaderModule shader_module = nullptr;
        VkShaderModuleCreateInfo const shader_module_create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .codeSize = static_cast<u32>(spirv.size() * sizeof(u32)),
            .pCode = spirv.data(),
        };
        CHECK_VK_RESULT(vkCreateShaderModule(device->vulkan_device, &shader_module_create_info, nullptr, &shader_module));
        VkPipelineShaderStageCreateInfo const vk_pipeline_shader_stage_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader_module,
            .pName = info.entry_point.c_str(),
            .pSpecializationInfo = nullptr,
        };
        layout = device->resource_table->pipeline_layouts.at((info.push_constant_size + 3) / 4);

        VkComputePipelineCreateInfo const compute_pipeline_create_info{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .stage = vk_pipeline_shader_stage_create_info,
            .layout = layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
        };

        CHECK_VK_RESULT(vkCreateComputePipelines(device->vulkan_device, VK_NULL_HANDLE, 1u, &compute_pipeline_create_info, nullptr, &pipeline));
        vkDestroyShaderModule(device->vulkan_device, shader_module, nullptr);
        {
            VkDebugUtilsObjectNameInfoEXT const name_info{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .pNext = nullptr,
                .objectType = VK_OBJECT_TYPE_PIPELINE,
                .objectHandle = std::bit_cast<u64>(pipeline),
                .pObjectName = info.name.c_str(),
            };
            CHECK_VK_RESULT(device->vkSetDebugUtilsObjectNameEXT(device->vulkan_device, &name_info));
        }
        BACKEND_LOG(fmt::format("[INFO][Pipeline::Pipeline()] Pipeline {} creation successful", info.name));
    }

    ComputePipeline::~ComputePipeline()
    {
        if(pipeline != VK_NULL_HANDLE)
        {
            device->pipeline_zombies.push({
                .pipeline = pipeline,
                .cpu_timeline_value = device->main_cpu_timeline_value,
            });
        }
    }
} // namespace ff