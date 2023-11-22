#pragma once

#include "fairy_forest.hpp"
#include "window.hpp"
using namespace fairyforest::types;

struct Application
{
public:
    Application();
    ~Application();

    auto run() -> i32;
private:
    void update();

    std::unique_ptr<Window> _window = {};
    bool keep_running = {};
};