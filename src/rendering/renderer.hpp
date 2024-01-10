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
	struct Pipelines
	{
		RasterPipeline prepass = {};
		RasterPipeline main_pass = {};

		ComputePipeline ssao_pass = {};
        ComputePipeline particles_pass = {};
	};

	struct Images
	{
		ImageId ss_normals = {};
		ImageId ambient_occlusion = {};
        ImageId depth = {};
	};

	struct Buffers
	{
        BufferId ssao_kernel = {};
        BufferId particles_in = {}; 
        BufferId particles_out = {};
        BufferId atomic_count = {};
        BufferId last_count = {};
	};

    struct Renderer
    {
      public:
        Renderer() = default;
        Renderer(std::shared_ptr<Context> context);
        ~Renderer();

        void draw_frame(SceneDrawCommands const & draw_commands, CameraInfo const & camera_info, f32 delta_time);
        void resize();

      private:
	  	void create_pipelines();
		void create_resolution_indep_resources();
		void create_resolution_dep_resources();

        std::shared_ptr<Context> context = {};
		Pipelines pipelines = {};
		Images images = {};
		Buffers buffers = {};

        u32 frame_index = {};
        SamplerId repeat_sampler = {};
    };
} // namespace ff