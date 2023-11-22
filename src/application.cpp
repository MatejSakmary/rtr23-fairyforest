#include "application.hpp"
#include <fmt/core.h>
#include <fmt/format.h>

Application::Application() : keep_running(true)
{
    _window = std::make_unique<Window>(1920, 1080, "Fairy Forest");
}

auto Application::run() -> i32
{
    while (keep_running)
    {
        _window->update(16.6);
        keep_running &= !static_cast<bool>(glfwWindowShouldClose(_window->glfw_handle));
    }
    return 0;
}

void Application::update()
{
}

Application::~Application()
{
}