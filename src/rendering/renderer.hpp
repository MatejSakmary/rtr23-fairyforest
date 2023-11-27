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

        void draw_frame(SceneDrawCommands const & draw_commands, CameraInfo const & camera_info);
        void resize();

      private:
        std::shared_ptr<Context> context = {};
        Pipeline triangle_pipeline = {};
        u32 frame_index = {};
        ImageId depth_buffer = {};
    };
} // namespace ff