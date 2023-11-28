#include "application.hpp"

void CameraController::process_input(Window & window, f32 dt)
{
    f32 speed = window.key_pressed(GLFW_KEY_LEFT_SHIFT) ? translation_speed * 4.0f : translation_speed;
    speed = window.key_pressed(GLFW_KEY_LEFT_CONTROL) ? speed * 0.25f : speed;

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
    cam_info.viewproj = cam_info.proj * cam_info.view;
    cam_info.pos = position;
    cam_info.up = up;
    glm::vec3 ws_ndc_corners[2][2][2];
    glm::mat4 inv_view_proj = glm::inverse(cam_info.proj * cam_info.view);
    for (u32 z = 0; z < 2; ++z)
    {
        for (u32 y = 0; y < 2; ++y)
        {
            for (u32 x = 0; x < 2; ++x)
            {
                glm::vec3 corner = glm::vec3((glm::vec2(x, y) - 0.5f) * 2.0f, 1.0f - z * 0.5f);
                glm::vec4 proj_corner = inv_view_proj * glm::vec4(corner, 1);
                ws_ndc_corners[x][y][z] = glm::vec3(proj_corner) / proj_corner.w;
            }
        }
    }
}

Application::Application()
    : keep_running(true),
      window{std::make_unique<Window>(1920, 1080, "Fairy Forest")},
      context{std::make_shared<Context>(window->get_handle())},
      renderer{std::make_unique<ff::Renderer>(context)},
      scene{std::make_unique<Scene>(context->device)},
      asset_processor{std::make_unique<AssetProcessor>(context->device)}

{
    std::filesystem::path const DEFAULT_ROOT_PATH = ".\\assets";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "medieval_battle\\medieval_battle_gltf\\medieval_battle.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "instanced_cubes\\instanced_cubes.gltf";
    std::filesystem::path const DEFAULT_SCENE_PATH = "forest\\forest.gltf";

    auto const result = scene->load_manifest_from_gltf(DEFAULT_ROOT_PATH, DEFAULT_SCENE_PATH);
    if (Scene::LoadManifestErrorCode const * err = std::get_if<Scene::LoadManifestErrorCode>(&result))
    {
        APP_LOG(fmt::format("[WARN][Application::Application()] Loading \"{}\" Error: {}",
                            (DEFAULT_ROOT_PATH / DEFAULT_SCENE_PATH).string(),
                            Scene::to_string(*err)));
    }
    auto const load_result = asset_processor->load_all(*scene);
    if (load_result != AssetProcessor::AssetLoadResultCode::SUCCESS)
    {
        APP_LOG(fmt::format("[INFO]Application::Application()] Loading Scene Assets \"{}\" Error: {}",
                            (DEFAULT_ROOT_PATH / DEFAULT_SCENE_PATH).string(),
                            AssetProcessor::to_string(load_result)));
    }
    else
    {
        auto const r_id = std::get<RenderEntityId>(result);
        RenderEntity& r_ent = *scene->_render_entities.slot(r_id);
        r_ent.transform = glm::mat4x3(
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f)
        ) * 100.0f;
        // ) * 100'000'000.0f;
        APP_LOG(fmt::format("[INFO]Application::Application()] Loading Scene Assets \"{}\" Success",
                            (DEFAULT_ROOT_PATH / DEFAULT_SCENE_PATH).string()));
    }
    asset_processor->record_gpu_load_processing_commands(*scene);
    commands = scene->record_scene_draw_commands();
    last_time_point = std::chrono::steady_clock::now();
}

using FpMilliseconds = std::chrono::duration<float, std::chrono::milliseconds::period>;
auto Application::run() -> i32
{
    while (keep_running)
    {
        auto new_time_point = std::chrono::steady_clock::now();
        delta_time = std::chrono::duration_cast<FpMilliseconds>(new_time_point - last_time_point).count() * 0.001f;
        last_time_point = new_time_point;
        window->update(delta_time);
        if (window->window_state->resized) { renderer->resize(); }
        update();

        renderer->draw_frame(commands, camera_controller.cam_info);
        keep_running &= !static_cast<bool>(glfwWindowShouldClose(window->glfw_handle));
    }
    return 0;
}

void Application::update()
{
    if (window->size.x == 0 || window->size.y == 0) { return; }
    camera_controller.process_input(*window, delta_time);
    camera_controller.update_matrices(*window);
}

Application::~Application()
{
}