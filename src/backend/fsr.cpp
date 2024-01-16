#include "fsr.hpp"
#include <cstring>

namespace ff
{
    Fsr::Fsr(CreateFsrInfo const & info) : device(info.device), fsr_info(info.fsr_info)
    {
        context_description.maxRenderSize.width = fsr_info.render_resolution.x;
        context_description.maxRenderSize.height = fsr_info.render_resolution.y;
        context_description.displaySize.width = fsr_info.display_resolution.x;
        context_description.displaySize.height = fsr_info.display_resolution.y;

        u32 const scratch_buffer_size = ffxFsr2GetScratchMemorySizeVK(info.device->vulkan_physical_device);
        scratch_buffer.resize(scratch_buffer_size);

        {
            FfxErrorCode const err = ffxFsr2GetInterfaceVK(
                &context_description.callbacks,
                scratch_buffer.data(),
                scratch_buffer_size,
                info.device->vulkan_physical_device,
                vkGetDeviceProcAddr);
            if (err != FFX_OK)
            {
                BACKEND_LOG("[ERROR][Fsr::Fsr()] FSR2 Failed to create Vulkan interface");
                throw std::runtime_error("[ERROR][Fsr::Fsr()] FSR2 Failed to create Vulkan interface");
            }
        }

        context_description.device = ffxGetDeviceVK(info.device->vulkan_device);
        context_description.flags = FFX_FSR2_ENABLE_DEPTH_INFINITE | FFX_FSR2_ENABLE_DEPTH_INVERTED | FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;

        {
            FfxErrorCode const err = ffxFsr2ContextCreate(&context, &context_description);
            if (err != FFX_OK)
            {
                BACKEND_LOG("[ERROR][Fsr::Fsr()] FSR Failed to create context");
                throw std::runtime_error("[ERROR][Fsr::Fsr()] FSR Failed to create context");
            }
        }
    }

    auto Fsr::get_jitter(u64 const index) const -> f32vec2
    {
        f32vec2 result;
        i32 const jitter_phase_count = ffxFsr2GetJitterPhaseCount(
            static_cast<i32>(fsr_info.render_resolution.x),
            static_cast<i32>(fsr_info.display_resolution.x));
        ffxFsr2GetJitterOffset(&result.x, &result.y, static_cast<i32>(index), jitter_phase_count);
        return result;
    }

    void Fsr::upscale(UpscaleInfo const & info)
    {
        /// NOTE: scratch_buffer is only empty when this object is default constructed
        if (scratch_buffer.empty())
        {
            BACKEND_LOG("[ERROR][Fsr::upscale()] FSR Trying to upscale but Fsr state was not initialized");
            throw std::runtime_error("[ERROR][Fsr::upscale()] FSR Trying to upscale but Fsr state was not initialized");
        }

        if (!device->resource_table->images.is_id_valid(info.color_id))
        {
            BACKEND_LOG("[ERROR][Fsr::upscale()] Received invalid color image ID");
            throw std::runtime_error("[ERROR][Fsr::upscale()] Received invalid color image ID");
        }
        if (!device->resource_table->images.is_id_valid(info.depth_id))
        {
            BACKEND_LOG("[ERROR][Fsr::upscale()] Received invalid depth image ID");
            throw std::runtime_error("[ERROR][Fsr::upscale()] Received invalid depth image ID");
        }
        if (!device->resource_table->images.is_id_valid(info.target_id))
        {
            BACKEND_LOG("[ERROR][Fsr::upscale()] Received invalid target image ID");
            throw std::runtime_error("[ERROR][Fsr::upscale()] Received invalid target image ID");
        }
        if (!device->resource_table->images.is_id_valid(info.motion_vectors_id))
        {
            BACKEND_LOG("[ERROR][Fsr::upscale()] Received invalid motion vectors image ID");
            throw std::runtime_error("[ERROR][Fsr::upscale()] Received invalid motion vectors image ID");
        }

        auto const & color = device->resource_table->images.slot(info.color_id);
        auto const & depth = device->resource_table->images.slot(info.depth_id);
        auto const & motion_vectors = device->resource_table->images.slot(info.motion_vectors_id);
        auto const & target = device->resource_table->images.slot(info.target_id);

        wchar_t fsr_input_color[] = L"FSR2_InputColor";
        wchar_t fsr_input_depth[] = L"FSR2_InputDepth";
        wchar_t fsr_input_motion_vectors[] = L"FSR2_InputMotionVectors";
        wchar_t fsr_input_exposure[] = L"FSR2_InputExposure";
        wchar_t fsr_output_upscaled_color[] = L"FSR2_OutputUpscaledColor";

        FfxFsr2DispatchDescription dispatch_description = {};
        dispatch_description.commandList = ffxGetCommandListVK(info.command_buffer.buffer);

        dispatch_description.color = ffxGetTextureResourceVK(
            &context, color->image, color->image_view,
            color->image_info.extent.width, color->image_info.extent.height,
            color->image_info.format, fsr_input_color);

        dispatch_description.depth = ffxGetTextureResourceVK(
            &context, depth->image, depth->image_view,
            depth->image_info.extent.width, depth->image_info.extent.height,
            depth->image_info.format, fsr_input_depth);

        dispatch_description.motionVectors = ffxGetTextureResourceVK(
            &context, motion_vectors->image, motion_vectors->image_view,
            motion_vectors->image_info.extent.width, motion_vectors->image_info.extent.height,
            motion_vectors->image_info.format, fsr_input_motion_vectors);

        dispatch_description.exposure = ffxGetTextureResourceVK(&context, nullptr, nullptr, 1, 1, VK_FORMAT_UNDEFINED, fsr_input_exposure);

        dispatch_description.output = ffxGetTextureResourceVK(
            &context, target->image, target->image_view,
            target->image_info.extent.width, target->image_info.extent.height,
            target->image_info.format, fsr_output_upscaled_color, FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        dispatch_description.jitterOffset.x = info.jitter.x;
        dispatch_description.jitterOffset.y = info.jitter.y;
        dispatch_description.motionVectorScale.x = static_cast<f32>(fsr_info.render_resolution.x);
        dispatch_description.motionVectorScale.y = static_cast<f32>(fsr_info.render_resolution.y);
        dispatch_description.reset = info.should_reset;
        dispatch_description.enableSharpening = info.should_sharpen;
        dispatch_description.sharpness = info.sharpening;
        dispatch_description.frameTimeDelta = info.delta_time;
        dispatch_description.preExposure = 1.0f;
        dispatch_description.renderSize.width = fsr_info.render_resolution.x;
        dispatch_description.renderSize.height = fsr_info.render_resolution.y;
        dispatch_description.cameraFar = info.camera_info.far_plane;
        dispatch_description.cameraNear = info.camera_info.near_plane;
        dispatch_description.cameraFovAngleVertical = info.camera_info.vertical_fov;

        FfxErrorCode const err = ffxFsr2ContextDispatch(&context, &dispatch_description);
        if (err != FFX_OK)
        {
            BACKEND_LOG("[ERROR][Fsr::upscale()] FSR2 failed to dispatch");
            throw std::runtime_error("[ERROR][Fsr::upscale()] FSR2 failed to dispatch");
        }
    }

    Fsr::~Fsr()
    {
        // We are only default constructed
        if (!scratch_buffer.empty())
        {
            FfxErrorCode const err = ffxFsr2ContextDestroy(&context);
            DBG_ASSERT_TRUE_M(err == FFX_OK, "[ERROR][Fsr::Fsr()] FSR Failed to create context");
        }
    }
} // namespace ff