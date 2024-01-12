#include "asset_processor.hpp"

#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fstream>
#include <cstring>
#include <FreeImage.h>
#include <variant>

#pragma region IMAGE_RAW_DATA_LOADING_HELPERS
struct RawImageData
{
    std::vector<std::byte> raw_data;
    std::filesystem::path image_path;
    fastgltf::MimeType mime_type;
};

using RawDataRet = std::variant<std::monostate, AssetProcessor::AssetLoadResultCode, RawImageData>;

struct RawImageDataFromURIInfo
{
    fastgltf::sources::URI const & uri;
    fastgltf::Asset const & asset;
    // Wihtout the scename.glb part
    std::filesystem::path const scene_dir_path;
};

static auto raw_image_data_from_path(std::filesystem::path image_path) -> RawDataRet
{
    std::ifstream ifs{image_path, std::ios::binary};
    if (!ifs)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_COULD_NOT_OPEN_TEXTURE_FILE;
    }
    ifs.seekg(0, ifs.end);
    i32 const filesize = ifs.tellg();
    ifs.seekg(0, ifs.beg);
    std::vector<std::byte> raw(filesize);
    if (!ifs.read(reinterpret_cast<char *>(raw.data()), filesize))
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_COULD_NOT_READ_TEXTURE_FILE;
    }
    return RawImageData{
        .raw_data = std::move(raw),
        .image_path = image_path,
        .mime_type = {}};
}

static auto raw_image_data_from_URI(RawImageDataFromURIInfo const & info) -> RawDataRet
{
    /// NOTE: Having global paths in your gltf is just wrong. I guess we could later support them by trying to
    //        load the file anyways but cmon what are the chances of that being successful - for now let's just return error
    if (!info.uri.uri.isLocalPath())
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_UNSUPPORTED_ABSOLUTE_PATH;
    }
    /// NOTE: I don't really see how fileoffsets could be valid in a URI context. Since we have no information about the size
    //        of the data we always just load everything in the file. Having just a single offset thus does not allow to pack
    //        multiple images into a single file so we just error on this for now.
    if (info.uri.fileByteOffset != 0)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_URI_FILE_OFFSET_NOT_SUPPORTED;
    }
    std::filesystem::path const full_image_path = info.scene_dir_path / info.uri.uri.fspath();
    APP_LOG(fmt::format("[AssetProcessor::raw_image_data_from_URI] Loading image {} ...", full_image_path.string()));
    RawDataRet raw_image_data_ret = raw_image_data_from_path(full_image_path);
    if (std::holds_alternative<AssetProcessor::AssetLoadResultCode>(raw_image_data_ret))
    {
        return raw_image_data_ret;
    }
    RawImageData & raw_data = std::get<RawImageData>(raw_image_data_ret);
    raw_data.mime_type = info.uri.mimeType;
    return raw_data;
}

struct RawImageDataFromBufferViewInfo
{
    fastgltf::sources::BufferView const & buffer_view;
    fastgltf::Asset const & asset;
    // Wihtout the scename.glb part
    std::filesystem::path const scene_dir_path;
};

static auto raw_image_data_from_buffer_view(RawImageDataFromBufferViewInfo const & info) -> RawDataRet
{
    fastgltf::BufferView const & gltf_buffer_view = info.asset.bufferViews.at(info.buffer_view.bufferViewIndex);
    fastgltf::Buffer const & gltf_buffer = info.asset.buffers.at(gltf_buffer_view.bufferIndex);

    if (!std::holds_alternative<fastgltf::sources::URI>(gltf_buffer.data))
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_FAULTY_BUFFER_VIEW;
    }
    fastgltf::sources::URI uri = std::get<fastgltf::sources::URI>(gltf_buffer.data);

    /// NOTE: load the section of the file containing the buffer for the mesh index buffer.
    std::filesystem::path const full_buffer_path = info.scene_dir_path / uri.uri.fspath();
    std::ifstream ifs{full_buffer_path, std::ios::binary};
    if (!ifs)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_COULD_NOT_OPEN_GLTF;
    }
    /// NOTE: Only load the relevant part of the file containing the view of the buffer we actually need.
    ifs.seekg(gltf_buffer_view.byteOffset + uri.fileByteOffset);
    std::vector<std::byte> raw = {};
    raw.resize(gltf_buffer_view.byteLength);
    /// NOTE: Only load the relevant part of the file containing the view of the buffer we actually need.
    if (!ifs.read(reinterpret_cast<char *>(raw.data()), gltf_buffer_view.byteLength))
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_COULD_NOT_READ_BUFFER_IN_GLTF;
    }
    return RawImageData{
        .raw_data = std::move(raw),
        .image_path = full_buffer_path,
        .mime_type = uri.mimeType};
}
#pragma engregion

#pragma region IMAGE_RAW_DATA_PARSING_HELPERS
struct ParsedImageData
{
    ff::BufferId src_buffer;
    ff::ImageId dst_image;
};

using ParsedImageRet = std::variant<std::monostate, AssetProcessor::AssetLoadResultCode, ParsedImageData>;

enum struct ChannelDataType
{
    SIGNED_INT,
    UNSIGNED_INT,
    FLOATING_POINT
};

struct ChannelInfo
{
    u8 byte_size;
    ChannelDataType data_type;
};
using ParsedChannel = std::variant<std::monostate, AssetProcessor::AssetLoadResultCode, ChannelInfo>;

