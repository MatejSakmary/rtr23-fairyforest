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
#include <vk_mem_alloc.h>
#include <stdexcept>

#define CHECK_VK_RESULT(x)                                                                                                  \
    if((x) < 0)                                                                                                             \
    {                                                                                                                       \
        BACKEND_LOG(fmt::format("[ERROR][Core::CHECK_VK_RESULT] VK_RESULT has non-success value {}", static_cast<i32>(x)))  \
        throw std::runtime_error("[ERROR][CORE::CHECK_VK_RESULT] VK_RESULT did not return success");                        \
    }

#if defined(BACKEND_LOGGING)
#define BACKEND_LOG(M) fmt::println("[BACKEND]{}", M);
#else
#define BACKEND_LOG(M)
#endif