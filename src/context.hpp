#pragma once
#include "backend/backend.hpp"

struct Context
{
    std::shared_ptr<ff::Instance> instance = {};
    std::shared_ptr<ff::Device> device = {};
    std::shared_ptr<ff::Swapchain> swapchain = {};

    Context(void * window_handle);
    ~Context();
};