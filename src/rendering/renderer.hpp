#pragma once
#include "../backend/backend.hpp"
#include "../context.hpp"
#include "../scene/scene.hpp"

struct CameraInfo
{
	ff::UpscaleInfo::CameraInfo fsr_cam_info;
    f32mat4x4 view = {};
    f32mat4x4 proj = {};
    f32mat4x4 viewproj = {};
    f32vec3 pos = {};
    f32vec3 up = {};
	f32vec3 frust_right_offset;
	f32vec3 frust_top_offset;
	f32vec3 frust_front;
};

namespace ff
{
	struct Pipelines
	{
		RasterPipeline prepass = {};
		RasterPipeline prepass_discard = {};
		RasterPipeline shadowmap_pass = {};
		RasterPipeline shadowmap_pass_discard = {};
		RasterPipeline main_pass = {};

		ComputePipeline first_depth_pass = {};
		ComputePipeline subseq_depth_pass = {};
		ComputePipeline write_shadow_matrices = {};
		ComputePipeline first_esm_pass = {};
		ComputePipeline second_esm_pass = {};
		ComputePipeline ssao_pass = {};
		ComputePipeline fog_pass = {};
	};

	struct Images
	{
		ImageId ss_normals = {};
		ImageId motion_vectors = {};
		ImageId ambient_occlusion = {};
		ImageId ssao_kernel_noise = {};
        ImageId depth = {};

		ImageId shadowmap_cascades = {};
		ImageId esm_tmp_cascades = {};
		ImageId esm_cascades = {};

		ImageId offscreen = {};
		ImageId fsr_target = {};
	};

	struct Buffers
	{
        BufferId ssao_kernel = {};
		BufferId camera_info = {};
		BufferId depth_limits = {};
		BufferId cascade_data = {};
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
		Fsr fsr = {};

        u32 frame_index = {};
		f32 frame_time = {};
        SamplerId repeat_sampler = {};
		SamplerId clamp_sampler = {};
		SamplerId no_mip_sampler = {};

		//TODO(msakmary) Hacky...
		f32vec2 jitter = {};
		f32mat4x4 prev_view_projection = {};

    	static constexpr std::array<u32vec2, 8> resolution_table{
        	u32vec2{1u,1u}, u32vec2{2u,1u}, u32vec2{2u,2u}, u32vec2{2u,2u},
        	u32vec2{3u,2u}, u32vec2{3u,2u}, u32vec2{3u,3u}, u32vec2{3u,3u},
    	};
    };
} // namespace ff