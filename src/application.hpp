#pragma once

#include "fairy_forest.hpp"
#include "window.hpp"
#include "context.hpp"
#include "rendering/renderer.hpp"
#include "scene/scene.hpp"
#include "scene/asset_processor.hpp"
using namespace ff::types;



struct CameraController
{
    void process_input(Window & window, f32 dt);
    void update_matrices(Window & window);

    CameraInfo cam_info;

    bool zoom = false;
    f32 fov = 70.0f;
    f32 near_plane = 0.1f;
    f32 camera_sway_speed = 0.05f;
    f32 translation_speed = 10.0f;
    f32vec3 up = {0.f, 0.f, 1.0f};
    f32vec3 forward = {0.f, 0.f, 0.f};
    f32vec3 position = {0.f, 1.f, 0.f};
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
};

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
    std::unique_ptr<Window> window = {};
    std::shared_ptr<Context> context = {};
    std::unique_ptr<ff::Renderer> renderer = {};
    std::unique_ptr<Scene> scene = {};
    std::unique_ptr<AssetProcessor> asset_processor = {};
	CameraController camera_controller = {};
    SceneDrawCommands commands = {};
};