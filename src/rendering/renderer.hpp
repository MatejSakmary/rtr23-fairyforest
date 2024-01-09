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
        ComputePipeline particules_pass = {}; // Loanie
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
        BufferId particule_SSBO_in = {}; // Loanie
        BufferId particule_SSBO_out = {}; // Loanie
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
        void create_particules_resources(); // Loanie

        std::shared_ptr<Context> context = {};
		Pipelines pipelines = {};
		Images images = {};
		Buffers buffers = {};

        u32 frame_index = {};
        SamplerId repeat_sampler = {};
    };
} // namespace ff