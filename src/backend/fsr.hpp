#pragma once

#include "core.hpp"
#include "device.hpp"
#include "command_buffer.hpp"

#include <ffx-fsr2-api/ffx_fsr2.h>
#include <ffx-fsr2-api/vk/ffx_fsr2_vk.h>

namespace ff
{
    struct FsrInfo
    {
        u32vec2 render_resolution = {};
        u32vec2 display_resolution = {};
    };

    struct CreateFsrInfo
    {
        FsrInfo fsr_info = {};
        std::shared_ptr<Device> device = {};
    };

    struct UpscaleInfo
    {
        CommandBuffer & command_buffer;
        ImageId color_id = {};
        ImageId depth_id = {};
        ImageId motion_vectors_id = {};
        ImageId target_id = {};

        bool should_reset = {};
        f32 delta_time = {};
        f32vec2 jitter = {};

        bool should_sharpen = {};
        f32 sharpening = 0.0f;

        struct CameraInfo
        {
            f32 near_plane = {};
            f32 far_plane = {};
            f32 vertical_fov = {};
        } camera_info;
    };

    struct Fsr
    {
      public:
        Fsr() = default;
        Fsr(Fsr && other) = default;
        Fsr & operator=(Fsr && other) = default;

        Fsr(Fsr const &) = delete;
        Fsr & operator=(Fsr const &) = delete;

        Fsr(CreateFsrInfo const & info);
        auto get_jitter(u64 const index) const -> f32vec2;
        void upscale(UpscaleInfo const & info);
        ~Fsr();

      private:
        std::shared_ptr<Device> device = {};
        bool initialized = {};
        FfxFsr2Context context = {};
        FfxFsr2ContextDescription context_description = {};
        FsrInfo fsr_info;

        std::vector<std::byte> scratch_buffer = {};
    };
} // namespace ff