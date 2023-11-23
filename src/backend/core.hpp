#pragma once

#ifdef VULKAN_H_
#error MUST NOT INCLUDE VULKAN H BEFORE THIS FILE!
#endif
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#define APP_NAME "FairyForest"
#define ENGINE_NAME "ff"

#include <vulkan/vulkan.h>
#include <stdexcept>

#define CHECK_VK_RESULT(x)                                                          \
    if((x) < 0)                                                                     \
    {                                                                               \
        throw std::runtime_error("[CORE::ERROR] VK_RESULT did not return success"); \
    }

#if defined(BACKEND_LOGGING)
#define BACKEND_LOG(M) fmt::println("{}", M);
#else
#define BACKEND_LOG(M)
#endif