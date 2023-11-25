#include "device.hpp"
#include "features.hpp"

namespace ff
{
    Device::Device(std::shared_ptr<Instance> instance) :
        instance{instance},
        main_queue_family_index{-1},
        vulkan_physical_device{get_physical_device()},
        physical_device_properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2}
    {
        vkGetPhysicalDeviceProperties2(vulkan_physical_device, &physical_device_properties);
        u32 queue_family_properties_count = 0;
        std::vector<VkQueueFamilyProperties> queue_properties = {};

        vkGetPhysicalDeviceQueueFamilyProperties(vulkan_physical_device, &queue_family_properties_count, nullptr);
        queue_properties.resize(queue_family_properties_count);
        vkGetPhysicalDeviceQueueFamilyProperties(vulkan_physical_device, &queue_family_properties_count, queue_properties.data());

        std::vector<VkBool32> supports_present(queue_family_properties_count);
        for(u32 i = 0; i < queue_family_properties_count; i++)
        {
            if((queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
               (queue_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) != 0 &&
               (queue_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
            {
                main_queue_family_index = i;
                break;
            }
        }
        if(main_queue_family_index == -1) { throw std::runtime_error(fmt::format("[Device::Device()] Found no suitable queue family - ERROR")); }
        std::array<f32, 1> queue_priorities = {0.0f};

        VkDeviceQueueCreateInfo const queue_create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = static_cast<u32>(main_queue_family_index),
            .queueCount = static_cast<u32>(queue_priorities.size()),
            .pQueuePriorities = queue_priorities.data(),
        };

        PhysicalDeviceFeatureTable feature_table = {};
        feature_table.initialize();
        PhysicalDeviceExtensionList extension_list = {};
        extension_list.initialize();

        VkPhysicalDeviceFeatures2 physical_device_features_2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = feature_table.chain,
            .features = feature_table.features,
        };

        VkDeviceCreateInfo const device_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = reinterpret_cast<void const *>(&physical_device_features_2),
            .flags = {},
            .queueCreateInfoCount = static_cast<u32>(1),
            .pQueueCreateInfos = &queue_create_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<u32>(extension_list.size),
            .ppEnabledExtensionNames = extension_list.data,
            .pEnabledFeatures = nullptr,
        };

        CHECK_VK_RESULT(vkCreateDevice(vulkan_physical_device, &device_create_info, nullptr, &vulkan_device));
        BACKEND_LOG("[INFO][Device::Device()] Vulkan device creation successful")

        vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(vulkan_device, "vkSetDebugUtilsObjectNameEXT"));
        vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(vulkan_device, "vkCmdBeginDebugUtilsLabelEXT"));
        vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(vulkan_device, "vkCmdEndDebugUtilsLabelEXT"));

        vkGetDeviceQueue(vulkan_device, main_queue_family_index, 0, &main_queue);

        main_cpu_timeline_value = 0;
        VkSemaphoreTypeCreateInfo timeline_create_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = nullptr,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = main_cpu_timeline_value,
        };

        VkSemaphoreCreateInfo const vk_semaphore_create_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = reinterpret_cast<void *>(&timeline_create_info),
            .flags = {},
        };
        CHECK_VK_RESULT(vkCreateSemaphore(vulkan_device, &vk_semaphore_create_info, nullptr, &main_gpu_semaphore));

        u32 const device_max_ds_buffers = physical_device_properties.properties.limits.maxDescriptorSetStorageBuffers;
        if(MAX_BUFFERS > device_max_ds_buffers) {throw std::runtime_error(fmt::format("[ERROR][Device::Device()] MAX BUFFERS - {} exceeds device properties limit", MAX_BUFFERS, device_max_ds_buffers));}

        u32 const device_max_ds_images = std::min(
            physical_device_properties.properties.limits.maxDescriptorSetSampledImages,
            physical_device_properties.properties.limits.maxDescriptorSetStorageImages);
        if(MAX_BUFFERS > device_max_ds_buffers) {throw std::runtime_error(fmt::format("[ERROR][Device::Device()] MAX IMAGES - {} exceeds device properties limit - {}", MAX_IMAGES, device_max_ds_images));}

        u32 const device_max_ds_samplers = physical_device_properties.properties.limits.maxDescriptorSetSamplers;
        if(MAX_BUFFERS > device_max_ds_buffers) {throw std::runtime_error(fmt::format("[ERROR][Device::Device()] MAX SAMPLERS - {} exceeds device properties limit - {}", MAX_SAMPLERS, device_max_ds_samplers));}

         VkDebugUtilsObjectNameInfoEXT const device_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_DEVICE,
            .objectHandle = reinterpret_cast<uint64_t>(vulkan_device),
            .pObjectName = "FF Device",
        };
        CHECK_VK_RESULT(vkSetDebugUtilsObjectNameEXT(vulkan_device, &device_name_info));

        VkDebugUtilsObjectNameInfoEXT const main_queue_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_QUEUE,
            .objectHandle = reinterpret_cast<uint64_t>(main_queue),
            .pObjectName = "FF Main Queue",
        };
        CHECK_VK_RESULT(vkSetDebugUtilsObjectNameEXT(vulkan_device, &main_queue_name_info));

        VkDebugUtilsObjectNameInfoEXT const main_gpu_semaphore_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_SEMAPHORE,
            .objectHandle = reinterpret_cast<uint64_t>(main_gpu_semaphore),
            .pObjectName = "FF Main GPU semaphore",
        };
        CHECK_VK_RESULT(vkSetDebugUtilsObjectNameEXT(vulkan_device, &main_gpu_semaphore_name_info));
        BACKEND_LOG("[INFO][Device::Device()] Device initalization and setup successful")
        resource_table = std::make_unique<GpuResourceTable>(CreateGpuResourceTableInfo{
            .max_buffer_slots = MAX_BUFFERS,
            .max_image_slots = MAX_IMAGES,
            .max_sampler_slots = MAX_SAMPLERS,
            .vkSetDebugUtilsObjectNameEXT = vkSetDebugUtilsObjectNameEXT,
            .vulkan_device = vulkan_device
        });
    }

    auto Device::info_image(ImageId image_id) -> CreateImageInfo &
    {
        if(!resource_table->images.is_id_valid(image_id))
        {
            BACKEND_LOG(fmt::format("[ERROR][Device::info_image()] Invalid image id"));
            throw std::runtime_error("[ERROR][Device::info_image()] Invalid image id");
        }
        return resource_table->images.slot(image_id)->image_info;
    }

    auto Device::create_swapchain_image(VkImage swapchain_image, CreateImageInfo const & info) -> ImageId
    {
        ImageId const id = resource_table->images.create_slot();
        auto image = resource_table->images.slot(id);
        image->image = swapchain_image;
        image->image_info = info;

        VkImageViewCreateInfo const image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = swapchain_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = info.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };
        CHECK_VK_RESULT(vkCreateImageView(vulkan_device, &image_view_create_info, nullptr, &image->image_view));
        BACKEND_LOG(fmt::format("[INFO][Device::create_swapchain_image()] {} image view created successfuly", info.name))

        VkDebugUtilsObjectNameInfoEXT const swapchain_image_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_IMAGE,
            .objectHandle = reinterpret_cast<uint64_t>(image->image),
            .pObjectName = image->image_info.name.c_str(),
        };
        CHECK_VK_RESULT(vkSetDebugUtilsObjectNameEXT(vulkan_device, &swapchain_image_name_info));

        VkDebugUtilsObjectNameInfoEXT const swapchain_image_view_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
            .objectHandle = reinterpret_cast<uint64_t>(image->image_view),
            .pObjectName = image->image_info.name.c_str(),
        };
        CHECK_VK_RESULT(vkSetDebugUtilsObjectNameEXT(vulkan_device, &swapchain_image_view_name_info));

        resource_table->write_descriptor_set_image(id);
        return id;
    }

    auto Device::get_physical_device() -> VkPhysicalDevice
    {
        std::vector<VkPhysicalDevice> physical_devices = {};
        u32 physical_device_count = {};
        CHECK_VK_RESULT(vkEnumeratePhysicalDevices(instance->vulkan_instance, &physical_device_count, nullptr));
        physical_devices.resize(physical_device_count);
        CHECK_VK_RESULT(vkEnumeratePhysicalDevices(instance->vulkan_instance, &physical_device_count, physical_devices.data()));

#if BACKEND_LOGGING
        BACKEND_LOG(fmt::format("[INFO][Instance::create_device()] Found {} physical devices", physical_device_count));
        for(auto const & physical_device : physical_devices)
        {
            VkPhysicalDeviceProperties physical_device_properties = {};
            vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
            BACKEND_LOG(fmt::format("\t - {}", physical_device_properties.deviceName));
        }
#endif //BACKEND_LOGGING

        auto selector = [&](VkPhysicalDeviceProperties const & properties)
        {
            i32 score = 0;
            switch(properties.deviceType)
            {
                case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1000; break;
                case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 100; break;
                case VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 10; break;
                default: break;
            }
            return score;
        };

        auto device_score = [&](VkPhysicalDevice physical_device) -> i32
        {
            VkPhysicalDeviceProperties physical_device_properties = {};
            vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
            bool const supported_api_version = physical_device_properties.apiVersion < VK_API_VERSION_1_3;
            return supported_api_version ? -1 : selector(physical_device_properties);
        };

        auto device_comparator = [&](VkPhysicalDevice const & a, VkPhysicalDevice const & b) { return device_score(a) < device_score(b); };
        auto const best_physical_device_it = std::max_element(physical_devices.begin(), physical_devices.end(), device_comparator);
        if(device_score(*best_physical_device_it) == -1) { throw std::runtime_error(fmt::format("[ERROR][Device::get_physical_device()] Found no suitable device")); }
        return *best_physical_device_it;
    }

    void Device::destroy_swapchain_image(ImageId id)
    {
        auto image = resource_table->images.slot(id);
        vkDestroyImageView(vulkan_device, image->image_view, nullptr);
        BACKEND_LOG(fmt::format("[INFO][Device::destroy_swapchain_image()] {} image view destroyed ", image->image_info.name));
        resource_table->images.destroy_slot(id);
    }

    void Device::submit(SubmitInfo const & info)
    {
        main_cpu_timeline_value += 1;
        std::vector<VkSemaphore> submit_semaphore_waits = {};
        std::vector<VkPipelineStageFlags> submit_semaphore_wait_stages = {};
        std::vector<u64> submit_semaphore_wait_values = {};
        submit_semaphore_waits.reserve(info.wait_binary_semaphores.size() + info.wait_timeline_semaphores.size());
        for(u32 wait_binary_sema_idx = 0 ; wait_binary_sema_idx < info.wait_binary_semaphores.size(); wait_binary_sema_idx++)
        {
            submit_semaphore_waits.push_back(info.wait_binary_semaphores[wait_binary_sema_idx]);
            submit_semaphore_wait_stages.push_back(VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            submit_semaphore_wait_values.push_back(0); // Ignored for binary semaphores
        }
        for(u32 wait_timeline_sema_idx = 0 ; wait_timeline_sema_idx < info.wait_timeline_semaphores.size(); wait_timeline_sema_idx++)
        {
            submit_semaphore_waits.push_back(info.wait_timeline_semaphores[wait_timeline_sema_idx].semaphore);
            submit_semaphore_wait_stages.push_back(VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
            submit_semaphore_wait_values.push_back(info.wait_timeline_semaphores[wait_timeline_sema_idx].value);
        }

        std::vector<VkSemaphore> submit_semaphore_signals = {};
        std::vector<u64> submit_semaphore_signal_values = {};
        submit_semaphore_signals.reserve(info.signal_binary_semaphores.size() + info.signal_timeline_semaphores.size() + 1);
        submit_semaphore_signals.push_back(main_gpu_semaphore);
        submit_semaphore_signal_values.push_back(main_cpu_timeline_value);
        for(u32 signal_binary_sema_idx = 0 ; signal_binary_sema_idx < info.signal_binary_semaphores.size(); signal_binary_sema_idx++)
        {
            submit_semaphore_signals.push_back(info.signal_binary_semaphores[signal_binary_sema_idx]);
            submit_semaphore_signal_values.push_back(0); // Ignored for binary semaphores
        }
        for(u32 signal_timeline_sema_idx = 0 ; signal_timeline_sema_idx < info.signal_timeline_semaphores.size(); signal_timeline_sema_idx++)
        {
            submit_semaphore_signals.push_back(info.signal_timeline_semaphores[signal_timeline_sema_idx].semaphore);
            submit_semaphore_signal_values.push_back(info.signal_timeline_semaphores[signal_timeline_sema_idx].value);
        }

        VkTimelineSemaphoreSubmitInfo timeline_submit_info = {
            .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreValueCount = static_cast<u32>(submit_semaphore_wait_values.size()),
            .pWaitSemaphoreValues = submit_semaphore_wait_values.data(),
            .signalSemaphoreValueCount = static_cast<u32>(submit_semaphore_signal_values.size()),
            .pSignalSemaphoreValues = submit_semaphore_signal_values.data()
        };

        VkSubmitInfo const submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = reinterpret_cast<void*>(&timeline_submit_info),
            .waitSemaphoreCount = static_cast<u32>(submit_semaphore_waits.size()),
            .pWaitSemaphores = submit_semaphore_waits.data(),
            .pWaitDstStageMask = submit_semaphore_wait_stages.data(),
            .commandBufferCount = static_cast<u32>(info.command_buffers.size()),
            .pCommandBuffers = info.command_buffers.data(),
            .signalSemaphoreCount = static_cast<u32>(submit_semaphore_signals.size()),
            .pSignalSemaphores = submit_semaphore_signals.data(),
        };

        CHECK_VK_RESULT(vkQueueSubmit(main_queue, 1, &submit_info, VK_NULL_HANDLE));
    }

    void Device::cleanup_resources()
    {
        u64 gpu_timeline_value = {};
        CHECK_VK_RESULT(vkGetSemaphoreCounterValue(vulkan_device, main_gpu_semaphore, &gpu_timeline_value));
        while(!command_buffer_zombies.empty())
        {
            if(command_buffer_zombies.front().cpu_timeline_value > gpu_timeline_value)
            {
                break;
            }
            vkFreeCommandBuffers(vulkan_device, command_buffer_zombies.front().pool, 1, &command_buffer_zombies.front().buffer);
            vkDestroyCommandPool(vulkan_device, command_buffer_zombies.front().pool, nullptr);
            command_buffer_zombies.pop();
        }

        while(!pipeline_zombies.empty())
        {
            if(pipeline_zombies.front().cpu_timeline_value > gpu_timeline_value)
            {
                break;
            }
            vkDestroyPipeline(vulkan_device, pipeline_zombies.front().pipeline, nullptr);
            pipeline_zombies.pop();
        }
    }

    void Device::wait_idle()
    {
        CHECK_VK_RESULT(vkQueueWaitIdle(main_queue));
        CHECK_VK_RESULT(vkDeviceWaitIdle(vulkan_device));
    }

    Device::~Device()
    {
        resource_table.reset();
        wait_idle();
        cleanup_resources();
        vkDestroySemaphore(vulkan_device, main_gpu_semaphore, nullptr);
        vkDestroyDevice(vulkan_device, nullptr);
        BACKEND_LOG("[INFO][Device::~Device()] Device destroyed")
    }
}