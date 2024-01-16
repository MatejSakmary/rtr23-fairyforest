#pragma once

#include "fairy_forest.hpp"
#include "window.hpp"
#include "context.hpp"
#include "scene/scene.hpp"
#include "scene/asset_processor.hpp"
#include "rendering/renderer.hpp"
#include "camera.hpp"
using namespace ff::types;

struct Application
{
  public:
    Application();
    ~Application();

    auto run() -> i32;

  private:
    void update();
    f32 delta_time = 0.016666f;
    std::chrono::time_point<std::chrono::steady_clock> last_time_point = {};

    bool keep_running = {};
    u32 no_ao = {};
    u32 force_ao = {};
    u32 no_albedo = {};
    u32 no_shadows = {};
    u32 no_normal_maps = {};
    std::unique_ptr<Window> window = {};
    std::shared_ptr<Context> context = {};
    std::unique_ptr<ff::Renderer> renderer = {};
    std::unique_ptr<Scene> scene = {};
    std::unique_ptr<AssetProcessor> asset_processor = {};
    CameraController camera_controller = {};
    CinematicCamera camera = {};
    SceneDrawCommands commands = {};
};