constexpr static auto parse_channel_info(FREE_IMAGE_TYPE image_type) -> ParsedChannel
{
    ChannelInfo ret = {};
    switch (image_type)
    {
        case FREE_IMAGE_TYPE::FIT_BITMAP:
        {
            ret.byte_size = 1u;
            ret.data_type = ChannelDataType::UNSIGNED_INT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_UINT16:
        {
            ret.byte_size = 2u;
            ret.data_type = ChannelDataType::UNSIGNED_INT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_INT16:
        {
            ret.byte_size = 2u;
            ret.data_type = ChannelDataType::SIGNED_INT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_UINT32:
        {
            ret.byte_size = 4u;
            ret.data_type = ChannelDataType::UNSIGNED_INT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_INT32:
        {
            ret.byte_size = 4u;
            ret.data_type = ChannelDataType::SIGNED_INT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_FLOAT:
        {
            ret.byte_size = 4u;
            ret.data_type = ChannelDataType::FLOATING_POINT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_RGB16:
        {
            ret.byte_size = 2u;
            ret.data_type = ChannelDataType::UNSIGNED_INT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_RGBA16:
        {
            ret.byte_size = 2u;
            ret.data_type = ChannelDataType::UNSIGNED_INT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_RGBF:
        {
            ret.byte_size = 4u;
            ret.data_type = ChannelDataType::FLOATING_POINT;
            break;
        }
        case FREE_IMAGE_TYPE::FIT_RGBAF:
        {
            ret.byte_size = 4u;
            ret.data_type = ChannelDataType::FLOATING_POINT;
            break;
        }
        default:
            return AssetProcessor::AssetLoadResultCode::ERROR_UNSUPPORTED_TEXTURE_PIXEL_FORMAT;
    }
    return ret;
};

struct PixelInfo
{
    u8 channel_count;
    u8 channel_byte_size;
    ChannelDataType channel_data_type;
    bool is_normal;
};

constexpr static auto image_format_from_pixel_info(PixelInfo const & info) -> VkFormat
{
    std::array<std::array<std::array<VkFormat, 3>, 4>, 3> translation = {
        // BYTE SIZE 1
        std::array{
            // CHANNEL COUNT 1
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R8_SRGB, VkFormat::VK_FORMAT_R8_SINT, VkFormat::VK_FORMAT_UNDEFINED}},
            // CHANNEL COUNT 2
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R8G8_SRGB, VkFormat::VK_FORMAT_R8G8_SINT, VkFormat::VK_FORMAT_UNDEFINED}},
            /// NOTE: Free image stores images in BGRA on little endians (Win,Linux) this will break on Mac
            // CHANNEL COUNT 3
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_B8G8R8A8_SRGB, VkFormat::VK_FORMAT_B8G8R8A8_SINT, VkFormat::VK_FORMAT_UNDEFINED}},
            // CHANNEL COUNT 4
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_B8G8R8A8_SRGB, VkFormat::VK_FORMAT_B8G8R8A8_SINT, VkFormat::VK_FORMAT_UNDEFINED}},
        },
        // BYTE SIZE 2
        std::array{
            // CHANNEL COUNT 1
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R16_UINT, VkFormat::VK_FORMAT_R16_SINT, VkFormat::VK_FORMAT_R16_SFLOAT}},
            // CHANNEL COUNT 2
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R16G16_UINT, VkFormat::VK_FORMAT_R16G16_SINT, VkFormat::VK_FORMAT_R16G16_SFLOAT}},
            // CHANNEL COUNT 3
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R16G16B16A16_UINT, VkFormat::VK_FORMAT_R16G16B16A16_SINT, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT}},
            // CHANNEL COUNT 4
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R16G16B16A16_UINT, VkFormat::VK_FORMAT_R16G16B16A16_SINT, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT}},
        },
        // BYTE SIZE 4
        std::array{
            // CHANNEL COUNT 1
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R32_UINT, VkFormat::VK_FORMAT_R32_SINT, VkFormat::VK_FORMAT_R32_SFLOAT}},
            // CHANNEL COUNT 2
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R32G32_UINT, VkFormat::VK_FORMAT_R32G32_SINT, VkFormat::VK_FORMAT_R32G32_SFLOAT}},
            // CHANNEL COUNT 3
            /// TODO: Channel count 3 might not be supported possible just replace with four channel alternatives
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R32G32B32_UINT, VkFormat::VK_FORMAT_R32G32B32_SINT, VkFormat::VK_FORMAT_R32G32B32_SFLOAT}},
            // CHANNEL COUNT 4
            std::array{/* CHANNEL FORMAT */ std::array{VkFormat::VK_FORMAT_R32G32B32A32_UINT, VkFormat::VK_FORMAT_R32G32B32A32_SINT, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT}},
        },
    };
    u8 channel_byte_size_idx{};
    switch (info.channel_byte_size)
    {
        case 1:
            channel_byte_size_idx = 0u;
            break;
        case 2:
            channel_byte_size_idx = 1u;
            break;
        case 4:
            channel_byte_size_idx = 2u;
            break;
        default:
            return VkFormat::VK_FORMAT_UNDEFINED;
    }
    u8 const channel_count_idx = info.channel_count - 1;
    u8 channel_format_idx{};
    switch (info.channel_data_type)
    {
        case ChannelDataType::UNSIGNED_INT:
            channel_format_idx = 0u;
            break;
        case ChannelDataType::SIGNED_INT:
            channel_format_idx = 1u;
            break;
        case ChannelDataType::FLOATING_POINT:
            channel_format_idx = 2u;
            break;
        default:
            return VkFormat::VK_FORMAT_UNDEFINED;
    }
    auto deduced_format = translation[channel_byte_size_idx][channel_count_idx][channel_format_idx];
    if (info.is_normal)
    {
        switch (deduced_format)
        {
            case VkFormat::VK_FORMAT_R8_SRGB:
            {
                deduced_format = VkFormat::VK_FORMAT_R8_UNORM;
                break;
            }
            case VkFormat::VK_FORMAT_R8G8_SRGB:
            {
                deduced_format = VkFormat::VK_FORMAT_R8G8_UNORM;
                break;
            }
            case VkFormat::VK_FORMAT_B8G8R8A8_SRGB:
            {
                deduced_format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
                break;
            }
            default: break;
        }
    }
    return deduced_format;
};

static auto free_image_parse_raw_image_data(RawImageData && raw_data, std::shared_ptr<ff::Device> & device, bool is_normal) -> ParsedImageRet
{
    /// NOTE: Since we handle the image data loading ourselves we need to wrap the buffer with a FreeImage
    //        wrapper so that it can internally process the data
    FIMEMORY * fif_memory_wrapper = FreeImage_OpenMemory(reinterpret_cast<BYTE *>(raw_data.raw_data.data()), raw_data.raw_data.size());
    defer { FreeImage_CloseMemory(fif_memory_wrapper); };
    FREE_IMAGE_FORMAT image_format = FreeImage_GetFileTypeFromMemory(fif_memory_wrapper, 0);
    // could not deduce filetype from metadata in memory try to guess the format from the file extension
    if (image_format == FIF_UNKNOWN)
    {
        image_format = FreeImage_GetFIFFromFilename(raw_data.image_path.string().c_str());
    }
    // could not deduce filetype at all
    if (image_format == FIF_UNKNOWN)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_UNKNOWN_FILETYPE_FORMAT;
    }
    if (!FreeImage_FIFSupportsReading(image_format))
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_UNSUPPORTED_READ_FOR_FILEFORMAT;
    }
    FIBITMAP * image_bitmap = FreeImage_LoadFromMemory(image_format, fif_memory_wrapper);
    defer { FreeImage_Unload(image_bitmap); };
    if (!image_bitmap)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_COULD_NOT_READ_TEXTURE_FILE_FROM_MEMSTREAM;
    }
    FREE_IMAGE_TYPE const image_type = FreeImage_GetImageType(image_bitmap);
    FREE_IMAGE_COLOR_TYPE const color_type = FreeImage_GetColorType(image_bitmap);
    u32 const bits_per_pixel = FreeImage_GetBPP(image_bitmap);
    u32 const width = FreeImage_GetWidth(image_bitmap);
    u32 const height = FreeImage_GetHeight(image_bitmap);
    bool const has_red_channel = FreeImage_GetRedMask(image_bitmap) != 0;
    bool const has_green_channel = FreeImage_GetGreenMask(image_bitmap) != 0;
    bool const has_blue_channel = FreeImage_GetBlueMask(image_bitmap) != 0;

    bool const should_contain_all_color_channels =
        (color_type == FREE_IMAGE_COLOR_TYPE::FIC_RGB) ||
        (color_type == FREE_IMAGE_COLOR_TYPE::FIC_RGBALPHA);
    bool const contains_all_color_channels = has_red_channel && has_green_channel && has_blue_channel;
    DBG_ASSERT_TRUE_M(should_contain_all_color_channels == contains_all_color_channels,
                      std::string("[ERROR][free_image_parse_raw_image_data()] Image color type indicates color channels present") +
                          std::string(" but not all channels were present accoring to color masks"));

    ParsedChannel parsed_channel = parse_channel_info(image_type);
    if (auto const * err = std::get_if<AssetProcessor::AssetLoadResultCode>(&parsed_channel))
    {
        return *err;
    }

    ChannelInfo const & channel_info = std::get<ChannelInfo>(parsed_channel);
    u32 const channel_count = bits_per_pixel / (channel_info.byte_size * 8u);

    VkFormat vulkan_image_format = image_format_from_pixel_info({
        .channel_count = static_cast<u8>(channel_count),
        .channel_byte_size = channel_info.byte_size,
        .channel_data_type = channel_info.data_type,
        .is_normal = is_normal,
    });

    /// TODO: Breaks for 32bit 3 channel images (or overallocates idk)
    u32 const rounded_channel_count = channel_count == 3 ? 4 : channel_count;
    FIBITMAP * modified_bitmap;
    if (channel_count == 3)
    {
        modified_bitmap = FreeImage_ConvertTo32Bits(image_bitmap);
    }
    else
    {
        modified_bitmap = image_bitmap;
    }

    defer
    {
        if (channel_count == 3) FreeImage_Unload(modified_bitmap);
    };
    ParsedImageData ret = {};
    u32 const total_image_byte_size = width * height * rounded_channel_count * channel_info.byte_size;
    FreeImage_FlipVertical(modified_bitmap);
    ret.src_buffer = device->create_buffer({
        .size = total_image_byte_size,
        .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .name = raw_data.image_path.filename().string() + " staging",
    });
    std::byte * staging_dst_ptr = reinterpret_cast<std::byte *>(device->get_buffer_host_pointer(ret.src_buffer));
    memcpy(staging_dst_ptr, reinterpret_cast<std::byte *>(FreeImage_GetBits(modified_bitmap)), total_image_byte_size);
    u32 mip_levels = is_normal ? 1 : static_cast<u32>(std::floor(std::log2(std::max(width, height)))) + 1;
    VkImageUsageFlags usage_flags = VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;
    if (mip_levels > 1)
    {
        usage_flags |= VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    ret.dst_image = device->create_image({
        .dimensions = 2,
        .format = vulkan_image_format,
        .extent = {width, height, 1},
        .mip_level_count = mip_levels,
        .array_layer_count = 1,
        .sample_count = 1,
        /// TODO: Potentially take more flags from the user here
        .usage = usage_flags,
        .alloc_flags = {},
        .aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
        .name = raw_data.image_path.filename().string(),
    });
    return ret;
}

