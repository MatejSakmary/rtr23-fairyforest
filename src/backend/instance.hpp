#pragma once

#include "../fairy_forest.hpp"
#include "core.hpp"

namespace ff
{
    struct Instance
    {
      public:
        Instance();
        ~Instance();

      private:
        friend struct Device;
        friend struct Swapchain;
        VkInstance vulkan_instance = {};
    };
} // namespace ff