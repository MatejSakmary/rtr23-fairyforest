#include "application.hpp"

Application::Application()
    : keep_running(true),
      window{std::make_unique<Window>(1920, 1080, "Fairy Forest")},
      context{std::make_shared<Context>(window->get_handle())},
      renderer{std::make_unique<ff::Renderer>(context)},
      scene{std::make_unique<Scene>(context->device)},
      asset_processor{std::make_unique<AssetProcessor>(context->device)}

{
    std::filesystem::path const DEFAULT_ROOT_PATH = ".\\assets";
    std::filesystem::path const DEFAULT_SCENE_PATH = "medieval_battle\\medieval_battle_gltf\\medieval_battle.gltf";

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
        APP_LOG(fmt::format("[INFO]Application::Application()] Loading Scene Assets \"{}\" Success",
                            (DEFAULT_ROOT_PATH / DEFAULT_SCENE_PATH).string()));
    }
    asset_processor->record_gpu_load_processing_commands(*scene);
}

auto Application::run() -> i32
{
    while (keep_running)
    {
        window->update(16.6);
        if (window->window_state->resized)
        {
            APP_LOG("[INFO][Application::run()] Window resize detected");
            renderer->resize();
        }
        renderer->draw_frame();
        keep_running &= !static_cast<bool>(glfwWindowShouldClose(window->glfw_handle));
    }
    return 0;
}

void Application::update()
{
}

Application::~Application()
{
}