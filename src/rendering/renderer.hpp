#pragma once
#include "../backend/backend.hpp"
#include "../context.hpp"
#include "../scene/scene.hpp"

struct CameraInfo
{
    f32mat4x4 view = {};
    f32mat4x4 proj = {};
    f32mat4x4 viewproj = {};
    f32vec3 pos = {};
    f32vec3 up = {};
};

namespace ff
{
    struct Renderer
    {
      public:
        Renderer() = default;
        Renderer(std::shared_ptr<Context> context);
        ~Renderer();

        void draw_frame(SceneDrawCommands const & draw_commands, CameraInfo const & camera_info, f32 delta_time);
        void resize();

      private:
        std::shared_ptr<Context> context = {};
        RasterPipeline triangle_pipeline = {};
        ComputePipeline compute_test_pipeline = {};
		BufferId compute_test_buffer = {};
        u32 frame_index = {};
        SamplerId repeat_sampler = {};
        ImageId depth_buffer = {};
    };
} // namespace ff