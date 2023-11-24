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
                throw std::runtime_error(fmt::format("[ERROR][Instance::Instance] did not find support for extension {}", required_extension));
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
        CHECK_VK_RESULT(vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance));
        BACKEND_LOG("[INFO][Instance::Instance()] Instance creation successful");
    }

    Instance::~Instance()
    {
        vkDestroyInstance(vulkan_instance, nullptr);
        BACKEND_LOG("[INFO][Instance::~Instance()] Instance destroyed");
    }
}