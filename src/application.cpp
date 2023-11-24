#include "application.hpp"

Application::Application() : 
    keep_running(true),
    window{std::make_unique<Window>(1920, 1080, "Fairy Forest")},
    context{std::make_shared<Context>(window->get_handle())},
    renderer{std::make_unique<ff::Renderer>(context)}
{
}

auto Application::run() -> i32
{
    while (keep_running)
    {
        window->update(16.6);
        if(window->window_state->resized) 
        {
            APP_LOG("[INFO][Application::run()] Window resize detected");
            renderer->resize(); 
        }
        renderer->draw_frame();
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