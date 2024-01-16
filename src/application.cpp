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
            .start_rotation = {-0.5992529, 0.6082096, 0.37080798, 0.36534727},
            .end_rotation = {0.6747596, -0.66948044, -0.21878937, -0.22051468},
            .start_position = {952.99524, -306.82825, 23},
            .first_control_point = {892.64, -295.54, 23},
            .second_control_point = {876.71, -267.25, 23},
            .end_position = {876.5687, -237.30687, 23},
            .transition_time = 7.0f},
        AnimationKeyframe{
            .start_rotation = {0.6747596, -0.66948044, -0.21878937, -0.22051468},
            .end_rotation = {0.56785387, -0.56538224, -0.42209232, -0.42393756},
            .start_position = {876.5687, -237.30687, 23},
            .first_control_point = {876.61, -205.94, 23},
            .second_control_point = {845.67, -160.87, 23},
            .end_position = {813.7164, -170.50764, 23},
            .transition_time = 6.0f},
        AnimationKeyframe{
            .start_rotation = {0.56785387, -0.56538224, -0.42209232, -0.42393756},
            .end_rotation = {-0.40847868, 0.4017618, 0.5747157, 0.58432406},
            .start_position = {813.7164, -170.50764, 23},
            .first_control_point = {785.1, -178.46, 23},
            .second_control_point = {782.24, -202.53, 23}, // 767.62, -198.09, 23
            .end_position = {751.7979, -200.00076, 23},
            .transition_time = 5.0f},
        AnimationKeyframe{
            .start_rotation = {-0.40847868, 0.4017618, 0.5747157, 0.58432406},
            .end_rotation = {-0.57182485, 0.5723245, 0.41579014, 0.4154271},
            .start_position = {751.7979, -200.00076, 23},
            .first_control_point = {723.9, -197.88, 23},
            .second_control_point = {579.78, -153.06, 23},
            .end_position = {565.22064, -148.87428, 23},
            .transition_time = 8.0f},
        AnimationKeyframe{
            .start_rotation = {-0.57182485, 0.5723245, 0.41579014, 0.4154271},
            .end_rotation = {0.6334516, -0.61332214, -0.32816905, -0.3389397},
            .start_position = {565.22064, -148.87428, 23},
            .first_control_point = {550.3, -143.77, 23},
            .second_control_point = {201.89, 8.115, 23},
            .end_position = {221.9193, 35.31217, 23},
            .transition_time = 16.0f},
        AnimationKeyframe{
            .start_rotation = {0.6334516, -0.61332214, -0.32816905, -0.3389397},
            .end_rotation = {-0.022903541, 0.021171803, 0.67847013, 0.7339655},
            .start_position = {221.9193, 35.31217, 23},
            .first_control_point = {372.68, 223, 23},
            .second_control_point = {245.5, 341.98, 23},
            .end_position = {76.26797, 337.3477, 23},
            .transition_time = 30.0f},
    };
    camera = CinematicCamera(keyframes, *window);
    std::filesystem::path const DEFAULT_ROOT_PATH = ".\\assets";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "forest\\forest.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "forest_leaves_twofaced\\forest_leaves_twofaced.gltf";
    std::filesystem::path const DEFAULT_SCENE_PATH = "forest_new\\forest_leaves_twofaced.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "instanced_cubes\\instanced_cubes.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "old_sponza\\old_sponza.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "new_sponza\\new_sponza.gltf";
    // std::filesystem::path const DEFAULT_SCENE_PATH = "cube_on_plane\\cube.gltf";

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
        RenderEntity & r_ent = *scene->_render_entities.slot(r_id);
        APP_LOG(fmt::format("[INFO]Application::Application()] Loading Scene Assets \"{}\" Success",
                            (DEFAULT_ROOT_PATH / DEFAULT_SCENE_PATH).string()));
        // r_ent.transform = glm::mat4x3(
        //                       glm::vec3(1.0f, 0.0f, 0.0f),
        //                       glm::vec3(0.0f, 0.0f, 1.0f),
        //                       glm::vec3(0.0f, 1.0f, 0.0f),
        //                       glm::vec3(0.0f, 0.0f, 0.0f)) *
        //                   1.0f;
        // ) * 100'000'000.0f;
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
        if (window->window_state->resized)
        {
            renderer->resize();
        }
        update();
        camera.update_position(*window, delta_time);
        commands.no_ao = no_ao;
        commands.no_albedo = no_albedo;
        commands.no_shadows = no_shadows;
        commands.force_ao = force_ao;
        // renderer->draw_frame(commands, camera_controller.cam_info, delta_time);
        renderer->draw_frame(commands, camera.info, delta_time);
        keep_running &= !static_cast<bool>(glfwWindowShouldClose(window->glfw_handle));
    }
    return 0;
}

void Application::update()
{
    if (window->size.x == 0 || window->size.y == 0)
    {
        return;
    }
    if(window->key_just_pressed(GLFW_KEY_0))
    {
        no_albedo = 1 - no_albedo;
    }
    if(window->key_just_pressed(GLFW_KEY_1))
    {
        no_shadows = 1 - no_shadows;
    }
    if(window->key_just_pressed(GLFW_KEY_2))
    {
        no_ao = 1 - no_ao;
        if(window->key_pressed(GLFW_KEY_LEFT_SHIFT))
        {
            no_ao = 0;
            force_ao = 1 - force_ao;
        }
    }
    if(window->key_just_pressed(GLFW_KEY_3))
    {
        no_normal_maps = 1 - no_normal_maps;
    }
    camera_controller.process_input(*window, delta_time);
    camera_controller.update_matrices(*window);
}

Application::~Application()
{
}