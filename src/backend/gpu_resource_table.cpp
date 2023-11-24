#include "gpu_resource_table.hpp"

namespace ff
{
    GpuResourceTable::GpuResourceTable(CreateGpuResourceTableInfo const & info) :
        info{info},
        vulkan_device{info.vulkan_device}
    {
        VkDescriptorPoolSize const buffer_descriptor_pool_size = {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = info.max_buffer_slots
        };
        VkDescriptorPoolSize const storage_image_descriptor_pool_size = {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = info.max_image_slots
        };
        VkDescriptorPoolSize const sampled_image_descriptor_pool_size = {
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = info.max_image_slots
        };
        VkDescriptorPoolSize const sampler_descriptor_pool_size = {
            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = info.max_sampler_slots
        };

        std::array<VkDescriptorPoolSize, 4> const pool_sizes = {
            buffer_descriptor_pool_size,
            storage_image_descriptor_pool_size,
            sampled_image_descriptor_pool_size,
            sampler_descriptor_pool_size
        };

        VkDescriptorPoolCreateInfo const descriptor_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 
                VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT |
                VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = static_cast<u32>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(), 
        };

        CHECK_VK_RESULT(vkCreateDescriptorPool(vulkan_device, &descriptor_pool_create_info, nullptr, &descriptor_pool));
        BACKEND_LOG("[INFO][GpuResourceTable::GpuResourceTable()] Bindless descriptor pool creation successful");
        VkDebugUtilsObjectNameInfoEXT descriptor_pool_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL,
            .objectHandle = reinterpret_cast<uint64_t>(descriptor_pool),
            .pObjectName = "FF Bindless descriptor pool",
        };
        CHECK_VK_RESULT(info.vkSetDebugUtilsObjectNameEXT(vulkan_device, &descriptor_pool_name_info));

        VkDescriptorSetLayoutBinding const buffer_descriptor_set_layout_binding{
            .binding = BUFFER_BINDING,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = static_cast<u32>(info.max_buffer_slots),
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr,
        };

        VkDescriptorSetLayoutBinding const storage_image_descriptor_set_layout_binding{
            .binding = STORAGE_IMAGE_BINDING,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = static_cast<u32>(info.max_image_slots),
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr,
        };

        VkDescriptorSetLayoutBinding const sampled_image_descriptor_set_layout_binding{
            .binding = SAMPLED_IMAGE_BINDING,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = static_cast<u32>(info.max_image_slots),
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr,
        };

        VkDescriptorSetLayoutBinding const sampler_descriptor_set_layout_binding{
            .binding = SAMPLER_BINDING,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = static_cast<u32>(info.max_sampler_slots),
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr,
        };

        std::array<VkDescriptorSetLayoutBinding, 4> const descriptor_set_layout_bindings = {
            buffer_descriptor_set_layout_binding,
            storage_image_descriptor_set_layout_binding,
            sampled_image_descriptor_set_layout_binding,
            sampler_descriptor_set_layout_binding,
        };

        std::array<VkDescriptorBindingFlags, 4> const descriptor_binding_flags = {
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        };

        VkDescriptorSetLayoutBindingFlagsCreateInfo descriptor_set_layout_binding_flags_create_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .pNext = nullptr,
            .bindingCount = static_cast<u32>(descriptor_binding_flags.size()),
            .pBindingFlags = descriptor_binding_flags.data(),
        };

        VkDescriptorSetLayoutCreateInfo const descriptor_set_layout_create_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &descriptor_set_layout_binding_flags_create_info,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = static_cast<u32>(descriptor_set_layout_bindings.size()),
            .pBindings = descriptor_set_layout_bindings.data(),
        };
        CHECK_VK_RESULT(vkCreateDescriptorSetLayout(vulkan_device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout));
        BACKEND_LOG("[INFO][GpuResourceTable::GpuResourceTable()] Bindless descriptor pool layout creation successful");
        VkDebugUtilsObjectNameInfoEXT descriptor_layout_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
            .objectHandle = reinterpret_cast<uint64_t>(descriptor_set_layout),
            .pObjectName = "FF bindless descriptor layout",
        };
        CHECK_VK_RESULT(info.vkSetDebugUtilsObjectNameEXT(vulkan_device, &descriptor_layout_name_info));

        VkDescriptorSetAllocateInfo const descriptor_set_allocate_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptor_set_layout,
        };

        CHECK_VK_RESULT(vkAllocateDescriptorSets(vulkan_device, &descriptor_set_allocate_info, &descriptor_set));
        VkDebugUtilsObjectNameInfoEXT descriptor_set_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET,
            .objectHandle = reinterpret_cast<uint64_t>(descriptor_set),
            .pObjectName = "FF bindless descriptor set",
        };
        CHECK_VK_RESULT(info.vkSetDebugUtilsObjectNameEXT(vulkan_device, &descriptor_set_name_info));
        BACKEND_LOG("[INFO][GpuResourceTable::GpuResourceTable()] Bindless descriptor set allocation successful");

        VkPipelineLayoutCreateInfo pipeline_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .setLayoutCount = 1,
            .pSetLayouts = &descriptor_set_layout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        };

        CHECK_VK_RESULT(vkCreatePipelineLayout(vulkan_device, &pipeline_create_info, nullptr, pipeline_layouts.data()));
        {
            VkDebugUtilsObjectNameInfoEXT name_info{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .pNext = nullptr,
                .objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                .objectHandle = reinterpret_cast<uint64_t>(*pipeline_layouts.data()),
                .pObjectName = "pipeline layout (push constant size 0)",
            };
            CHECK_VK_RESULT(info.vkSetDebugUtilsObjectNameEXT(vulkan_device, &name_info));
        }

        for (usize i = 1; i < PIPELINE_LAYOUT_COUNT; ++i)
        {
            VkPushConstantRange const vk_push_constant_range{
                .stageFlags = VK_SHADER_STAGE_ALL,
                .offset = 0,
                .size = static_cast<u32>(i * 4),
            };
            pipeline_create_info.pushConstantRangeCount = 1;
            pipeline_create_info.pPushConstantRanges = &vk_push_constant_range;
            CHECK_VK_RESULT(vkCreatePipelineLayout(vulkan_device, &pipeline_create_info, nullptr, &pipeline_layouts.at(i)));
            {
                auto name = fmt::format("pipeline layout (push constant size {})", i * 4);
                VkDebugUtilsObjectNameInfoEXT name_info{
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                    .pNext = nullptr,
                    .objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                    .objectHandle = reinterpret_cast<uint64_t>(pipeline_layouts.at(i)),
                    .pObjectName = name.c_str(),
                };
                CHECK_VK_RESULT(info.vkSetDebugUtilsObjectNameEXT(vulkan_device, &name_info));
            }
        }
        BACKEND_LOG("[INFO][GpuResourceTable::GpuResourceTable()] Pipeline layouts creation successful");
    }

    void GpuResourceTable::write_descriptor_set_image(ImageId id)
    {
        auto const * image = images.slot(id);
        u32 descriptor_set_write_count = 0;
        std::array<VkWriteDescriptorSet, 2> descriptor_set_writes = {};

        VkDescriptorImageInfo const vk_descriptor_image_info{
            .sampler = VK_NULL_HANDLE,
            .imageView = image->image_view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };

        VkWriteDescriptorSet const write_descriptor_set{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptor_set,
            .dstBinding = STORAGE_IMAGE_BINDING,
            .dstArrayElement = id.index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &vk_descriptor_image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        };

        if ((image->image_info.usage & VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT) != 0)
        {
            descriptor_set_writes.at(descriptor_set_write_count++) = write_descriptor_set;
        }

        VkDescriptorImageInfo const descriptor_image_info_sampled{
            .sampler = VK_NULL_HANDLE,
            .imageView = image->image_view,
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet const write_descriptor_set_sampled{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptor_set,
            .dstBinding = SAMPLED_IMAGE_BINDING,
            .dstArrayElement = id.index,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &descriptor_image_info_sampled,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        };

        if ((image->image_info.usage & VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT) != 0)
        {
            descriptor_set_writes.at(descriptor_set_write_count++) = write_descriptor_set_sampled;
        }

        vkUpdateDescriptorSets(vulkan_device, descriptor_set_write_count, descriptor_set_writes.data(), 0, nullptr);
        BACKEND_LOG(fmt::format("[INFO][GpuResourceTable::write_descriptor_set_image] updated descriptor set index {} with image {}", id.index, image->image_info.name));
    }

    void GpuResourceTable::write_descriptor_set_buffer(BufferId id)
    {
        /// TODO: (msakmary) Implement
    }

    void GpuResourceTable::write_descriptor_set_sampler(SamplerId id)
    {
        /// TODO: (msakmary) Implement
    }

    GpuResourceTable::~GpuResourceTable()
    {
        for(auto const & pipeline_layout : pipeline_layouts)
        {
            vkDestroyPipelineLayout(vulkan_device, pipeline_layout, nullptr);
        }
        BACKEND_LOG("[INFO][GpuResourceTable::~GpuResourceTable()] Pipeline layouts destroyed");
        vkFreeDescriptorSets(vulkan_device, descriptor_pool, 1, &descriptor_set);
        BACKEND_LOG("[INFO][GpuResourceTable::~GpuResourceTable()] Bindless descriptor set freed");
        vkDestroyDescriptorSetLayout(vulkan_device, descriptor_set_layout, nullptr);
        BACKEND_LOG("[INFO][GpuResourceTable::~GpuResourceTable()] Bindless descriptor layout destroyed");
        vkDestroyDescriptorPool(vulkan_device, descriptor_pool, nullptr);
        BACKEND_LOG("[INFO][GpuResourceTable::~GpuResourceTable()] Bindless descriptor pool destroyed");
    }
}