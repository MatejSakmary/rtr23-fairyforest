#pragma once

#include "fairy_forest.hpp"
#include "window.hpp"
#include "context.hpp"
#include "rendering/renderer.hpp"
using namespace ff::types;

struct Application
{
public:
    Application();
    ~Application();

    auto run() -> i32;
private:
    void update();

    bool keep_running = {};
    std::unique_ptr<Window> window = {};
    std::shared_ptr<Context> context = {};
    std::unique_ptr<ff::Renderer> renderer = {};
};