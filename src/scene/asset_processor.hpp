#pragma once

#include <filesystem>

#include "../fairy_forest.hpp"
#include "../context.hpp"
#include "../backend/backend.hpp"
#include "scene.hpp"

using namespace ff::types;

using MeshIndex = size_t;
using ImageIndex = size_t;

#define MAX_MESHES 10000

// inline std::string generate_mesh_name(aiMesh* mesh)
// {
//     return
//         std::string(mesh->mName.C_Str()) + std::string(" m:") + std::to_string(mesh->mMaterialIndex);
// }
//
// inline std::string generate_texture_name(aiMaterial * material, aiTextureType type)
// {
//     return std::string(material->GetName().C_Str()) + aiTextureTypeToString(type);
// }

struct AssetProcessor
{
    enum struct AssetLoadResultCode
    {
        SUCCESS,
        ERROR_MISSING_INDEX_BUFFER,
        ERROR_FAULTY_INDEX_BUFFER_GLTF_ACCESSOR,
        ERROR_FAULTY_BUFFER_VIEW,
        ERROR_COULD_NOT_OPEN_GLTF,
        ERROR_COULD_NOT_READ_BUFFER_IN_GLTF,
        ERROR_COULD_NOT_OPEN_TEXTURE_FILE,
        ERROR_COULD_NOT_READ_TEXTURE_FILE,
        ERROR_COULD_NOT_READ_TEXTURE_FILE_FROM_MEMSTREAM,
        ERROR_UNSUPPORTED_TEXTURE_PIXEL_FORMAT,
        ERROR_UNKNOWN_FILETYPE_FORMAT,
        ERROR_UNSUPPORTED_READ_FOR_FILEFORMAT,
        ERROR_URI_FILE_OFFSET_NOT_SUPPORTED,
        ERROR_UNSUPPORTED_ABSOLUTE_PATH,
        ERROR_MISSING_VERTEX_POSITIONS,
        ERROR_FAULTY_GLTF_VERTEX_POSITIONS,
        ERROR_MISSING_VERTEX_TEXCOORD_0,
        ERROR_FAULTY_GLTF_VERTEX_TEXCOORD_0,
    };
    static auto to_string(AssetLoadResultCode code) -> std::string_view
    {
        switch(code)
        {
            case AssetLoadResultCode::SUCCESS:                                          return "SUCCESS";
            case AssetLoadResultCode::ERROR_MISSING_INDEX_BUFFER:                       return "ERROR_MISSING_INDEX_BUFFER";
            case AssetLoadResultCode::ERROR_FAULTY_INDEX_BUFFER_GLTF_ACCESSOR:          return "ERROR_FAULTY_INDEX_BUFFER_GLTF_ACCESSOR";
            case AssetLoadResultCode::ERROR_FAULTY_BUFFER_VIEW:                         return "ERROR_FAULTY_BUFFER_VIEW";
            case AssetLoadResultCode::ERROR_COULD_NOT_OPEN_GLTF:                        return "ERROR_COULD_NOT_OPEN_GLTF";
            case AssetLoadResultCode::ERROR_COULD_NOT_READ_BUFFER_IN_GLTF:              return "ERROR_COULD_NOT_READ_BUFFER_IN_GLTF";
            case AssetLoadResultCode::ERROR_COULD_NOT_OPEN_TEXTURE_FILE:                return "ERROR_COULD_NOT_OPEN_TEXTURE_FILE";
            case AssetLoadResultCode::ERROR_COULD_NOT_READ_TEXTURE_FILE:                return "ERROR_COULD_NOT_READ_TEXTURE_FILE";
            case AssetLoadResultCode::ERROR_COULD_NOT_READ_TEXTURE_FILE_FROM_MEMSTREAM: return "ERROR_COULD_NOT_READ_TEXTURE_FILE_FROM_MEMSTREAM";
            case AssetLoadResultCode::ERROR_UNSUPPORTED_TEXTURE_PIXEL_FORMAT:           return "ERROR_UNSUPPORTED_TEXTURE_PIXEL_FORMAT";
            case AssetLoadResultCode::ERROR_UNKNOWN_FILETYPE_FORMAT:                    return "ERROR_UNKNOWN_FILETYPE_FORMAT";
            case AssetLoadResultCode::ERROR_UNSUPPORTED_READ_FOR_FILEFORMAT:            return "ERROR_UNSUPPORTED_READ_FOR_FILEFORMAT";
            case AssetLoadResultCode::ERROR_URI_FILE_OFFSET_NOT_SUPPORTED:              return "ERROR_URI_FILE_OFFSET_NOT_SUPPORTED";
            case AssetLoadResultCode::ERROR_UNSUPPORTED_ABSOLUTE_PATH:                  return "ERROR_UNSUPPORTED_ABSOLUTE_PATH";
            case AssetLoadResultCode::ERROR_MISSING_VERTEX_POSITIONS:                   return "ERROR_MISSING_VERTEX_POSITIONS";
            case AssetLoadResultCode::ERROR_FAULTY_GLTF_VERTEX_POSITIONS:               return "ERROR_FAULTY_GLTF_VERTEX_POSITIONS";
            case AssetLoadResultCode::ERROR_MISSING_VERTEX_TEXCOORD_0:                  return "ERROR_MISSING_VERTEX_TEXCOORD_0";
            case AssetLoadResultCode::ERROR_FAULTY_GLTF_VERTEX_TEXCOORD_0:              return "ERROR_FAULTY_GLTF_VERTEX_TEXCOORD_0";
            default: return "UNKNOWN";
        }
    }
    AssetProcessor(std::shared_ptr<ff::Device> device);
    AssetProcessor(AssetProcessor &&) = default;
    ~AssetProcessor();

    using NonmanifestLoadRet = std::variant<AssetProcessor::AssetLoadResultCode, ff::ImageId>;
    auto load_nonmanifest_texture(std::filesystem::path const & filepath) -> NonmanifestLoadRet;

    auto load_texture(Scene &scene, u32 texture_manifest_index) -> AssetLoadResultCode;
    auto load_mesh_group(Scene &scene, u32 mesh_group_manifest_index) -> AssetLoadResultCode;

    auto load_all(Scene& scene) -> AssetLoadResultCode;

    void record_gpu_load_processing_commands(Scene & scene);

private:
    std::vector<u32> indices = {};
    std::vector<f32vec3> positions = {};
    std::vector<f32vec2> uvs = {};
    static inline const std::string VERT_ATTRIB_POSITION_NAME = "POSITION";
    static inline const std::string VERT_ATTRIB_NORMAL_NAME = "NORMAL";
    static inline const std::string VERT_ATTRIB_TEXCOORD0_NAME = "TEXCOORD_0";

    struct TextureUpload
    {
        Scene *scene = {};
        ff::BufferId staging_buffer = {};
        ff::ImageId dst_image = {};
        u32 texture_manifest_index = {};
    };

    struct MeshUpload
    {
        // TODO: replace with buffer offset into staging memory.
        Scene *scene = {};
        ff::BufferId staging_buffer = {};
        u32 mesh_manifest_index = {};
    };
    std::shared_ptr<ff::Device> _device = {};
    // TODO: Replace with lockless queue.
    std::vector<MeshUpload> _upload_mesh_queue = {};
    std::vector<TextureUpload> _upload_texture_queue = {};

    auto load_mesh(Scene & scene, u32 mesh_manifest_index) -> AssetProcessor::AssetLoadResultCode;
};