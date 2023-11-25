#pragma once
#include "../backend/backend.hpp"
#include "../context.hpp"

namespace ff
{
    struct Renderer
    {
        public:
            Renderer() = default;
            Renderer(std::shared_ptr<Context> context);
            ~Renderer();

            void draw_frame();
            void resize();
        private:
            std::shared_ptr<Context> context = {};
            Pipeline triangle_pipeline = {};
            ImageId test_image = {};
            BufferId test_buffer = {};
    };
}