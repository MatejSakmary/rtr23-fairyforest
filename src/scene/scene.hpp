#pragma once

#include <optional>
#include <variant>

#include <fastgltf/parser.hpp>
#include <fastgltf/types.hpp>
#include "../fairy_forest.hpp"

#include "../shared/shared.inl"
#include "../backend/slotmap.hpp"
using namespace ff::types;
#define MAX_MESHES_PER_MESHGROUP 105

struct TextureManifestEntry
{
    struct MaterialManifestIndex
    {
        bool diffuse = {}; 
        bool normal = {};
        bool metalic = {};
        bool roughness = {};
        u32 material_manifest_index = {};
    };
    u32 scene_file_manifest_index = {};
    u32 in_scene_file_index = {};
    // List of materials that use this texture and how they use it 
    // The GPUMaterial contrains ImageIds directly,
    // So the GPUMaterial Need to be updated when the texture changes.
    std::vector<MaterialManifestIndex> material_manifest_indices = {};
    std::optional<ff::ImageId> runtime = {};
    std::string name = {};
};

struct MaterialManifestEntry
{
    std::optional<u32> diffuse_tex_index = {};
    std::optional<u32> normal_tex_index = {};
    u32 scene_file_manifest_index = {};
    u32 in_scene_file_index = {};
    std::string name = {};
};

struct MeshDescriptorCpu
{
    u32 vertex_count = {};
    u32 positions_offset = {};
    u32 uvs_offset = {};
    u32 tangents_offset = {};
    u32 normals_offset = {};
    u32 index_count = {};
    u32 indices_offset = {};
    u32 transforms_offset = {};
};

struct MeshManifestEntry
{
    std::optional<u32> material_manifest_index = {};
    u32 scene_file_manifest_index = {};
    u32 scene_file_mesh_index = {};
    u32 scene_file_primitive_index = {};
    std::optional<MeshDescriptor> gpu_runtime = {};
    std::optional<MeshDescriptorCpu> cpu_runtime = {};
};

struct MeshGroupManifestEntry
{
    std::array<u32, MAX_MESHES_PER_MESHGROUP> mesh_manifest_indices = {};
    std::vector<f32mat4x3> instance_transforms = {};
    u32 mesh_count = {};
    u32 scene_file_manifest_index = {};
    u32 in_scene_file_index = {};
    std::string name = {};
};

struct RenderEntity;
using RenderEntityId = ff::SlotMap<RenderEntity>::Id;

enum struct EntityType
{
    ROOT,
    TRANSFORM,
    LIGHT,
    CAMERA,
    MESHGROUP,
    UNKNOWN
};

struct RenderEntity
{
    glm::mat4x3 transform = {};
    std::optional<RenderEntityId> first_child = {};
    std::optional<RenderEntityId> next_sibling = {};
    std::optional<RenderEntityId> parent = {};
    std::optional<u32> mesh_group_manifest_index = {};
    EntityType type = EntityType::UNKNOWN;
    std::string name = {};
};

struct SceneFileManifestEntry
{
    std::filesystem::path path = {};
    fastgltf::Asset gltf_asset{};
    u32 texture_manifest_offset = {};
    u32 material_manifest_offset = {};
    u32 mesh_group_manifest_offset = {};
    u32 mesh_manifest_offset = {};
    RenderEntityId root_render_entity = {};
};

using RenderEntitySlotMap = ff::SlotMap<RenderEntity>;

struct DrawCommand
{
    u32 mesh_idx = {};
    u32 vertex_count = {};
    u32 index_count = {};
    u32 index_offset = {};
    u32 instance_count = {};
};
struct SceneDrawCommands 
{
    bool enable_ao = {};
    VkDeviceAddress scene_descriptor = {};
    std::vector<DrawCommand> draw_commands = {};
};

struct Scene
{
    ff::BufferId _gpu_materials = {};
    ff::BufferId _gpu_mesh_transforms = {};
    ff::BufferId _gpu_mesh_positions = {};
    ff::BufferId _gpu_mesh_tangents = {};
    ff::BufferId _gpu_mesh_normals = {};
    ff::BufferId _gpu_mesh_uvs = {};
    ff::BufferId _gpu_mesh_indices = {};

    ff::BufferId _gpu_mesh_descriptors = {};
    ff::BufferId _gpu_material_descriptors = {};

    ff::BufferId _gpu_scene_descriptor = {};

    RenderEntitySlotMap _render_entities = {};
    std::vector<RenderEntityId> _dirty_render_entities = {}; 

    std::vector<SceneFileManifestEntry> _scene_file_manifest = {};
    std::vector<TextureManifestEntry> _material_texture_manifest = {};
    std::vector<MaterialManifestEntry> _material_manifest = {};
    std::vector<MeshManifestEntry> _mesh_manifest = {};
    std::vector<MeshGroupManifestEntry> _mesh_group_manifest = {};
    // Count the added meshes and meshgroups when loading.
    // Used to do the initialization of these on the gpu when recording manifest update.
    u32 _new_mesh_manifest_entries = {};
    u32 _new_mesh_group_manifest_entries = {};
    u32 _new_material_manifest_entries = {};

    Scene(std::shared_ptr<ff::Device> device);
    ~Scene();

    enum struct LoadManifestErrorCode
    {
        FILE_NOT_FOUND,
        COULD_NOT_LOAD_ASSET,
        INVALID_GLTF_FILE_TYPE,
        COULD_NOT_PARSE_ASSET_NODES,
    };
    static auto to_string(LoadManifestErrorCode result) -> std::string_view
    {
        switch(result)
        {
            case LoadManifestErrorCode::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
            case LoadManifestErrorCode::COULD_NOT_LOAD_ASSET: return "COULD_NOT_LOAD_ASSET";
            case LoadManifestErrorCode::INVALID_GLTF_FILE_TYPE: return "INVALID_GLTF_FILE_TYPE";
            case LoadManifestErrorCode::COULD_NOT_PARSE_ASSET_NODES: return "COULD_NOT_PARSE_ASSET_NODES";
            default: return "UNKNOWN";
        }
        return "UNKNOWN";
    }
    auto load_manifest_from_gltf(std::filesystem::path const& root_path, std::filesystem::path const& glb_name) -> std::variant<RenderEntityId, LoadManifestErrorCode>;

    auto record_scene_draw_commands() -> SceneDrawCommands;

    std::shared_ptr<ff::Device> _device = {};
};