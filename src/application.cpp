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
            .start_rotation = {-0.21702081, 0.12821823, 0.4922416, 0.83316284},
            .end_rotation = {-0.13598983, 0.23248576, 0.8312773, 0.486246},
            .start_position = {-2.3502305, 12.295344, 0.5642019},
            .first_control_point = {-2.3502305, 12.295344, 0.5642019},
            .second_control_point = {-2.3502305, 12.295344, 0.5642019},
            .end_position = {-2.3502305, 12.295344, 0.5642019},
            .transition_time = 4.0f},
        AnimationKeyframe{
            .start_rotation = {-0.13598983, 0.23248576, 0.8312773, 0.486246},
            .end_rotation = {-0.22093865, 0.22600928, 0.67842716, 0.6632063},
            .start_position = {-2.3502305, 12.295344, 0.5642019},
            .first_control_point = {-2.3502305, 12.295344, 0.5642019},
            .second_control_point = {-2.3502305, 12.295344, 0.5642019},
            .end_position = {-2.3502305, 12.295344, 0.5642019},
            .transition_time = 3.0f},
        AnimationKeyframe{
            .start_rotation = {-0.22093865, 0.22600928, 0.67842716, 0.6632063},
            .end_rotation = {-0.3243918, 0.33562425, 0.63590086, 0.61461896},
            .start_position = {-2.3502305, 12.295344, 0.5642019},
            .first_control_point = {-2.3502305, 12.295344, 0.5642019},
            .second_control_point = {-4.1274786, 10.174047, 0.5175211},
            .end_position = {-4.1274786, 10.174047, 0.5175211},
            .transition_time = 4.0f},
        AnimationKeyframe{
            .start_rotation = {-0.3243918, 0.33562425, 0.63590086, 0.61461896},
            .end_rotation = {-0.31515124, 0.30808055, 0.62748915, 0.6418905},
            .start_position = {-4.1274786, 10.174047, 0.5175211},
            .first_control_point = {-4.1274786, 10.174047, 0.5175211},
            .second_control_point = {-4.1274786, 10.174047, 0.5175211},
            .end_position = {-4.1274786, 10.174047, 0.5175211},
            .transition_time = 1.0f},
        AnimationKeyframe{
            .start_rotation = {-0.31515124, 0.30808055, 0.62748915, 0.6418905},
            .end_rotation = {-0.30329093, 0.31324527, 0.64654374, 0.6259979},
            .start_position = {-4.1274786, 10.174047, 0.5175211},
            .first_control_point = {-4.1274786, 10.174047, 0.5175211},
            .second_control_point = {-5.234389, 9.420338, 0.50579655},
            .end_position = {-5.234389, 9.420338, 0.50579655},
            .transition_time = 4.0f},
        AnimationKeyframe{
            .start_rotation = {-0.30329093, 0.31324527, 0.64654374, 0.6259979},
            .end_rotation = {0.06453691, -0.06113628, 0.6849984, 0.7231006},
            .start_position = {-5.234389, 9.420338, 0.50579655},
            .first_control_point = {-5.234389, 9.420338, 0.50579655},
            .second_control_point = {-5.155626, 9.009403, 0.5258944},
            .end_position = {-5.155626, 9.009403, 0.5258944},
            .transition_time = 2.0f},
        AnimationKeyframe{
            .start_rotation = {0.06453691, -0.06113628, 0.6849984, 0.7231006},
            .end_rotation = {-0.04376447, 0.041931953, 0.69055617, 0.7207348},
            .start_position = {-5.155626, 9.009403, 0.5258944},
            .first_control_point = {-5.155626, 9.009403, 0.5258944},
            .second_control_point = {-4.945674, 6.9060564, 0.46652806},
            .end_position = {-4.945674, 6.9060564, 0.46652806},
            .transition_time = 4.0f},
        AnimationKeyframe{
            .start_rotation = {-0.04376447, 0.041931953, 0.69055617, 0.7207348},
            .end_rotation = {0.13680285, -0.16274667, 0.7479827, 0.62874514},
            .start_position = {-4.945674, 6.9060564, 0.46652806},
            .first_control_point = {-4.945674, 6.9060564, 0.46652806},
            .second_control_point = {-5.1979656, 6.3712234, 0.4429053},
            .end_position = {-5.1979656, 6.3712234, 0.4429053},
            .transition_time = 1.5f},
        AnimationKeyframe{
            .start_rotation = {0.13680285, -0.16274667, 0.7479827, 0.62874514},
            .end_rotation = {0.37128294, -0.36231983, 0.5970831, 0.61185384},
            .start_position = {-5.1979656, 6.3712234, 0.4429053},
            .first_control_point = {-5.1979656, 6.3712234, 0.4429053},
            .second_control_point = {-4.4847164, 4.914666, 0.6918752},
            .end_position = {-4.4847164, 4.914666, 0.6918752},
            .transition_time = 4.0f},
        AnimationKeyframe{
            .start_rotation = {0.37128294, -0.36231983, 0.5970831, 0.61185384},
            .end_rotation = {0.045614913, -0.04565473, 0.7059405, 0.70532477},
            .start_position = {-4.4847164, 4.914666, 0.6918752},
            .first_control_point = {-4.4847164, 4.914666, 0.6918752},
            .second_control_point = {-4.4847164, 4.914666, 0.6918752},
            .end_position = {-4.4847164, 4.914666, 0.6918752},
            .transition_time = 4.0f},
        AnimationKeyframe{
            .start_rotation = {0.045614913, -0.04565473, 0.7059405, 0.70532477},
            .end_rotation = {0.27070943, -0.2653297, 0.6477386, 0.66087174},
            .start_position = {-4.4847164, 4.914666, 0.6918752},
            .first_control_point = {-4.4847164, 4.914666, 0.6918752},
            .second_control_point = {-4.4847164, 4.914666, 0.6918752},
            .end_position = {-4.4847164, 4.914666, 0.6918752},
            .transition_time = 4.0f},
        AnimationKeyframe{
            .start_rotation = {0.27070943, -0.2653297, 0.6477386, 0.66087174},
            .end_rotation = {0.22521454, -0.21038365, 0.6494082, 0.69518787},
            .start_position = {-4.4847164, 4.914666, 0.6918752},
            .first_control_point = {-4.4847164, 4.914666, 0.6918752},
            .second_control_point = {-2.416358, 3.1896625, 0.8320923},
            .end_position = {-2.416358, 3.1896625, 0.8320923},
            .transition_time = 5.0f},
        AnimationKeyframe{
            .start_rotation = {0.22521454, -0.21038365, 0.6494082, 0.69518787},
            .end_rotation = {0.23222286, -0.22523615, 0.65878624, 0.6792215},
            .start_position = {-2.416358, 3.1896625, 0.8320923},
            .first_control_point = {-2.416358, 3.1896625, 0.8320923},
            .second_control_point = {-1.5272771, 1.4759452, 0.6429163},
            .end_position = {-1.5272771, 1.4759452, 0.6429163},
            .transition_time = 5.0f},
        AnimationKeyframe{
            .start_rotation = {0.23222286, -0.22523615, 0.65878624, 0.6792215},
            .end_rotation = {0.014151443, -0.013618239, 0.69326806, 0.7204122},
            .start_position = {-1.5272771, 1.4759452, 0.6429163},
            .first_control_point = {-1.5272771, 1.4759452, 0.6429163},
            .second_control_point = {-2.8304698, -1.1964859, 0.8168586},
            .end_position = {-2.8304698, -1.1964859, 0.8168586},
            .transition_time = 6.0f},
        AnimationKeyframe{
            .start_rotation = {0.014151443, -0.013618239, 0.69326806, 0.7204122},
            .end_rotation = {0.25225312, -0.25513074, 0.66376173, 0.6562752},
            .start_position = {-2.8304698, -1.1964859, 0.8168586},
            .first_control_point = {-2.8304698, -1.1964859, 0.8168586},
            .second_control_point = {-2.237842, -4.617661, 0.48798138},
            .end_position = {-2.237842, -4.617661, 0.48798138},
            .transition_time = 6.0f},
        AnimationKeyframe{
            .start_rotation = {0.25225312, -0.25513074, 0.66376173, 0.6562752},
            .end_rotation = {0.62975484, -0.6226495, 0.32654923, 0.33027557},
            .start_position = {-2.237842, -4.617661, 0.48798138},
            .first_control_point = {-2.237842, -4.617661, 0.48798138},
            .second_control_point = {-1.34, -7.466, 0.48775858},
            .end_position = {0.31615227, -7.537089, 0.48775858},
            .transition_time = 6.0f},
        AnimationKeyframe{
            .start_rotation = {0.62975484, -0.6226495, 0.32654923, 0.33027557},
            .end_rotation = {-0.69398654, 0.7198989, -0.008159298, -0.007865609},
            .start_position = {0.31615227, -7.537089, 0.48775858},
            .first_control_point = {0.31615227, -7.537089, 0.48775858},
            .second_control_point = {0.31615227, -7.537089, 0.48775858},
            .end_position = {0.31615227, -7.537089, 0.48775858},
            .transition_time = 2.0f},
        AnimationKeyframe{
            .start_rotation = {-0.69398654, 0.7198989, -0.008159298, -0.007865609},
            .end_rotation = {0.70843226, -0.50485283, -0.28622785, -0.40164778},
            .start_position = {0.31615227, -7.537089, 0.48775858},
            .first_control_point = {1.33, -7.64, 0.48775858},
            .second_control_point = {3.6630793, -5.4383855, 1.5184249},
            .end_position = {3.6630793, -5.4383855, 1.5184249},
            .transition_time = 7.0f},
        AnimationKeyframe{
            .start_rotation = {0.70843226, -0.50485283, -0.28622785, -0.40164778},
            .end_rotation = {-0.5165925, 0.3045996, 0.40644264, 0.6893154},
            .start_position = {3.6630793, -5.4383855, 1.5184249},
            .first_control_point = {3.8, -5.15, 1.5184249},
            .second_control_point = {5.3, -2.7, 2.5592735},
            .end_position = {4.8405137, -1.1420408, 2.5592735},
            .transition_time = 6.0f},
        AnimationKeyframe{
            .start_rotation = {-0.5165925, 0.3045996, 0.40644264, 0.6893154},
            .end_rotation = {0.29515088, -0.19964395, 0.5235001, 0.7739354},
            .start_position = {4.8405137, -1.1420408, 2.5592735},
            .first_control_point = {4.14, 2.2, 2.5592735},
            .second_control_point = {-0.6, 3.54, 2.8520598},
            .end_position = {-2.8672833, 2.5240207, 2.8520598},
            .transition_time = 6.0f},
        AnimationKeyframe{
            .start_rotation = {0.29515088, -0.19964395, 0.5235001, 0.7739354},
            .end_rotation = {0.35389286, -0.29959208, 0.57246864, 0.6762279},
            .start_position = {-2.8672833, 2.5240207, 2.8520598},
            .first_control_point = {-3.4, 2.26, 2.8520598},
            .second_control_point = {-3.38, 2.38, 2.0330203},
            .end_position = {-4.410391, 2.1541417, 2.0330203},
            .transition_time = 6.0f},
        AnimationKeyframe{
            .start_rotation = {0.35389286, -0.29959208, 0.57246864, 0.6762279},
            .end_rotation = {0.045614913, -0.04565473, 0.7059405, 0.70532477},
            .start_position = {-4.410391, 2.1541417, 2.0330203},
            .first_control_point = {-4.410391, 2.1541417, 2.0330203},
            .second_control_point = {-10.737522, 6.5435715, 3.3999836},
            .end_position = {-10.737522, 6.5435715, 3.3999836},
            .transition_time = 10.0f},
        AnimationKeyframe{
            .start_rotation = {0.3564937, -0.2964923, 0.56654567, 0.68119794},
            .end_rotation = {0.3564937, -0.2964923, 0.56654567, 0.68119794},
            .start_position = {-10.737522, 6.5435715, 3.3999836},
            .first_control_point = {-10.737522, 6.5435715, 3.3999836},
            .second_control_point = {-10.737522, 6.5435715, 3.3999836},
            .end_position = {-10.737522, 6.5435715, 3.3999836},
            .transition_time = 5.0f},
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
        if (!use_manual_camera)
        {
            camera.update_position(*window, delta_time);
        }
        commands.no_ao = no_ao;
        commands.no_albedo = no_albedo;
        commands.no_shadows = no_shadows;
        commands.force_ao = force_ao;
        commands.no_normal_maps = no_normal_maps;
        commands.reset_fsr = reset_fsr;
        commands.no_fog = no_fog;
        commands.no_fsr = no_fsr;
        reset_fsr = false;
        if (use_manual_camera)
        {
            renderer->draw_frame(commands, camera_controller.cam_info, delta_time);
        }
        else
        {
            renderer->draw_frame(commands, camera.info, delta_time);
        }
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
    if (window->key_just_pressed(GLFW_KEY_0))
    {
        no_albedo = 1 - no_albedo;
    }
    if (window->key_just_pressed(GLFW_KEY_1))
    {
        no_shadows = 1 - no_shadows;
    }
    if (window->key_just_pressed(GLFW_KEY_2))
    {
        no_ao = 1 - no_ao;
        if (window->key_pressed(GLFW_KEY_LEFT_SHIFT))
        {
            no_ao = 0;
            force_ao = 1 - force_ao;
        }
    }
    if (window->key_just_pressed(GLFW_KEY_3))
    {
        no_normal_maps = 1 - no_normal_maps;
    }
    if (window->key_just_pressed(GLFW_KEY_4))
    {
        no_fog = !no_fog;
    }
    if (window->key_just_pressed(GLFW_KEY_5))
    {
        renderer->change_fsr_scaling(1.0f);
        reset_fsr = true;
    }
    if (window->key_just_pressed(GLFW_KEY_6))
    {
        renderer->change_fsr_scaling(1.5f);
        reset_fsr = true;
    }
    if (window->key_just_pressed(GLFW_KEY_7))
    {
        renderer->change_fsr_scaling(1.75f);
        reset_fsr = true;
    }
    if (window->key_just_pressed(GLFW_KEY_8))
    {
        renderer->change_fsr_scaling(2.0f);
        reset_fsr = true;
    }
    if (window->key_just_pressed(GLFW_KEY_9))
    {
        renderer->change_fsr_scaling(3.0f);
        reset_fsr = true;
    }
    if (window->key_just_pressed(GLFW_KEY_MINUS))
    {
        renderer->change_fsr_scaling(1.0f);
        no_fsr = !no_fsr;
    }
    if (window->key_just_pressed(GLFW_KEY_M))
    {
        use_manual_camera = !use_manual_camera;
        reset_fsr = true;
    }
    if (use_manual_camera)
    {
        camera_controller.process_input(*window, delta_time);
    }
    camera_controller.update_matrices(*window);
}

Application::~Application()
{
}