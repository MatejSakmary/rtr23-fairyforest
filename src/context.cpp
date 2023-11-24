#include "context.hpp"

Context::Context(void * window_handle) :
    instance{std::make_shared<ff::Instance>()},
    device{std::make_shared<ff::Device>(instance)},
    swapchain{std::make_shared<ff::Swapchain>(ff::CreateSwapchainInfo{
        .instance = instance,
        .device = device,
        .window_handle = window_handle 
    })}
{

}

Context::~Context()
{

};