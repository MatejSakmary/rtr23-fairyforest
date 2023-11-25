#pragma once

#include "../fairy_forest.hpp"
#include "core.hpp"

namespace ff
{
    struct PhysicalDeviceFeatureTable
    {
        VkPhysicalDeviceFeatures features = {};
        VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address = {};
        VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing = {};
        VkPhysicalDeviceHostQueryResetFeatures host_query_reset = {};
        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {};
        VkPhysicalDeviceSynchronization2Features sync2 = {};
        VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore = {};
        VkPhysicalDeviceScalarBlockLayoutFeatures scalar_layout = {};
        void * chain = {};

        void initialize();
    };

    struct PhysicalDeviceExtensionList
    {
        static constexpr inline u32 EXTENSION_LIST_MAX = 16;
        char const * data[EXTENSION_LIST_MAX] = {};
        u32 size = {};

        void initialize();
    };
} // namespace ff
