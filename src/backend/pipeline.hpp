#pragma once

#include "core.hpp"
#include <filesystem>
#include "../fairy_forest.hpp"
#include "device.hpp"

namespace ff
{
    struct RasterInfo
    {
        VkPrimitiveTopology primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        bool primitive_restart_enable = 0;
        VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags face_culling = VK_CULL_MODE_NONE;
        VkFrontFace front_face_winding = VK_FRONT_FACE_CLOCKWISE;
        bool depth_clamp_enable = false;
        bool rasterizer_discard_enable = false;
        bool depth_bias_enable = false;
        f32 depth_bias_constant_factor = 0.0f;
        f32 depth_bias_clamp = 0.0f;
        f32 depth_bias_slope_factor = 0.0f;
        f32 line_width = 1.0f;
    };

    struct BlendInfo
    {
        VkBlendFactor src_color_blend_factor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dst_color_blend_factor = VK_BLEND_FACTOR_ZERO;
        VkBlendOp color_blend_op = VK_BLEND_OP_ADD;
        VkBlendFactor src_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO;
        VkBlendOp alpha_blend_op = VK_BLEND_OP_ADD;
        VkColorComponentFlags color_write_mask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    };

    struct RenderAttachmentInfo
    {
        VkFormat format = {};
        std::optional<BlendInfo> blend_info = {};
    };

    struct DepthTestInfo
    {
        VkFormat depth_attachment_format = VK_FORMAT_UNDEFINED;
        bool enable_depth_write = 0;
        VkCompareOp depth_test_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
        f32 min_depth_bounds = 0.0f;
        f32 max_depth_bounds = 1.0f;
    };

    struct PipelineCreateInfo
    {
        std::shared_ptr<Device> device = {};
        /// TODO: add other stages here - if needed
        std::optional<std::filesystem::path> vert_spirv_path = {};
        std::optional<std::filesystem::path> frag_spirv_path = {};
        std::vector<RenderAttachmentInfo> attachments = {};
        std::optional<DepthTestInfo> depth_test = {};
        RasterInfo raster_info = {};
        std::string entry_point = {};
        u32 push_constant_size = {};
        std::string name = {};
    };

    /// TODO: I do raster pipeline ONLY for now add compute pipeline
    struct Pipeline
    {
      public:
        Pipeline() = default;
        Pipeline(PipelineCreateInfo const & info);
        Pipeline(Pipeline const & other) = delete;
        Pipeline & operator=(Pipeline const & other) = delete;
        Pipeline(Pipeline && other) = delete;
        Pipeline & operator=(Pipeline && other) = delete;
        ~Pipeline();

      private:
        friend struct CommandBuffer;

        std::shared_ptr<Device> device = {};
        VkPipelineLayout layout = {};
        VkPipeline pipeline = {};
    };
} // namespace ff