#pragma endregion

AssetProcessor::AssetProcessor(std::shared_ptr<ff::Device> device)
    : _device{device}
{
// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
    FreeImage_Initialise();
#endif
}

AssetProcessor::~AssetProcessor()
{
// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
    FreeImage_DeInitialise();
#endif
}

auto AssetProcessor::load_texture(Scene & scene, u32 texture_manifest_index) -> AssetLoadResultCode
{
    TextureManifestEntry const & texture_entry = scene._material_texture_manifest.at(texture_manifest_index);
    SceneFileManifestEntry const & scene_entry = scene._scene_file_manifest.at(texture_entry.scene_file_manifest_index);
    fastgltf::Asset const & gltf_asset = scene_entry.gltf_asset;
    fastgltf::Image const & image = gltf_asset.images.at(texture_entry.in_scene_file_index);
    std::vector<std::byte> raw_data = {};

    RawDataRet ret = {};
    if (auto const * uri = std::get_if<fastgltf::sources::URI>(&image.data))
    {
        ret = std::move(raw_image_data_from_URI(RawImageDataFromURIInfo{
            .uri = *uri,
            .asset = gltf_asset,
            .scene_dir_path = std::filesystem::path(scene_entry.path).remove_filename()}));
    }
    else if (auto const * buffer_view = std::get_if<fastgltf::sources::BufferView>(&image.data))
    {
        ret = std::move(raw_image_data_from_buffer_view(RawImageDataFromBufferViewInfo{
            .buffer_view = *buffer_view,
            .asset = gltf_asset,
            .scene_dir_path = std::filesystem::path(scene_entry.path).remove_filename()}));
    }
    else
    {
        return AssetLoadResultCode::ERROR_FAULTY_BUFFER_VIEW;
    }

    if (auto const * error = std::get_if<AssetLoadResultCode>(&ret))
    {
        return *error;
    }
    RawImageData & raw_image_data = std::get<RawImageData>(ret);
    ParsedImageRet parsed_data_ret;
    if (raw_image_data.mime_type == fastgltf::MimeType::KTX2)
    {
        // KTX handles image loading
    }
    else
    {
        // FreeImage handles image loading
        bool is_diffuse = false;
        bool is_normal = false;
        for (auto const & indices : texture_entry.material_manifest_indices)
        {
            is_diffuse |= indices.diffuse;
            is_normal |= indices.normal;
        }
        if (!(is_diffuse || is_normal))
        {
            APP_LOG(fmt::format(
                "[INFO][AssetProcessor::load_texture()] Skipping texture {} because"
                "it is not referenced as normal or diffuse by any mesh",
                texture_manifest_index));
            return AssetLoadResultCode::SUCCESS;
        }
        DBG_ASSERT_TRUE_M(!(is_diffuse && is_normal),
                          "[ERROR][AssetProcessor::load_texture()] Texture {} used both as normal map and diffuse map - not supported");
        parsed_data_ret = free_image_parse_raw_image_data(std::move(raw_image_data), _device, is_normal);
    }
    if (auto const * error = std::get_if<AssetProcessor::AssetLoadResultCode>(&parsed_data_ret))
    {
        return *error;
    }
    ParsedImageData const & parsed_data = std::get<ParsedImageData>(parsed_data_ret);
    /// NOTE: Append the processed texture to the upload queue.
    {
        _upload_texture_queue.push_back(TextureUpload{
            .scene = &scene,
            .staging_buffer = parsed_data.src_buffer,
            .dst_image = parsed_data.dst_image,
            .texture_manifest_index = texture_manifest_index});
    }
    return AssetLoadResultCode::SUCCESS;
}

/// NOTE: Overload ElementTraits for glm vec3 for fastgltf to understand the type.
template <>
struct fastgltf::ElementTraits<glm::vec4> : fastgltf::ElementTraitsBase<float, fastgltf::AccessorType::Vec4>
{
};
template <>
struct fastgltf::ElementTraits<glm::vec3> : fastgltf::ElementTraitsBase<float, fastgltf::AccessorType::Vec3>
{
};

template <>
struct fastgltf::ElementTraits<glm::vec2> : fastgltf::ElementTraitsBase<float, fastgltf::AccessorType::Vec2>
{
};

