#pragma once
#include "fairy_forest.hpp"
#include "window.hpp"
#include "rendering/renderer.hpp"

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
    f32 translation_speed = 2.0f;
    f32vec3 up = {0.f, 0.f, 1.0f};
    f32vec3 forward = {0.f, 0.f, 0.f};
    f32vec3 position = {0.f, 1.f, 0.f};
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
};

struct AnimationKeyframe
{
    glm::fquat start_rotation;
    glm::fquat end_rotation;

    f32vec3 start_position;
    f32vec3 first_control_point;
    f32vec3 second_control_point;
    f32vec3 end_position;
    f32 transition_time;
};

struct CinematicCamera
{
    CinematicCamera() = default;
    CinematicCamera(std::vector<AnimationKeyframe> const & keyframes, Window & window);
    void update_position(f32 dt);
    CameraInfo info;

    f32 near_plane = 0.1f;

    private:
        f32 current_keyframe_time = 0.0f;
        u32 current_keyframe_index = 0;
        std::vector<AnimationKeyframe> path_keyframes = {};
};