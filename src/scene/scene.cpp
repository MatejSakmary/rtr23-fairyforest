#include "scene.hpp"

#include <fastgltf/parser.hpp>
#include <fstream>

#include <fmt/format.h>
#include <glm/gtx/quaternion.hpp>

Scene::Scene(std::shared_ptr<ff::Device> device)
    : _device{device}
{
}

Scene::~Scene()
{
    _device->destroy_buffer(_gpu_mesh_transforms);
    _device->destroy_buffer(_gpu_mesh_positions);
    _device->destroy_buffer(_gpu_mesh_uvs);
    _device->destroy_buffer(_gpu_mesh_indices);
    _device->destroy_buffer(_gpu_mesh_descriptors);
    _device->destroy_buffer(_gpu_scene_descriptor);
    _device->destroy_buffer(_gpu_material_descriptors);
    for(auto & texture : _material_texture_manifest)
    {
        if(texture.runtime.has_value())
        {
            _device->destroy_image(texture.runtime.value());
        }
    };
}

// TODO: Loading god function.
auto Scene::load_manifest_from_gltf(std::filesystem::path const & root_path, std::filesystem::path const & glb_name) -> std::variant<RenderEntityId, LoadManifestErrorCode>
{
#pragma region SETUP
    auto file_path = root_path / glb_name;

    fastgltf::Parser parser{};

    constexpr auto gltf_options =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble;

    fastgltf::GltfDataBuffer data;
    bool const worked = data.loadFromFile(file_path);
    if (!worked)
    {
        return LoadManifestErrorCode::FILE_NOT_FOUND;
    }
    auto type = fastgltf::determineGltfFileType(&data);
    fastgltf::Asset asset;
    switch (type)
    {
        case fastgltf::GltfType::glTF:
        {
            fastgltf::Expected<fastgltf::Asset> result = parser.loadGLTF(&data, file_path.parent_path(), gltf_options);
            APP_LOG(fmt::format("{}",static_cast<u32>(result.error())));
            if (result.error() != fastgltf::Error::None)
            {
                return LoadManifestErrorCode::COULD_NOT_LOAD_ASSET;
            }
            asset = std::move(result.get());
            break;
        }
        case fastgltf::GltfType::GLB:
        {
            fastgltf::Expected<fastgltf::Asset> result = parser.loadBinaryGLTF(&data, file_path.parent_path(), gltf_options);
            if (result.error() != fastgltf::Error::None)
            {
                return LoadManifestErrorCode::COULD_NOT_LOAD_ASSET;
            }
            asset = std::move(result.get());
            break;
        }
        default:
            return LoadManifestErrorCode::INVALID_GLTF_FILE_TYPE;
    }

    u32 const scene_file_manifest_index = static_cast<u32>(_scene_file_manifest.size());
    u32 const texture_manifest_offset = static_cast<u32>(_material_texture_manifest.size());
    u32 const material_manifest_offset = static_cast<u32>(_material_manifest.size());
    u32 const mesh_group_manifest_offset = static_cast<u32>(_mesh_group_manifest.size());
    u32 const mesh_manifest_offset = static_cast<u32>(_mesh_manifest.size());
#pragma endregion

#pragma region POPULATE_TEXTURE_MANIFEST
    /// NOTE: Texture = image + sampler - since we don't care about the samplers we only load the images.
    //        Later when we load in the materials which reference the textures rather than images we just
    //        translate the textures image index and store that in the material
    for (u32 i = 0; i < static_cast<u32>(asset.images.size()); ++i)
    {
        u32 const texture_manifest_index = static_cast<u32>(_material_texture_manifest.size());
        _material_texture_manifest.push_back(TextureManifestEntry{
            .scene_file_manifest_index = scene_file_manifest_index,
            .in_scene_file_index = i,
            .material_manifest_indices = {}, // Filled when reading in materials
            .runtime = {},                   // Loaded later
            .name = asset.images[i].name.c_str(),
        });
    }
#pragma endregion

#pragma region POPULATE_MATERIAL_MANIFEST
    for (u32 material_index = 0; material_index < static_cast<u32>(asset.materials.size()); material_index++)
    {
        /// NOTE: Because we previously only loaded the images and not textures we now need to translate
        //        the texture indices into image indeces and store that
        auto gltf_texture_to_manifest_texture_index = [&](u32 const texture_index) -> std::optional<u32>
        {
            const bool gltf_texture_has_image_index = asset.textures.at(texture_index).imageIndex.has_value();
            if (!gltf_texture_has_image_index)
            {
                return std::nullopt;
            }
            else
            {
                return static_cast<u32>(asset.textures.at(texture_index).imageIndex.value()) + texture_manifest_offset;
            }
        };

        auto const & material = asset.materials.at(material_index);
        /// NOTE: This will not work once we add multiple threads since some other thread might push to the vector
        //        while we are marking the textures as being used by this material
        u32 const material_manifest_index = _material_manifest.size();
        bool const has_normal_texture = material.normalTexture.has_value();
        bool const has_diffuse_texture = material.pbrData.baseColorTexture.has_value();
        std::optional<u32> diffuse_texture_index = {};
        std::optional<u32> normal_texture_index = {};
        if (has_diffuse_texture)
        {
            u32 const texture_index = static_cast<u32>(material.pbrData.baseColorTexture.value().textureIndex);
            diffuse_texture_index = gltf_texture_to_manifest_texture_index(texture_index);
            _material_texture_manifest.at(diffuse_texture_index.value()).material_manifest_indices.push_back({.diffuse = true, .material_manifest_index = material_manifest_index});
        }
        if (has_normal_texture)
        {
            u32 const texture_index = static_cast<u32>(material.normalTexture.value().textureIndex);
            normal_texture_index = gltf_texture_to_manifest_texture_index(texture_index);
            _material_texture_manifest.at(normal_texture_index.value()).material_manifest_indices.push_back({.normal = true, .material_manifest_index = material_manifest_index});
        }
        _material_manifest.push_back(MaterialManifestEntry{
            .diffuse_tex_index = diffuse_texture_index,
            .normal_tex_index = normal_texture_index,
            .scene_file_manifest_index = scene_file_manifest_index,
            .in_scene_file_index = material_index,
            .name = material.name.c_str()});
        _new_material_manifest_entries += 1;
    }
#pragma endregion

#pragma region POPULATE_MESHGROUP_AND_MESH_MANIFEST
    /// NOTE: fastgltf::Mesh is a MeshGroup
    std::array<u32, MAX_MESHES_PER_MESHGROUP> mesh_manifest_indices;
    for (u32 mesh_group_index = 0; mesh_group_index < static_cast<u32>(asset.meshes.size()); mesh_group_index++)
    {
        auto const & mesh_group = asset.meshes.at(mesh_group_index);
        u32 const mesh_group_manifest_index = static_cast<u32>(_mesh_group_manifest.size());
        /// NOTE: fastgltf::Primitive is Mesh
        for (u32 mesh_index = 0; mesh_index < static_cast<u32>(mesh_group.primitives.size()); mesh_index++)
        {
            u32 const mesh_manifest_entry = _mesh_manifest.size();
            auto const & mesh = mesh_group.primitives.at(mesh_index);
            mesh_manifest_indices.at(mesh_index) = mesh_manifest_entry;
            std::optional<u32> material_manifest_index = mesh.materialIndex.has_value() ? std::optional{static_cast<u32>(mesh.materialIndex.value()) + material_manifest_offset} : std::nullopt;
            _mesh_manifest.push_back(MeshManifestEntry{
                .material_manifest_index = std::move(material_manifest_index),
                .scene_file_manifest_index = scene_file_manifest_index,
                .scene_file_mesh_index = mesh_group_index,
                .scene_file_primitive_index = mesh_index,
            });
            _new_mesh_manifest_entries += 1;
        }

        _mesh_group_manifest.push_back(MeshGroupManifestEntry{
            .mesh_manifest_indices = std::move(mesh_manifest_indices),
            .mesh_count = static_cast<u32>(mesh_group.primitives.size()),
            .scene_file_manifest_index = scene_file_manifest_index,
            .in_scene_file_index = mesh_group_index,
            .name = mesh_group.name.c_str()});
        _new_mesh_group_manifest_entries += 1;
        mesh_manifest_indices.fill(0u);
    }
#pragma endregion

#pragma region POPULATE_RENDER_ENTITIES
    /// NOTE: fastgltf::Node is Entity
    DBG_ASSERT_TRUE_M(asset.nodes.size() != 0, "[ERROR][load_manifest_from_gltf()] Empty node array - what to do now?");
    std::vector<RenderEntityId> node_index_to_entity_id = {};
    /// NOTE: Here we allocate space for each entity and create a translation table between node index and entity id
    for (u32 node_index = 0; node_index < static_cast<u32>(asset.nodes.size()); node_index++)
    {
        node_index_to_entity_id.push_back(_render_entities.create_slot());
        _dirty_render_entities.push_back(node_index_to_entity_id.back());
    }
    for (u32 node_index = 0; node_index < static_cast<u32>(asset.nodes.size()); node_index++)
    {
        // TODO: For now store transform as a matrix - later should be changed to something else (TRS: translation, rotor, scale).
        auto fastgltf_to_glm_mat4x3_transform = [](std::variant<fastgltf::Node::TRS, fastgltf::Node::TransformMatrix> const & trans) -> glm::mat4x3
        {
            glm::mat4x3 ret_trans;
            if (auto const * trs = std::get_if<fastgltf::Node::TRS>(&trans))
            {
                auto const scaled = glm::scale(glm::identity<glm::mat4x4>(), glm::vec3(trs->scale[0], trs->scale[1], trs->scale[2]));
                auto const quat_rotation_mat = glm::toMat4(glm::quat(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2])) * scaled;
                auto const rotated_scaled = quat_rotation_mat * scaled;
                auto const translated_rotated_scaled = glm::translate(
                                                           glm::identity<glm::mat4x4>(),
                                                           glm::vec3(trs->translation[0], trs->translation[1], trs->translation[2])) *
                                                       rotated_scaled;
                /// NOTE: As the last row is always (0,0,0,1) we dont store it.
                ret_trans = glm::mat4x3(translated_rotated_scaled);
            }
            else if (auto const * trs = std::get_if<fastgltf::Node::TransformMatrix>(&trans))
            {
                // Gltf and glm matrices are column major.
                ret_trans = glm::mat4x3(std::bit_cast<glm::mat4x4>(*trs));
            }
            return ret_trans;
        };

        fastgltf::Node const & node = asset.nodes[node_index];
        RenderEntityId const parent_r_ent_id = node_index_to_entity_id[node_index];
        RenderEntity & r_ent = *_render_entities.slot(parent_r_ent_id);
        if (node.meshIndex.has_value())
        {
            r_ent.mesh_group_manifest_index = static_cast<u32>(node.meshIndex.value()) + mesh_group_manifest_offset;
        }
        else
        {
            r_ent.mesh_group_manifest_index = std::optional<u32>(std::nullopt);
        }
        r_ent.transform = fastgltf_to_glm_mat4x3_transform(node.transform);
        r_ent.name = node.name.c_str();
        if (node.meshIndex.has_value())
        {
            r_ent.type = EntityType::MESHGROUP;
        }
        else if (node.cameraIndex.has_value())
        {
            r_ent.type = EntityType::CAMERA;
        }
        else if (node.lightIndex.has_value())
        {
            r_ent.type = EntityType::LIGHT;
        }
        else if (!node.children.empty())
        {
            r_ent.type = EntityType::TRANSFORM;
        }
        if (!node.children.empty())
        {
            r_ent.first_child = node_index_to_entity_id[node.children[0]];
        }
        for (u32 curr_child_vec_idx = 0; curr_child_vec_idx < node.children.size(); curr_child_vec_idx++)
        {
            u32 const curr_child_node_idx = node.children[curr_child_vec_idx];
            RenderEntityId const curr_child_r_ent_id = node_index_to_entity_id[curr_child_node_idx];
            RenderEntity & curr_child_r_ent = *_render_entities.slot(curr_child_r_ent_id);
            curr_child_r_ent.parent = parent_r_ent_id;
            bool const has_next_sibling = curr_child_vec_idx < (node.children.size() - 1ull);
            if (has_next_sibling)
            {
                RenderEntityId const next_r_ent_child_id = node_index_to_entity_id[node.children[curr_child_vec_idx + 1]];
                curr_child_r_ent.next_sibling = next_r_ent_child_id;
            }
        }
    }

    /// NOTE: Find all root render entities (aka render entities that have no parent) and store them as
    //        Child root entites under scene root node
    RenderEntityId root_r_ent_id = _render_entities.create_slot({
        .transform = glm::mat4x3(glm::identity<glm::mat4x3>()),
        .first_child = std::nullopt,
        .next_sibling = std::nullopt,
        .parent = std::nullopt,
        .mesh_group_manifest_index = std::nullopt,
        .name = glb_name.filename().replace_extension("").string() + "_" + std::to_string(scene_file_manifest_index),
    });
    _dirty_render_entities.push_back(root_r_ent_id);
    RenderEntity & root_r_ent = *_render_entities.slot(root_r_ent_id);
    root_r_ent.type = EntityType::ROOT;
    std::optional<RenderEntityId> root_r_ent_prev_child = {};
    for (u32 node_index = 0; node_index < static_cast<u32>(asset.nodes.size()); node_index++)
    {
        RenderEntityId const r_ent_id = node_index_to_entity_id[node_index];
        RenderEntity & r_ent = *_render_entities.slot(r_ent_id);
        if (!r_ent.parent.has_value())
        {
            r_ent.parent = root_r_ent_id;
            if (!root_r_ent_prev_child.has_value()) // First child
            {
                root_r_ent.first_child = r_ent_id;
            }
            else // We have other root children already
            {
                _render_entities.slot(root_r_ent_prev_child.value())->next_sibling = r_ent_id;
            }
            root_r_ent_prev_child = r_ent_id;
        }
    }