template <typename ElemT, bool IS_INDEX_BUFFER>
auto load_accessor_data_from_file(
    std::filesystem::path const & root_path,
    fastgltf::Asset const & gltf_asset,
    fastgltf::Accessor const & accesor)
    -> std::variant<std::vector<ElemT>, AssetProcessor::AssetLoadResultCode>
{
    static_assert(!IS_INDEX_BUFFER || std::is_same_v<ElemT, u32>, "Index Buffer must be u32");
    fastgltf::BufferView const & gltf_buffer_view = gltf_asset.bufferViews.at(accesor.bufferViewIndex.value());
    fastgltf::Buffer const & gltf_buffer = gltf_asset.buffers.at(gltf_buffer_view.bufferIndex);
    if (!std::holds_alternative<fastgltf::sources::URI>(gltf_buffer.data))
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_FAULTY_BUFFER_VIEW;
    }
    fastgltf::sources::URI uri = std::get<fastgltf::sources::URI>(gltf_buffer.data);

    /// NOTE: load the section of the file containing the buffer for the mesh index buffer.
    std::filesystem::path const full_buffer_path = root_path / uri.uri.fspath();
    std::ifstream ifs{full_buffer_path, std::ios::binary};
    if (!ifs)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_COULD_NOT_OPEN_GLTF;
    }
    /// NOTE: Only load the relevant part of the file containing the view of the buffer we actually need.
    ifs.seekg(gltf_buffer_view.byteOffset + accesor.byteOffset + uri.fileByteOffset);
    std::vector<u16> raw = {};
    raw.resize(gltf_buffer_view.byteLength / 2);
    /// NOTE: Only load the relevant part of the file containing the view of the buffer we actually need.
    auto const elem_byte_size = fastgltf::getElementByteSize(accesor.type, accesor.componentType);
    if (!ifs.read(reinterpret_cast<char *>(raw.data()), accesor.count * elem_byte_size))
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_COULD_NOT_READ_BUFFER_IN_GLTF;
    }
    auto buffer_adapter = [&](fastgltf::Buffer const & buffer)
    {
        /// NOTE:   We only have a ptr to the loaded data to the accessors section of the buffer.
        ///         Fastgltf expects a ptr to the begin of the buffer, so we just subtract the offsets.
        ///         Fastgltf adds these on in the accessor tool, so in the end it gets the right ptr.
        auto const fastgltf_reverse_byte_offset = (gltf_buffer_view.byteOffset + accesor.byteOffset);
        return reinterpret_cast<std::byte *>(raw.data()) - fastgltf_reverse_byte_offset;
    };

    std::vector<ElemT> ret(accesor.count);
    if constexpr (IS_INDEX_BUFFER)
    {
        /// NOTE: Transform the loaded file section into a 32 bit index buffer.
        if (accesor.componentType == fastgltf::ComponentType::UnsignedShort)
        {
            std::vector<u16> u16_index_buffer(accesor.count);
            fastgltf::copyFromAccessor<u16>(gltf_asset, accesor, u16_index_buffer.data(), buffer_adapter);
            for (size_t i = 0; i < u16_index_buffer.size(); ++i)
            {
                ret[i] = static_cast<u32>(u16_index_buffer[i]);
            }
        }
        else
        {
            fastgltf::copyFromAccessor<u32>(gltf_asset, accesor, ret.data(), buffer_adapter);
        }
    }
    else
    {
        fastgltf::copyFromAccessor<ElemT>(gltf_asset, accesor, ret.data(), buffer_adapter);
    }
    return {std::move(ret)};
}

auto AssetProcessor::load_mesh(Scene & scene, u32 mesh_manifest_index) -> AssetProcessor::AssetLoadResultCode
{
    MeshManifestEntry & mesh_data = scene._mesh_manifest.at(mesh_manifest_index);
    SceneFileManifestEntry & gltf_scene = scene._scene_file_manifest.at(mesh_data.scene_file_manifest_index);
    fastgltf::Asset & gltf_asset = gltf_scene.gltf_asset;

    fastgltf::Mesh & gltf_mesh = gltf_asset.meshes[mesh_data.scene_file_mesh_index];
    fastgltf::Primitive & gltf_prim = gltf_mesh.primitives[mesh_data.scene_file_primitive_index];

/// NOTE: Process indices (they are required)
#pragma region INDICES
    if (!gltf_prim.indicesAccessor.has_value())
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_MISSING_INDEX_BUFFER;
    }
    fastgltf::Accessor & index_buffer_gltf_accessor = gltf_asset.accessors.at(gltf_prim.indicesAccessor.value());
    bool const index_buffer_accessor_valid =
        (index_buffer_gltf_accessor.componentType == fastgltf::ComponentType::UnsignedInt ||
         index_buffer_gltf_accessor.componentType == fastgltf::ComponentType::UnsignedShort) &&
        index_buffer_gltf_accessor.type == fastgltf::AccessorType::Scalar &&
        index_buffer_gltf_accessor.bufferViewIndex.has_value();
    if (!index_buffer_accessor_valid)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_FAULTY_INDEX_BUFFER_GLTF_ACCESSOR;
    }
    auto index_buffer_data = load_accessor_data_from_file<u32, true>(std::filesystem::path{gltf_scene.path}.remove_filename(), gltf_asset, index_buffer_gltf_accessor);
    if (auto const * err = std::get_if<AssetProcessor::AssetLoadResultCode>(&index_buffer_data))
    {
        return *err;
    }
    std::vector<u32> index_buffer = std::get<std::vector<u32>>(std::move(index_buffer_data));
#pragma endregion

/// NOTE: Load vertex positions
#pragma region VERTICES
    auto vert_attrib_iter = gltf_prim.findAttribute(VERT_ATTRIB_POSITION_NAME);
    if (vert_attrib_iter == gltf_prim.attributes.end())
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_MISSING_VERTEX_POSITIONS;
    }
    fastgltf::Accessor & gltf_vertex_pos_accessor = gltf_asset.accessors.at(vert_attrib_iter->second);
    bool const gltf_vertex_pos_accessor_valid =
        gltf_vertex_pos_accessor.componentType == fastgltf::ComponentType::Float &&
        gltf_vertex_pos_accessor.type == fastgltf::AccessorType::Vec3;
    if (!gltf_vertex_pos_accessor_valid)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_FAULTY_GLTF_VERTEX_POSITIONS;
    }
    // TODO: we can probably load this directly into the staging buffer.
    auto vertex_pos_result = load_accessor_data_from_file<glm::vec3, false>(std::filesystem::path{gltf_scene.path}.remove_filename(), gltf_asset, gltf_vertex_pos_accessor);
    if (auto const * err = std::get_if<AssetProcessor::AssetLoadResultCode>(&vertex_pos_result))
    {
        return *err;
    }
    std::vector<glm::vec3> vert_positions = std::get<std::vector<glm::vec3>>(std::move(vertex_pos_result));
    u32 const vertex_count = static_cast<u32>(vert_positions.size());
#pragma endregion

/// NOTE: Load vertex UVs
#pragma region UVS
    auto texcoord0_attrib_iter = gltf_prim.findAttribute(VERT_ATTRIB_TEXCOORD0_NAME);
    if (texcoord0_attrib_iter == gltf_prim.attributes.end())
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_MISSING_VERTEX_TEXCOORD_0;
    }
    fastgltf::Accessor & gltf_vertex_texcoord0_accessor = gltf_asset.accessors.at(texcoord0_attrib_iter->second);
    bool const gltf_vertex_texcoord0_accessor_valid =
        gltf_vertex_texcoord0_accessor.componentType == fastgltf::ComponentType::Float &&
        gltf_vertex_texcoord0_accessor.type == fastgltf::AccessorType::Vec2;
    if (!gltf_vertex_texcoord0_accessor_valid)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_FAULTY_GLTF_VERTEX_TEXCOORD_0;
    }
    auto vertex_texcoord0_pos_result = load_accessor_data_from_file<glm::vec2, false>(std::filesystem::path{gltf_scene.path}.remove_filename(), gltf_asset, gltf_vertex_texcoord0_accessor);
    if (auto const * err = std::get_if<AssetProcessor::AssetLoadResultCode>(&vertex_texcoord0_pos_result))
    {
        return *err;
    }
    std::vector<glm::vec2> vert_texcoord0 = std::get<std::vector<glm::vec2>>(std::move(vertex_texcoord0_pos_result));
    DBG_ASSERT_TRUE_M(vert_texcoord0.size() == vert_positions.size(), "[AssetProcessor::load_mesh()] Mismatched position and uv count");
