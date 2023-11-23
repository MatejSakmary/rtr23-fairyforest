#pragma once

#include "../fairy_forest.hpp"
#include "core.hpp"
#include "device.hpp"

namespace ff
{
    struct Instance
    {
        public:
            Instance();
            ~Instance();

            auto create_device() -> Device;
        private:
            VkInstance vk_instance;
    };
}