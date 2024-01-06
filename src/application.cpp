#include "application.hpp"


Application::Application()
    : keep_running(true),
      window{std::make_unique<Window>(1920, 1080, "Fairy Forest")},
      context{std::make_shared<Context>(window->get_handle())},
      renderer{std::make_unique<ff::Renderer>(context)},
      scene{std::make_unique<Scene>(context->device)},
      asset_processor{std::make_unique<AssetProcessor>(context->device)}

{
    std::vector<AnimationKeyframe> keyframes = {
        AnimationKeyframe{
            .start_rotation =       {0.20892, -0.176558, 0.62084, 0.7346},
            .end_rotation =         {0.20892, -0.176558, 0.62084, 0.7346},
            .start_position =       {0.0, 1.0, 10.0},
            .first_control_point =  {-300.0, 1.0, 20.0},
            .second_control_point = {-400.0, 300.0, 30.0},
            .end_position =         {0.0, 1.0, 40.0},
            .transition_time = 3.0f
        },
        AnimationKeyframe{
            .start_rotation =       {0.20892, -0.176558, 0.62084, 0.7346},
            .end_rotation =         {0.74034554, -0.6245334, -0.16035266, -0.19008811},
            .start_position =       {0.0, 1.0, 40.0},
            .first_control_point =  {159.04951, -218.05574, 63.40108},
            .second_control_point = {159.04951, -218.05574, 63.40108},
            .end_position =         {159.04951, -218.05574, 63.40108},
            .transition_time = 3.0f
        },
        AnimationKeyframe{
            .start_rotation =       {0.74034554, -0.6245334, -0.16035266, -0.19008811},
            .end_rotation =         {0.20892, -0.176558, 0.62084, 0.7346},
            .start_position =       {159.04951, -218.05574, 63.40108},
            .first_control_point =  {100.0, 1.0, 50.0},
            .second_control_point = {100.0, 200.0, 45.0},
            .end_position =         {0.0, 1.0, 10.0},
            .transition_time = 3.0f
        },
    };
    camera = CinematicCamera(keyframes, *window);
    std::filesystem::path const DEFAULT_ROOT_PATH = ".\\assets";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "medieval_battle\\medieval_battle_gltf\\medieval_battle.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "instanced_cubes\\instanced_cubes.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "forest\\forest.gltf";
    std::filesystem::path const DEFAULT_SCENE_PATH = "forest_scaled\\forest_scaled.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "new_sponza\\NewSponza_Main_glTF_002.gltf";

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

        camera.update_position(delta_time);
        renderer->draw_frame(commands, camera.info, delta_time);
        // renderer->draw_frame(commands, camera_controller.cam_info, delta_time);
        APP_LOG(fmt::format("{} {} {}", camera_controller.cam_info.pos.x, camera_controller.cam_info.pos.y, camera_controller.cam_info.pos.z));
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