#pragma endregion
#pragma region Tangents
    auto tangent_attrib_iter = gltf_prim.findAttribute(VERT_ATTRIB_TANGENT_NAME);
    if (tangent_attrib_iter == gltf_prim.attributes.end())
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_MISSING_VERTEX_TANGENT;
    }
    fastgltf::Accessor & gltf_vertex_tangent_accessor = gltf_asset.accessors.at(tangent_attrib_iter->second);
    bool const gltf_vertex_tangent_accessor_valid =
        gltf_vertex_tangent_accessor.componentType == fastgltf::ComponentType::Float &&
        gltf_vertex_tangent_accessor.type == fastgltf::AccessorType::Vec4;
    if (!gltf_vertex_tangent_accessor_valid)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_FAULTY_GLTF_VERTEX_TANGENT;
    }
    auto vertex_tangent_pos_result = load_accessor_data_from_file<glm::vec4, false>(std::filesystem::path{gltf_scene.path}.remove_filename(), gltf_asset, gltf_vertex_tangent_accessor);
    if (auto const * err = std::get_if<AssetProcessor::AssetLoadResultCode>(&vertex_tangent_pos_result))
    {
        return *err;
    }
    std::vector<glm::vec4> vert_tangent = std::get<std::vector<glm::vec4>>(std::move(vertex_tangent_pos_result));
    DBG_ASSERT_TRUE_M(vert_tangent.size() == vert_positions.size(), "[AssetProcessor::load_mesh()] Mismatched position and uv count");
#pragma endregion

#pragma region Normals
    auto normal_attrib_iter = gltf_prim.findAttribute(VERT_ATTRIB_NORMAL_NAME);
    if (normal_attrib_iter == gltf_prim.attributes.end())
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_MISSING_VERTEX_NORMAL;
    }
    fastgltf::Accessor & gltf_vertex_normal_accessor = gltf_asset.accessors.at(normal_attrib_iter->second);
    bool const gltf_vertex_normal_accessor_valid =
        gltf_vertex_normal_accessor.componentType == fastgltf::ComponentType::Float &&
        gltf_vertex_normal_accessor.type == fastgltf::AccessorType::Vec3;
    if (!gltf_vertex_normal_accessor_valid)
    {
        return AssetProcessor::AssetLoadResultCode::ERROR_FAULTY_GLTF_VERTEX_NORMAL;
    }
    auto vertex_normal_pos_result = load_accessor_data_from_file<glm::vec3, false>(std::filesystem::path{gltf_scene.path}.remove_filename(), gltf_asset, gltf_vertex_normal_accessor);
    if (auto const * err = std::get_if<AssetProcessor::AssetLoadResultCode>(&vertex_normal_pos_result))
    {
        return *err;
    }
    std::vector<glm::vec3> vert_normals = std::get<std::vector<glm::vec3>>(std::move(vertex_normal_pos_result));
    DBG_ASSERT_TRUE_M(vert_normals.size() == vert_positions.size(), "[AssetProcessor::load_mesh()] Mismatched normal and uv count");
#pragma endregion

    u32 const positions_offset = positions.size();
    u32 const uvs_offset = uvs.size();
    u32 const indices_offset = indices.size();
    u32 const tangents_offset = tangents.size();
    u32 const normals_offset = normals.size();

    positions.reserve(positions.size() + vert_positions.size());
    uvs.reserve(uvs.size() + vert_texcoord0.size());
    tangents.reserve(tangents.size() + vert_tangent.size());
    normals.reserve(normals.size() + vert_normals.size());
    indices.reserve(indices.size() + index_buffer.size());

    positions.insert(positions.end(), vert_positions.begin(), vert_positions.end());
    uvs.insert(uvs.end(), vert_texcoord0.begin(), vert_texcoord0.end());
    tangents.insert(tangents.end(), vert_tangent.begin(), vert_tangent.end());
    normals.insert(normals.end(), vert_normals.begin(), vert_normals.end());
    indices.insert(indices.end(), index_buffer.begin(), index_buffer.end());

    mesh_data.cpu_runtime = MeshDescriptorCpu{
        .vertex_count = static_cast<u32>(vert_positions.size()),
        .positions_offset = positions_offset,
        .uvs_offset = uvs_offset,
        .tangents_offset = tangents_offset,
        .normals_offset = normals_offset,
        .index_count = static_cast<u32>(index_buffer.size()),
        .indices_offset = indices_offset,
    };
    return AssetProcessor::AssetLoadResultCode::SUCCESS;
}

auto AssetProcessor::load_mesh_group(Scene & scene, u32 mesh_group_manifest_index) -> AssetProcessor::AssetLoadResultCode
{
    MeshGroupManifestEntry & mesh_group_data = scene._mesh_group_manifest.at(mesh_group_manifest_index);
    SceneFileManifestEntry & gltf_scene = scene._scene_file_manifest.at(mesh_group_data.scene_file_manifest_index);
    fastgltf::Asset & gltf_asset = gltf_scene.gltf_asset;

    AssetProcessor::AssetLoadResultCode result = AssetLoadResultCode::SUCCESS;
    for (u32 mesh_in_meshgroup_index = 0; mesh_in_meshgroup_index < mesh_group_data.mesh_count; mesh_in_meshgroup_index++)
    {
        u32 const mesh_manifest_index = mesh_group_data.mesh_manifest_indices.at(mesh_in_meshgroup_index);
        result = load_mesh(scene, mesh_manifest_index);
        if (result != AssetLoadResultCode::SUCCESS)
        {
            return result;
        }
    }
    return result;
}

void process_node(Scene & scene, RenderEntity const * node, f32mat4x3 curr_transform)
{
    auto const has_children = node->first_child.has_value();
    auto const has_meshgroup = node->mesh_group_manifest_index.has_value();
    DBG_ASSERT_TRUE_M(!(has_children && has_meshgroup), "[ERROR][AssetProcessor::process_node()] Node is both meshgroup and has children");

    auto mat4x3_to_mat4x4 = [](f32mat4x3 orig) -> f32mat4x4
    {
        return f32mat4x4(
            f32vec4(orig[0][0], orig[0][1], orig[0][2], 0.0),
            f32vec4(orig[1][0], orig[1][1], orig[1][2], 0.0),
            f32vec4(orig[2][0], orig[2][1], orig[2][2], 0.0),
            f32vec4(orig[3][0], orig[3][1], orig[3][2], 1.0));
    };

    auto new_transform = mat4x3_to_mat4x4(curr_transform) * mat4x3_to_mat4x4(node->transform);
    if (has_children)
    {
        auto * curr_child = scene._render_entities.slot(node->first_child.value());
        while (true)
        {
            process_node(scene, curr_child, f32mat4x3(new_transform));
            if (!curr_child->next_sibling.has_value())
            {
                return;
            }
            curr_child = scene._render_entities.slot(curr_child->next_sibling.value());
        }
    }
    else if (has_meshgroup)
    {
        auto & meshgroup_data = scene._mesh_group_manifest.at(node->mesh_group_manifest_index.value());
        meshgroup_data.instance_transforms.push_back(f32mat4x3(new_transform));
    }
}

