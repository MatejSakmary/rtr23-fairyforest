#include "application.hpp"

Application::Application() : 
    keep_running(true),
    window{std::make_unique<Window>(1920, 1080, "Fairy Forest")},
    context{std::make_unique<Context>(window->get_handle())}
{
}

auto Application::run() -> i32
{
    while (keep_running)
    {
        window->update(16.6);
        keep_running &= !static_cast<bool>(glfwWindowShouldClose(window->glfw_handle));
    }
    return 0;
}

void Application::update()
{
}

Application::~Application()
{
}