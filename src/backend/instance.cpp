#include "instance.hpp"
#include <algorithm>

namespace ff 
{
    Instance::Instance()
    {
        std::vector<char const *> required_extensions = {};
        required_extensions.push_back({VK_KHR_SURFACE_EXTENSION_NAME});
        required_extensions.push_back({VK_EXT_DEBUG_UTILS_EXTENSION_NAME});
        required_extensions.push_back({VK_KHR_WIN32_SURFACE_EXTENSION_NAME});

        std::vector<VkExtensionProperties> instance_extensions = {};
        u32 instance_extension_count = {};
        CHECK_VK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));
        instance_extensions.resize(instance_extension_count);
        CHECK_VK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.data()));

        for(auto const * required_extension : required_extensions)
        {
            auto ext_predicate = [=](VkExtensionProperties const & property) { return std::strcmp(property.extensionName, required_extension) == 0; };
            auto const req_instance_extension_it = std::find_if(instance_extensions.begin(), instance_extensions.end(), ext_predicate);
            if(req_instance_extension_it == instance_extensions.end())
            {
                throw std::runtime_error(fmt::format("[Instance::Instance] did not find support for extension {}", required_extension));
            }
        }

        const VkApplicationInfo application_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = APP_NAME,
            .applicationVersion = 0,
            .pEngineName = ENGINE_NAME,
            .engineVersion = 1,
            .apiVersion = VK_API_VERSION_1_3,
        };

        VkInstanceCreateInfo const instance_create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .pApplicationInfo = &application_info,
            .enabledLayerCount = 0u,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
            .ppEnabledExtensionNames = required_extensions.data(),
        };
        CHECK_VK_RESULT(vkCreateInstance(&instance_create_info, nullptr, &vk_instance));
        BACKEND_LOG("[Instance::Instance()] Instance creation - SUCCESS");
    }

    auto Instance::create_device() -> Device
    {
        std::vector<VkPhysicalDevice> physical_devices = {};
        u32 physical_device_count = {};
        CHECK_VK_RESULT(vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr));
        physical_devices.resize(physical_device_count);
        CHECK_VK_RESULT(vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices.data()));

#if BACKEND_LOGGING
        BACKEND_LOG(fmt::format("[Instance::create_device()] Found {} physical devices", physical_device_count));
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
        if(device_score(*best_physical_device_it) == -1) { throw std::runtime_error(fmt::format("[Instance::create_device()] Found no suitable device - ERROR")); }
        VkPhysicalDevice const best_physical_device = *best_physical_device_it;
        return ff::Device(best_physical_device);
    }

    Instance::~Instance()
    {
        vkDestroyInstance(vk_instance, nullptr);
        BACKEND_LOG("[Instance::Instance()] Instance destruction - SUCCESS");
    }
}