void AssetProcessor::record_gpu_load_processing_commands(Scene & scene)
{
#pragma region RECORD_MESH_UPLOAD_COMMANDS
    {
        auto indices_command_buffer = ff::CommandBuffer(_device);
        indices_command_buffer.begin();
        scene._gpu_mesh_indices = _device->create_buffer({
            .size = indices.size() * sizeof(u32),
            .flags = {},
            .name = "gpu_mesh_indices",
        });

        auto indices_staging = _device->create_buffer({
            .size = indices.size() * sizeof(u32),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "gpu_mesh_indices staging",
        });

        void * staging_ptr = _device->get_buffer_host_pointer(indices_staging);
        std::memcpy(staging_ptr, indices.data(), sizeof(u32) * indices.size());
        indices_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = indices_staging,
            .src_offset = 0,
            .dst_buffer = scene._gpu_mesh_indices,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(u32) * indices.size()),
        });
        indices_command_buffer.end();
        auto recorded_command_buffer = indices_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(indices_staging);
        indices.clear();
        _device->wait_idle();
        _device->cleanup_resources();
    }

    {
        auto positions_command_buffer = ff::CommandBuffer(_device);
        positions_command_buffer.begin();
        scene._gpu_mesh_positions = _device->create_buffer({
            .size = positions.size() * sizeof(f32vec3),
            .flags = {},
            .name = "gpu_mesh_positions",
        });

        auto positions_staging = _device->create_buffer({
            .size = positions.size() * sizeof(f32vec3),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "gpu_mesh_positions staging",
        });

        void * staging_ptr = _device->get_buffer_host_pointer(positions_staging);
        std::memcpy(staging_ptr, positions.data(), sizeof(f32vec3) * positions.size());
        positions_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = positions_staging,
            .src_offset = 0,
            .dst_buffer = scene._gpu_mesh_positions,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(f32vec3) * positions.size()),
        });
        positions_command_buffer.end();
        auto recorded_command_buffer = positions_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(positions_staging);
        positions.clear();
        _device->wait_idle();
        _device->cleanup_resources();
    }

    {
        auto uvs_command_buffer = ff::CommandBuffer(_device);
        uvs_command_buffer.begin();
        scene._gpu_mesh_uvs = _device->create_buffer({
            .size = uvs.size() * sizeof(f32vec2),
            .flags = {},
            .name = "gpu_mesh_uvs",
        });

        auto uvs_staging = _device->create_buffer({
            .size = uvs.size() * sizeof(f32vec2),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "gpu_mesh_uvs staging",
        });

        void * staging_ptr = _device->get_buffer_host_pointer(uvs_staging);
        std::memcpy(staging_ptr, uvs.data(), sizeof(f32vec2) * uvs.size());
        uvs_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = uvs_staging,
            .src_offset = 0,
            .dst_buffer = scene._gpu_mesh_uvs,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(f32vec2) * uvs.size()),
        });
        uvs_command_buffer.end();
        auto recorded_command_buffer = uvs_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(uvs_staging);
        uvs.clear();
        _device->wait_idle();
        _device->cleanup_resources();
    }
    {
        auto tangents_command_buffer = ff::CommandBuffer(_device);
        tangents_command_buffer.begin();
        scene._gpu_mesh_tangents = _device->create_buffer({
            .size = tangents.size() * sizeof(f32vec4),
            .flags = {},
            .name = "gpu_mesh_tangents",
        });

        auto tangents_staging = _device->create_buffer({
            .size = tangents.size() * sizeof(f32vec4),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "gpu_mesh_tangents staging",
        });

        void * staging_ptr = _device->get_buffer_host_pointer(tangents_staging);
        std::memcpy(staging_ptr, tangents.data(), sizeof(f32vec4) * tangents.size());
        tangents_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = tangents_staging,
            .src_offset = 0,
            .dst_buffer = scene._gpu_mesh_tangents,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(f32vec4) * tangents.size()),
        });
        tangents_command_buffer.end();
        auto recorded_command_buffer = tangents_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(tangents_staging);
        tangents.clear();
        _device->wait_idle();
        _device->cleanup_resources();
    }
    {
        auto normals_command_buffer = ff::CommandBuffer(_device);
        normals_command_buffer.begin();
        scene._gpu_mesh_normals = _device->create_buffer({
            .size = normals.size() * sizeof(f32vec3),
            .flags = {},
            .name = "gpu_mesh_normals",
        });

        auto normals_staging = _device->create_buffer({
            .size = normals.size() * sizeof(f32vec3),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "gpu_mesh_normals staging",
        });

        void * staging_ptr = _device->get_buffer_host_pointer(normals_staging);
        std::memcpy(staging_ptr, normals.data(), sizeof(f32vec3) * normals.size());
        normals_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = normals_staging,
            .src_offset = 0,
            .dst_buffer = scene._gpu_mesh_normals,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(f32vec3) * normals.size()),
        });
        normals_command_buffer.end();
        auto recorded_command_buffer = normals_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(normals_staging);
        normals.clear();
        _device->wait_idle();
        _device->cleanup_resources();
    }

    auto const * root_node = scene._render_entities.slot(scene._scene_file_manifest.at(0).root_render_entity);
    process_node(scene, root_node, f32mat4x3(glm::identity<glm::mat4x4>()));
    std::vector<f32mat4x3> transforms = {};
    std::vector<MeshDescriptor> mesh_descriptors = {};
    for (auto const & meshgroup : scene._mesh_group_manifest)
    {
        for (i32 mesh_idx = 0; mesh_idx < meshgroup.mesh_count; mesh_idx++)
        {
            auto & mesh = scene._mesh_manifest.at(meshgroup.mesh_manifest_indices.at(mesh_idx));
            mesh.cpu_runtime->transforms_offset = static_cast<u32>(transforms.size());
            mesh_descriptors.push_back({
                .transforms_offset = mesh.cpu_runtime->transforms_offset,
                .positions_offset = mesh.cpu_runtime->positions_offset,
                .uvs_offset = mesh.cpu_runtime->uvs_offset,
                .tangents_offset = mesh.cpu_runtime->tangents_offset,
                .normals_offset = mesh.cpu_runtime->normals_offset,
                .indices_offset = mesh.cpu_runtime->indices_offset,
                .material_index = mesh.material_manifest_index.value_or(0),
            });
        }
        transforms.insert(transforms.end(), meshgroup.instance_transforms.begin(), meshgroup.instance_transforms.end());
    }
    {
        auto transforms_command_buffer = ff::CommandBuffer(_device);
        transforms_command_buffer.begin();
        scene._gpu_mesh_transforms = _device->create_buffer({
            .size = transforms.size() * sizeof(f32mat4x3),
            .flags = {},
            .name = "gpu_mesh_transforms",
        });

        auto transforms_staging = _device->create_buffer({
            .size = transforms.size() * sizeof(f32mat4x3),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "gpu_mesh_transforms staging",
        });

        void * staging_ptr = _device->get_buffer_host_pointer(transforms_staging);
        std::memcpy(staging_ptr, transforms.data(), sizeof(f32mat4x3) * transforms.size());
        transforms_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = transforms_staging,
            .src_offset = 0,
            .dst_buffer = scene._gpu_mesh_transforms,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(f32mat4x3) * transforms.size()),
        });
        transforms_command_buffer.end();
        auto recorded_command_buffer = transforms_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(transforms_staging);
        _device->wait_idle();
        _device->cleanup_resources();
    }
    {
        auto mesh_descriptors_command_buffer = ff::CommandBuffer(_device);
        mesh_descriptors_command_buffer.begin();
        scene._gpu_mesh_descriptors = _device->create_buffer({
            .size = mesh_descriptors.size() * sizeof(MeshDescriptor),
            .flags = {},
            .name = "gpu_mesh_descriptors",
        });

        auto mesh_descriptors_staging = _device->create_buffer({
            .size = mesh_descriptors.size() * sizeof(MeshDescriptor),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "gpu_mesh_descriptors staging",
        });

        void * staging_ptr = _device->get_buffer_host_pointer(mesh_descriptors_staging);
        std::memcpy(staging_ptr, mesh_descriptors.data(), sizeof(MeshDescriptor) * mesh_descriptors.size());
        mesh_descriptors_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = mesh_descriptors_staging,
            .src_offset = 0,
            .dst_buffer = scene._gpu_mesh_descriptors,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(MeshDescriptor) * mesh_descriptors.size()),
        });
        mesh_descriptors_command_buffer.end();
        auto recorded_command_buffer = mesh_descriptors_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(mesh_descriptors_staging);
        _device->wait_idle();
        _device->cleanup_resources();
    }

