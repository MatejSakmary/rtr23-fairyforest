#pragma once

#include "core.hpp"
#include "../fairy_forest.hpp"

namespace ff
{
    struct Device
    {
        public:

            VkPhysicalDeviceProperties2 physical_device_properties = {};

            Device() = default;
            Device(VkPhysicalDevice const & physical_device);
            ~Device();
        private:
            VkPhysicalDevice physical_device = {};
            VkDevice device = {};
            i32 main_queue_family_index = {};
    };
}
