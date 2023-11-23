#include "device.hpp"
#include "features.hpp"

namespace ff
{
    Device::Device(VkPhysicalDevice const & physical_device) :
        main_queue_family_index{-1},
        physical_device{physical_device},
        physical_device_properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2}
    {
        vkGetPhysicalDeviceProperties2(physical_device, &physical_device_properties);
        u32 queue_family_properties_count = 0;
        std::vector<VkQueueFamilyProperties> queue_properties = {};

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, nullptr);
        queue_properties.resize(queue_family_properties_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, queue_properties.data());

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

        CHECK_VK_RESULT(vkCreateDevice(physical_device, &device_create_info, nullptr, &device));
        BACKEND_LOG("[Device::Device()] Device creation - SUCCESS")
    }

    Device::~Device()
    {
        vkDestroyDevice(device, nullptr);
        BACKEND_LOG("[Device::Device()] Device destruction - SUCCESS")
    }
}