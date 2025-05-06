#pragma once

#include <tinygltf/tiny_gltf.h>

#include <memory>
#include <string>
#include <vector>

#include "core/ecs.hpp"
#include "core/resource.hpp"
#include "rendering/render_components.hpp"

namespace bee
{

struct Material;
class Mesh;
struct Texture;
struct Sampler;
struct Light;
class Image;
struct Skeleton;
class SkeletalAnimation;
/// <summary>
/// Represents a model that can be read from a GLTF file.
/// </summary>
class Model : public Resource
{
public:
    /// <summary>
    /// Creates a new model by loading it from a given GLTF file.
    /// </summary>
    Model(const std::string& filename);
    ~Model() override;
    const tinygltf::Model& GetDocument() const { return m_model; }
    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_meshes; }
    const std::vector<std::shared_ptr<Texture>>& GetTextures() const { return m_textures; }
    const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return m_materials; }
    const std::vector<std::shared_ptr<Image>>& GetImages() const { return m_images; }
    const std::vector<std::shared_ptr<Sampler>>& GetSamplers() const { return m_samplers; }
    const std::vector<std::shared_ptr<Light>>& GetLights() const { return m_lights; }

    /// <summary>
    /// Instantiates new entities and components with the contents of this model.
    /// </summary>
    /// <param name="parent">An optional parent entity. If specified, the newly created entities will be linked to this parent.</param>
    void Instantiate(Entity parent = entt::null, const ConstantBufferData constantData = ConstantBufferData()) const;

    /// <summary>
    /// Creates a MeshRenderer containing the mesh and material corresponding to the GLTF node with the given name.
    /// If no node with this name exists in the model, the MeshRenderer will store nullptr for the mesh/material.
    /// </summary>
    MeshRenderer CreateMeshRendererFromNode(const std::string& nodeName) const;


    template <typename T>
     int LoadAccessor(const tinygltf::Accessor& accessor, const T*& pointer, unsigned int* count = nullptr, int* type = nullptr) const 
    {
        const tinygltf::BufferView& view = m_model.bufferViews[accessor.bufferView];

        pointer = reinterpret_cast<const T*>(&(m_model.buffers[view.buffer].data[accessor.byteOffset+view.byteOffset]));

        if (count)
        {
            count[0] = static_cast<unsigned int>(accessor.count);
        }
        if (type)
        {
            type[0] = accessor.type;
        }
        return accessor.componentType;
    }


private:
    void InstantiateNode(uint32_t nodeIdx, Entity parent, const ConstantBufferData constantData = ConstantBufferData()) const;

protected:
    tinygltf::Model m_model;
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::vector<std::shared_ptr<Skeleton>> m_skeletons;
    std::vector<std::shared_ptr<SkeletalAnimation>> m_animations;
    std::vector<std::shared_ptr<Texture>> m_textures;
    std::vector<std::shared_ptr<Image>> m_images;
    std::vector<std::shared_ptr<Material>> m_materials;
    std::vector<std::shared_ptr<Sampler>> m_samplers;
    std::vector<std::shared_ptr<Light>> m_lights;
};

}  // namespace bee
