#pragma once
#include "core.hpp"
#include "slotmap.hpp"
#include "../fairy_forest.hpp"

namespace ff
{
    static inline VkBufferUsageFlags const BUFFER_USE_FLAGS =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT |
        VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT |
        VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    struct CreateBufferInfo
    {
        size_t size = {};
        VmaAllocationCreateFlags flags = {};
        std::string name = {};
    };

    struct CreateImageInfo
    {
        u32 dimensions = 2;
        VkFormat format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
        VkExtent3D extent = {0, 0, 0};
        u32 mip_level_count = 1;
        u32 array_layer_count = 1;
        u32 sample_count = 1;
        VkImageUsageFlags usage = {};
        VmaAllocationCreateFlags alloc_flags = {};
        VkImageAspectFlags aspect = {};
        std::string name = {};
    };

	struct CreateSamplerInfo
	{
    	VkFilter magnification_filter = VkFilter::VK_FILTER_LINEAR;
    	VkFilter minification_filter = VkFilter::VK_FILTER_LINEAR;
    	VkSamplerMipmapMode mipmap_filter = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
    	VkSamplerReductionMode reduction_mode = VkSamplerReductionMode::VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
    	VkSamplerAddressMode address_mode_u = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    	VkSamplerAddressMode address_mode_v = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    	VkSamplerAddressMode address_mode_w = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    	f32 mip_lod_bias = 0.5f;
    	bool enable_anisotropy = false;
    	f32 max_anisotropy = 0.0f;
    	bool enable_compare = false;
    	VkCompareOp compare_op = VkCompareOp::VK_COMPARE_OP_ALWAYS;
    	f32 min_lod = 0.0f;
    	f32 max_lod = 1000.0f;
    	VkBorderColor border_color = VkBorderColor::VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    	bool enable_unnormalized_coordinates = 0;
    	std::string name = {};
	};

    struct Image
    {
      public:
        CreateImageInfo image_info = {};

      private:
        friend struct Device;
        friend struct GpuResourceTable;
        friend struct CommandBuffer;
        VkImage image = {};
        VkImageView image_view = {};
        VmaAllocation allocation = {};
    };

    struct Buffer
    {
      public:
        CreateBufferInfo buffer_info = {};

      private:
        friend struct Device;
        friend struct GpuResourceTable;
        friend struct CommandBuffer;
        VkBuffer buffer = {};
        VmaAllocation allocation = {};
        VkDeviceAddress device_address = {};
        void * host_address = {};
    };

    struct Sampler
    {	
		public:
			CreateSamplerInfo sampler_info;
		private:
        	friend struct Device;
        	friend struct GpuResourceTable;
        	friend struct CommandBuffer;
        	VkSampler sampler = {};
    };

    using ImageId = SlotMap<Image>::Id;
    using BufferId = SlotMap<Buffer>::Id;
    using SamplerId = SlotMap<Sampler>::Id;

    struct CreateGpuResourceTableInfo
    {
        u32 max_buffer_slots = {};
        u32 max_image_slots = {};
        u32 max_sampler_slots = {};
        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = {};
        VkDevice vulkan_device = {};
    };

    static inline constexpr u32 MAX_PUSH_CONSTANT_WORD_SIZE = {32};
    static inline constexpr u32 MAX_PUSH_CONSTANT_BYTE_SIZE = {MAX_PUSH_CONSTANT_WORD_SIZE * 4};
    static inline constexpr u32 PIPELINE_LAYOUT_COUNT = {MAX_PUSH_CONSTANT_WORD_SIZE + 1};

    /// NOTE: Must match the ones in backend.glsl
    static inline constexpr u32 BUFFER_BINDING = 0;
    static inline constexpr u32 STORAGE_IMAGE_BINDING = 1;
    static inline constexpr u32 SAMPLED_IMAGE_BINDING = 2;
    static inline constexpr u32 SAMPLER_BINDING = 3;

    struct GpuResourceTable
    {
      public:
        SlotMap<Image> images = {};
        SlotMap<Buffer> buffers = {};
        SlotMap<Sampler> samplers = {};

        GpuResourceTable(CreateGpuResourceTableInfo const & info);
        void write_descriptor_set_image(ImageId id);
        void write_descriptor_set_buffer(BufferId id);
        void write_descriptor_set_sampler(SamplerId id);
        ~GpuResourceTable();

      private:
        friend struct RasterPipeline;
        friend struct ComputePipeline;
        friend struct CommandBuffer;
        CreateGpuResourceTableInfo info = {};
        std::array<VkPipelineLayout, PIPELINE_LAYOUT_COUNT> pipeline_layouts = {};
        VkDevice vulkan_device = {};
        VkDescriptorSetLayout descriptor_set_layout = {};
        VkDescriptorSet descriptor_set = {};
        VkDescriptorPool descriptor_pool = {};
    };
} // namespace ff