#pragma endregion

#pragma region RECORD_TEXTURE_UPLOAD_COMMANDS
    auto upload_textures_command_buffer = ff::CommandBuffer(_device);
    upload_textures_command_buffer.begin();
    // Transition texture from UNDEFINED -> VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    for (TextureUpload const & texture_upload : _upload_texture_queue)
    {
        texture_upload.scene->_material_texture_manifest.at(texture_upload.texture_manifest_index).runtime = texture_upload.dst_image;
        upload_textures_command_buffer.cmd_image_memory_transition_barrier({
            .src_stages = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .src_access = VK_ACCESS_2_NONE_KHR,
            .dst_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
            .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .level_count = _device->info_image(texture_upload.dst_image).mip_level_count,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .image_id = texture_upload.dst_image,
        });
    }
    // Upload texture data into the texture (mip 0)
    for (TextureUpload const & texture_upload : _upload_texture_queue)
    {
        auto const image_extent = _device->info_image(texture_upload.dst_image).extent;
        upload_textures_command_buffer.cmd_copy_buffer_to_image({
            .buffer_id = texture_upload.staging_buffer,
            .image_id = texture_upload.dst_image,
            .image_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .image_offset = {0, 0, 0},
            .image_extent = _device->info_image(texture_upload.dst_image).extent,
        });
    }
    // Generate mips
    for (TextureUpload const & texture_upload : _upload_texture_queue)
    {
        auto const mip_count = _device->info_image(texture_upload.dst_image).mip_level_count;
        if (mip_count == 1)
        {
            continue;
        }
        auto const extent = _device->info_image(texture_upload.dst_image).extent;
        auto mip_src_end_offset = VkOffset3D{static_cast<i32>(extent.width), static_cast<i32>(extent.height), 1};
        for (i32 mip_level = 1; mip_level < mip_count; mip_level++)
        {
            // mip_level - 1 TRANSFER_DST_OPTIMAL -> TRANSFER_SRC_OPTIMAL
            upload_textures_command_buffer.cmd_image_memory_transition_barrier({
                .src_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .src_access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dst_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .dst_access = VK_ACCESS_2_TRANSFER_READ_BIT,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .base_mip_level = static_cast<u32>(mip_level - 1),
                .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .image_id = texture_upload.dst_image,
            });

            upload_textures_command_buffer.cmd_blit_image({
                .src_image = texture_upload.dst_image,
                .dst_image = texture_upload.dst_image,
                .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .src_aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .dst_aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                .src_mip_level = static_cast<u32>(mip_level - 1),
                .dst_mip_level = static_cast<u32>(mip_level),
                .src_start_offset = {0, 0, 0},
                .src_end_offset = mip_src_end_offset,
                .dst_start_offset = {0, 0, 0},
                .dst_end_offset = {
                    mip_src_end_offset.x > 1 ? mip_src_end_offset.x / 2 : 1,
                    mip_src_end_offset.y > 1 ? mip_src_end_offset.y / 2 : 1,
                    1,
                },
            });
            mip_src_end_offset.x = mip_src_end_offset.x > 1 ? mip_src_end_offset.x / 2 : 1;
            mip_src_end_offset.y = mip_src_end_offset.y > 1 ? mip_src_end_offset.y / 2 : 1;
        }
        /// NOTE: We need to transfer the last mip separately because it is in TRANSFER DST layout as opposed
        /// to all other mips which are in TRANSFER_SRC_OPTIMAL
        // Last mip TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        upload_textures_command_buffer.cmd_image_memory_transition_barrier({
            .src_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .src_access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .dst_stages = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dst_access = VK_ACCESS_2_NONE,
            .src_layout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .base_mip_level = mip_count - 1,
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .image_id = texture_upload.dst_image,
        });
    }

    for (TextureUpload const & texture_upload : _upload_texture_queue)
    {
        auto const mipmap_count = _device->info_image(texture_upload.dst_image).mip_level_count;
        auto src_access = mipmap_count == 1 ? VK_ACCESS_2_TRANSFER_WRITE_BIT : VK_ACCESS_2_TRANSFER_READ_BIT;
        auto src_layout = mipmap_count == 1 ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        upload_textures_command_buffer.cmd_image_memory_transition_barrier({
            .src_stages = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .src_access = src_access,
            .dst_stages = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .dst_access = VK_ACCESS_2_NONE,
            .src_layout = src_layout,
            .dst_layout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .level_count = std::max(mipmap_count - 1, 1u),
            .aspect_mask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
            .image_id = texture_upload.dst_image,
        });
    }
    upload_textures_command_buffer.end();
    auto recorded_command_buffer = upload_textures_command_buffer.get_recorded_command_buffer();
    _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
    for (TextureUpload const & texture_upload : _upload_texture_queue)
    {
        _device->destroy_buffer(texture_upload.staging_buffer);
    }
    _device->wait_idle();
    _device->cleanup_resources();
