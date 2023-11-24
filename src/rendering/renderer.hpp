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

            void draw_frame();
        private:
            std::shared_ptr<Context> context = {};
    };
}