#pragma endregion

    _scene_file_manifest.push_back(SceneFileManifestEntry{
        .path = file_path,
        .gltf_asset = std::move(asset),
        .texture_manifest_offset = texture_manifest_offset,
        .material_manifest_offset = material_manifest_offset,
        .mesh_group_manifest_offset = mesh_group_manifest_offset,
        .mesh_manifest_offset = mesh_manifest_offset,
        .root_render_entity = root_r_ent_id,
    });
    return root_r_ent_id;
}

auto Scene::record_scene_draw_commands() -> SceneDrawCommands
{
    SceneDrawCommands commands = {};
    commands.scene_descriptor = _device->get_buffer_device_address(_gpu_scene_descriptor);
    u32 global_mesh_idx = 0;
    for(auto const & mesh_group : _mesh_group_manifest)
    {
        for(u32 mesh_index = 0; mesh_index < mesh_group.mesh_count; mesh_index++)
        {
            auto const & mesh = _mesh_manifest.at(mesh_group.mesh_manifest_indices.at(mesh_index));
            commands.draw_commands.push_back({
                .mesh_idx = global_mesh_idx,
                .vertex_count = mesh.cpu_runtime->vertex_count,
                .index_count = mesh.cpu_runtime->index_count,
                .index_offset = mesh.cpu_runtime->indices_offset,
                .instance_count = static_cast<u32>(mesh_group.instance_transforms.size())
            });
            global_mesh_idx += 1;
        }
    }
    return commands;
}