#pragma endregion
#pragma region RECORD_MATERIAL_UPLOAD_COMMANDS
    /// NOTE: We need to propagate each loaded texture image ID into the material manifest This will be done in two steps:
    //        1) We update the CPU manifest with the correct values and remember the materials that were updated
    //        2) For each dirty material we generate a copy buffer to buffer comand to update the GPU manifest
    std::vector<u32> dirty_material_entry_indices = {};
    // 1) Update CPU Manifest
    for (TextureUpload const & texture_upload : _upload_texture_queue)
    {
        auto const & texture_manifest = texture_upload.scene->_material_texture_manifest;
        auto & material_manifest = texture_upload.scene->_material_manifest;
        TextureManifestEntry const & texture_manifest_entry = texture_manifest.at(texture_upload.texture_manifest_index);
        for (auto const material_using_texture_info : texture_manifest_entry.material_manifest_indices)
        {
            MaterialManifestEntry & material_entry = material_manifest.at(material_using_texture_info.material_manifest_index);
            if (material_using_texture_info.diffuse)
            {
                material_entry.diffuse_tex_index = texture_upload.texture_manifest_index;
            }
            if (material_using_texture_info.normal)
            {
                material_entry.normal_tex_index = texture_upload.texture_manifest_index;
            }
            /// NOTE: Add material index only if it was not added previously
            if (std::find(
                    dirty_material_entry_indices.begin(),
                    dirty_material_entry_indices.end(),
                    material_using_texture_info.material_manifest_index) ==
                dirty_material_entry_indices.end())
            {
                dirty_material_entry_indices.push_back(material_using_texture_info.material_manifest_index);
            }
        }
    }
    // 2) Update GPU manifest
    scene._gpu_material_descriptors = _device->create_buffer({
        .size = scene._material_manifest.size() * sizeof(MaterialDescriptor),
        .flags = {},
        .name = "gpu_materials_descriptor",
    });
    // FIXME(msakmary) GIANT HACK
    if (dirty_material_entry_indices.size() == 0)
    {
        for (i32 i = 0; i < scene._material_manifest.size(); i++)
        {
            dirty_material_entry_indices.push_back(i);
        }
    }
    auto upload_manifest_command_buffer = ff::CommandBuffer(_device);
    upload_manifest_command_buffer.begin();
    auto const materials_update_staging_buffer = _device->create_buffer({
        .size = sizeof(MaterialDescriptor) * dirty_material_entry_indices.size(),
        .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .name = "gpu materials update",
    });
    MaterialDescriptor * const staging_origin_ptr = reinterpret_cast<MaterialDescriptor *>(_device->get_buffer_host_pointer(materials_update_staging_buffer));
    for (u32 dirty_materials_index = 0; dirty_materials_index < dirty_material_entry_indices.size(); dirty_materials_index++)
    {
        auto const & texture_manifest = scene._material_texture_manifest;
        auto const & material_manifest = scene._material_manifest;
        MaterialManifestEntry const & material = material_manifest.at(dirty_material_entry_indices.at(dirty_materials_index));

        if (material.diffuse_tex_index.has_value())
        {
            staging_origin_ptr[dirty_materials_index].albedo_index = texture_manifest.at(material.diffuse_tex_index.value()).runtime.value().index;
        }
        else
        {
            staging_origin_ptr[dirty_materials_index].albedo_index = -1;
        }
        if (material.normal_tex_index.has_value())
        {
            staging_origin_ptr[dirty_materials_index].normal_index = texture_manifest.at(material.normal_tex_index.value()).runtime.value().index;
        }
        else
        {
            staging_origin_ptr[dirty_materials_index].normal_index = -1;
        }

        upload_manifest_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = materials_update_staging_buffer,
            .src_offset = static_cast<u32>(sizeof(MaterialDescriptor) * dirty_materials_index),
            .dst_buffer = scene._gpu_material_descriptors,
            .dst_offset = static_cast<u32>(sizeof(MaterialDescriptor) * dirty_material_entry_indices.at(dirty_materials_index)),
            .size = sizeof(MaterialDescriptor),
        });
    }
    {
        upload_manifest_command_buffer.end();
        auto recorded_command_buffer = upload_manifest_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(materials_update_staging_buffer);
        _device->wait_idle();
        _device->cleanup_resources();
    }
    _upload_texture_queue.clear();
#pragma endregion
#pragma region RECORD_SCENE_DESCRIPTOR_UPLOAD_COMMANDS
    {
        auto scene_descriptor_command_buffer = ff::CommandBuffer(_device);
        scene_descriptor_command_buffer.begin();
        scene._gpu_scene_descriptor = _device->create_buffer({
            .size = sizeof(SceneDescriptor),
            .flags = {},
            .name = "gpu_scene_descriptor",
        });
        auto scene_descriptor_staging = _device->create_buffer({
            .size = sizeof(SceneDescriptor),
            .flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .name = "gpu_scene_descriptor staging",
        });
        SceneDescriptor * staging_ptr = reinterpret_cast<SceneDescriptor *>(_device->get_buffer_host_pointer(scene_descriptor_staging));
        *staging_ptr = {
            .mesh_descriptors_start = _device->get_buffer_device_address(scene._gpu_mesh_descriptors),
            .material_descriptors_start = _device->get_buffer_device_address(scene._gpu_material_descriptors),
            .transforms_start = _device->get_buffer_device_address(scene._gpu_mesh_transforms),
            .positions_start = _device->get_buffer_device_address(scene._gpu_mesh_positions),
            .uvs_start = _device->get_buffer_device_address(scene._gpu_mesh_uvs),
            .normals_start = _device->get_buffer_device_address(scene._gpu_mesh_normals),
            .tangents_start = _device->get_buffer_device_address(scene._gpu_mesh_tangents),
            .indices_start = _device->get_buffer_device_address(scene._gpu_mesh_indices),
        };
        scene_descriptor_command_buffer.cmd_copy_buffer_to_buffer({
            .src_buffer = scene_descriptor_staging,
            .src_offset = 0,
            .dst_buffer = scene._gpu_scene_descriptor,
            .dst_offset = 0,
            .size = static_cast<u32>(sizeof(SceneDescriptor)),
        });
        scene_descriptor_command_buffer.end();
        auto recorded_command_buffer = scene_descriptor_command_buffer.get_recorded_command_buffer();
        _device->submit({.command_buffers = {&recorded_command_buffer, 1}});
        _device->destroy_buffer(scene_descriptor_staging);
        _device->wait_idle();
        _device->cleanup_resources();
    }
#pragma endregion
}

auto AssetProcessor::load_all(Scene & scene) -> AssetProcessor::AssetLoadResultCode
{
    std::optional<AssetProcessor::AssetLoadResultCode> err = {};
    for (u32 i = 0; i < scene._material_texture_manifest.size(); ++i)
    {
        APP_LOG(fmt::format("[INFO][AssetProcessor::load_all] Loading texture {}", scene._material_texture_manifest.at(i).name));
        auto result = load_texture(scene, i);
        if (result != AssetProcessor::AssetLoadResultCode::SUCCESS)
        {
            APP_LOG("[ERROR][Scene::Scene()] Error loading texture");
            throw std::runtime_error("[ERROR][Scene::Scene()] Error loading texture");
        }
    }
    for (u32 i = 0; i < scene._mesh_group_manifest.size(); ++i)
    {
        APP_LOG(fmt::format("[INFO][AssetProcessor::load_all] Loading meshgroup {} of {}", i, scene._mesh_group_manifest.size()));
        auto result = load_mesh_group(scene, i);
        if (result != AssetProcessor::AssetLoadResultCode::SUCCESS)
        {
            APP_LOG("[ERROR][Scene::Scene()] Error loading mesh group");
            throw std::runtime_error("[ERROR][Scene::Scene()] Error loading mesh group");
        }
    }
    return AssetProcessor::AssetLoadResultCode::SUCCESS;
}