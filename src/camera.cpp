#include "camera.hpp"

void CameraController::process_input(Window & window, f32 dt)
{
    f32 speed = window.key_pressed(GLFW_KEY_LEFT_SHIFT) ? translation_speed * 4.0f : translation_speed;
    speed = window.key_pressed(GLFW_KEY_LEFT_ALT) ? speed * 0.25f : speed;

    if (window.is_focused())
    {
        if (window.key_just_pressed(GLFW_KEY_ESCAPE))
        {
            if (window.is_cursor_captured())
            {
                window.release_cursor();
            }
            else
            {
                window.capture_cursor();
            }
        }
    }
    else if (window.is_cursor_captured())
    {
        window.release_cursor();
    }

    auto camera_sway_speed_tmp = camera_sway_speed;
    if (window.key_pressed(GLFW_KEY_C))
    {
        camera_sway_speed_tmp *= 0.25;
        zoom = true;
    }
    else
    {
        zoom = false;
    }

    glm::vec3 right = glm::cross(forward, up);
    glm::vec3 fake_up = glm::cross(right, forward);
    if (window.is_cursor_captured())
    {
        if (window.key_pressed(GLFW_KEY_W))
        {
            position += forward * speed * dt;
        }
        if (window.key_pressed(GLFW_KEY_S))
        {
            position -= forward * speed * dt;
        }
        if (window.key_pressed(GLFW_KEY_A))
        {
            position -= glm::normalize(glm::cross(forward, up)) * speed * dt;
        }
        if (window.key_pressed(GLFW_KEY_D))
        {
            position += glm::normalize(glm::cross(forward, up)) * speed * dt;
        }
        if (window.key_pressed(GLFW_KEY_SPACE))
        {
            position += fake_up * speed * dt;
        }
        if (window.key_pressed(GLFW_KEY_LEFT_CONTROL))
        {
            position -= fake_up * speed * dt;
        }
        pitch += window.get_cursor_change_y() * camera_sway_speed_tmp;
        pitch = std::clamp(pitch, -85.0f, 85.0f);
        yaw += window.get_cursor_change_x() * camera_sway_speed_tmp;
    }
    forward.x = -glm::cos(glm::radians(yaw - 90.0f)) * glm::cos(glm::radians(pitch));
    forward.y = glm::sin(glm::radians(yaw - 90.0f)) * glm::cos(glm::radians(pitch));
    forward.z = -glm::sin(glm::radians(pitch));
}

void CameraController::update_matrices(Window & window)
{
    auto fov_tmp = fov;
    if (zoom)
    {
        fov_tmp *= 0.25f;
    }
    auto inf_depth_reverse_z_perspective = [](auto fov_rads, auto aspect, auto z_near)
    {
        assert(abs(aspect - std::numeric_limits<f32>::epsilon()) > 0.0f);

        f32 const tan_half_fovy = 1.0f / std::tan(fov_rads * 0.5f);

        glm::mat4x4 ret(0.0f);
        ret[0][0] = tan_half_fovy / aspect;
        ret[1][1] = tan_half_fovy;
        ret[2][2] = 0.0f;
        ret[2][3] = -1.0f;
        ret[3][2] = z_near;
        return ret;
    };
    glm::mat4 prespective = inf_depth_reverse_z_perspective(glm::radians(fov_tmp), f32(window.get_width()) / f32(window.get_height()), near_plane);
    prespective[1][1] *= -1.0f;
    cam_info.proj = prespective;
    cam_info.view = glm::lookAt(position, position + forward, up);
    auto const view_quat = glm::quat_cast(cam_info.view);
    // APP_LOG(fmt::format("view quat {} {} {} {}", view_quat.w, view_quat.x, view_quat.y, view_quat.z));
    cam_info.viewproj = cam_info.proj * cam_info.view;
    cam_info.pos = position;
    cam_info.up = up;
}

CinematicCamera::CinematicCamera(std::vector<AnimationKeyframe> const & keyframes, Window & window) : path_keyframes{keyframes}
{
    auto inf_depth_reverse_z_perspective = [](auto fov_rads, auto aspect, auto z_near)
    {
        assert(abs(aspect - std::numeric_limits<f32>::epsilon()) > 0.0f);

        f32 const tan_half_fovy = 1.0f / std::tan(fov_rads * 0.5f);

        glm::mat4x4 ret(0.0f);
        ret[0][0] = tan_half_fovy / aspect;
        ret[1][1] = tan_half_fovy;
        ret[2][2] = 0.0f;
        ret[2][3] = -1.0f;
        ret[3][2] = z_near;
        return ret;
    };
    glm::mat4 prespective = inf_depth_reverse_z_perspective(glm::radians(70.0f), f32(window.get_width()) / f32(window.get_height()), near_plane);
    prespective[1][1] *= -1.0f;
    info.proj = prespective;
    info.up = {0.0f, 0.0f, 1.0f};
}

void CinematicCamera::update_position(f32 dt)
{
    // TODO(msakmary) Whenever the update position dt is longer than a whole keyframe transition time
    // this code will not properly account for this
    f32 const current_keyframe_finish_time = path_keyframes.at(current_keyframe_index).transition_time;
    bool const keyframe_finished = current_keyframe_finish_time < current_keyframe_time + dt;
    bool const on_last_keyframe = current_keyframe_index == (path_keyframes.size() - 1);
    bool const animation_finished = keyframe_finished && on_last_keyframe;

    if(keyframe_finished)
    {
        current_keyframe_time = current_keyframe_finish_time - (current_keyframe_time + dt);
        current_keyframe_index = animation_finished ? 0 : current_keyframe_index + 1;
    } else {
        current_keyframe_time = current_keyframe_time + dt;
    }

    auto const & current_keyframe = path_keyframes.at(current_keyframe_index);
    f32 const t = current_keyframe_time / current_keyframe.transition_time;

    f32 w0 = static_cast<f32>(glm::pow(1.0f - t, 3));
    f32 w1 = static_cast<f32>(glm::pow(1.0f - t, 2) * 3.0f * t);
    f32 w2 = static_cast<f32>((1.0f - t) * 3 * t * t);
    f32 w3 = static_cast<f32>(t * t * t);

    info.pos = 
        w0 * current_keyframe.start_position +
        w1 * current_keyframe.first_control_point +
        w2 * current_keyframe.second_control_point +
        w3 * current_keyframe.end_position;

    auto const view_quat = glm::slerp(current_keyframe.start_rotation, current_keyframe.end_rotation, t);
    info.view = glm::toMat4(view_quat) * glm::translate(glm::identity<glm::mat4x4>(), glm::vec3(-info.pos[0], -info.pos[1], -info.pos[2]));
    info.viewproj = info.proj * info.view;
}