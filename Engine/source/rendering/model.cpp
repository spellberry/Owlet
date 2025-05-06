#include "rendering/model.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <set>

#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"
#include "rendering/image.hpp"
#include "rendering/mesh.hpp"
#include <platform/dx12/skeleton.hpp>
#include <platform/dx12/skeletal_animation.hpp>
#include <platform/dx12/animation_system.hpp>

//#include <material_system/material_system.hpp>
//#include <Dx12NiceRenderer\Renderer\include\mesh_dx12.h>
//#include <platform/opengl/mesh_gl.hpp>
#include "tools/log.hpp"
#include "tools/tools.hpp"

using namespace bee;
using namespace std;

Model::Model(const std::string& filename) : Resource(ResourceType::Model)
{
    m_path = filename;
    
    const string fullFilename = Engine.FileIO().GetPath(FileIO::Directory::Asset, filename);

    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool res = false;

    // Check which format to load
    if (StringEndsWith(filename, ".gltf"))
    {
        // auto path = Engine.FileIO().GetPath(FileIO::Directory::Asset, filename);
        res = loader.LoadASCIIFromFile(&m_model, &err, &warn, fullFilename);
        ////auto data = Engine.FileIO().ReadTextFile(FileIO::Directory::Asset, filename);
        // loader.LoadFromString(&m_model, &err, &warn, data.c_str(), data.length(), "");
    }
    else if (StringEndsWith(filename, ".glb"))
    {
        res = loader.LoadBinaryFromFile(&m_model, &err, &warn, fullFilename);
    }

    if (!warn.empty()) Log::Warn(warn);

    if (!err.empty()) Log::Error(err);

    if (!res)
        Log::Error("Failed to load glTF: {}", filename);
    else
        Log::Info("Loaded glTF: {}", filename);

    // Load meshes
    for (int i = 0; i < static_cast<int>(m_model.meshes.size()); i++)
    {
        auto mesh = Engine.Resources().Load<Mesh>(*this, i);
        m_meshes.push_back(mesh);
    }
    
    for (int i = 0; i < static_cast<int>(m_model.skins.size()); i++)
    {
     //   m_model.skins.
       
       // auto skin = m_model.skins[i];
     auto skeleton = make_shared<Skeleton>(*this, i);
     m_skeletons.push_back(skeleton);



    }

    for (int i = 0; i < static_cast<int>(m_model.animations.size()); i++)
    {
        auto animation = bee::Engine.Resources().Create<SkeletalAnimation>(*this, i, *m_skeletons[0].get());
        m_animations.push_back(animation);
    }

    // Load images (texture data)
    for (int i = 0; i < static_cast<int>(m_model.images.size()); i++)
    {
        auto image = Engine.Resources().Load<Image>(*this, i);
        m_images.push_back(image);
    }

    // Load samplers
    for (int i = 0; i < static_cast<int>(m_model.samplers.size()); i++)
    {
        auto sampler = make_shared<Sampler>(*this, i);
        m_samplers.push_back(sampler);
    }

    // Load textures
    for (int i = 0; i < static_cast<int>(m_model.textures.size()); i++)
    {
        auto texture = make_shared<Texture>(*this, i);
        m_textures.push_back(texture);
    }

    // Load materials
    for (int i = 0; i < static_cast<int>(m_model.materials.size()); i++)
    {
        auto material = make_shared<Material>(*this, i);

     //  material->material_index =  bee::Engine.ECS().GetSystem<MaterialSystem>().RegisterDefaultMaterial();

        m_materials.push_back(material);
    }

    // Load lights
    for (int i = 0; i < static_cast<int>(m_model.lights.size()); i++)
    {
        auto light = make_shared<Light>(*this, i);
        m_lights.push_back(light);
    }
}

Model::~Model() = default;

void Model::InstantiateNode(uint32_t nodeIdx, Entity parent, const ConstantBufferData constantData) const
{
    const auto& node = m_model.nodes[nodeIdx];
    const auto entity = Engine.ECS().CreateEntity();

    // Transform
    auto& transform = Engine.ECS().CreateComponent<Transform>(entity);
    transform.Name = node.name;
    if (parent != entt::null) transform.SetParent(parent);

    if (!node.matrix.empty())
    {
        glm::mat4 transformGLM = glm::make_mat4(node.matrix.data());
        Decompose(transformGLM, transform.Translation, transform.Scale, transform.Rotation);
    }
    else
    {
        if (!node.scale.empty()) transform.Scale = to_vec3(node.scale);
        if (!node.rotation.empty()) transform.Rotation = to_quat(node.rotation);
        if (!node.translation.empty()) transform.Translation = to_vec3(node.translation);
    }

    // Mesh
    if (node.mesh != -1)
    {
        auto& mesh = m_model.meshes[node.mesh];
        assert(!mesh.primitives.empty());

        auto& osmMesh = m_meshes[node.mesh];

        std::shared_ptr<bee::Skeleton> osmSkeleton;

        if (node.skin != -1)
        {
            //Creating an instance of the skeleton
            osmSkeleton = std::make_shared<bee::Skeleton>(*m_skeletons[node.skin]);
        }
        else
        {
            osmSkeleton = nullptr;
        }

        if (mesh.primitives[0].material != -1)
        {
            auto& osmMaterial = m_materials[mesh.primitives[0].material];
            auto& meshRenderer = Engine.ECS().CreateComponent<MeshRenderer>(entity, osmMesh, osmMaterial, osmSkeleton);
            // maybe this is wrong?
            meshRenderer.constant_data = constantData;
        }
        else
        {
            auto osmMaterial = make_shared<Material>();
            auto& meshRenderer =
                Engine.ECS().CreateComponent<MeshRenderer>(entity, osmMesh, osmMaterial, osmSkeleton);
            // maybe this is wrong?
            meshRenderer.constant_data = constantData;
        }
    }

    // Camera
    if (node.camera != -1)
    {
        auto camera = m_model.cameras[node.camera];
        glm::mat4 projection;
        if (camera.type == "perspective")
        {
            const auto& c = camera.perspective;
            float deviceAspectRatio = (float)Engine.Device().GetWidth() / (float)Engine.Device().GetHeight();
            float aspectRatio = (float)(c.aspectRatio == 0.0 ? deviceAspectRatio : c.aspectRatio);
            projection = glm::perspective((float)c.yfov, aspectRatio, (float)c.znear, (float)c.zfar);
        }
        else if (camera.type == "orthographic")
        {
            const auto& c = camera.orthographic;
            float hack = 1.0f / 1.77f;  // Orthographic is a bit broken in Blender
            projection =
                glm::ortho(c.xmag * -0.5f, c.xmag * 0.5f, c.ymag * -0.5f * hack, c.ymag * 0.5f * hack, c.znear, c.zfar);
        }
        Engine.ECS().CreateComponent<Camera>(entity, projection);
    }

    if (node.extensions.find("KHR_lights_punctual") != node.extensions.end())
    {
        auto& klp = node.extensions.at("KHR_lights_punctual");
        auto& l = klp.Get("light");
        int i = l.GetNumberAsInt();
        Engine.ECS().CreateComponent<Light>(entity, *m_lights[i]);
    }

    // Load children
    for (auto node : node.children) InstantiateNode(node, entity, constantData);
}


void Model::Instantiate(Entity parent, const ConstantBufferData constantData) const
{
    for (const uint32_t node : m_model.scenes[0].nodes) 
        InstantiateNode(node, parent, constantData);
}

MeshRenderer Model::CreateMeshRendererFromNode(const std::string& nodeName) const
{
    MeshRenderer result;

    const tinygltf::Node* node = nullptr;
    for (auto& n : m_model.nodes)
        if (n.name == nodeName) node = &n;

    if (node && node->mesh != -1)
    {
        result.Mesh = m_meshes[node->mesh];

        if (node->skin != -1)
        {
            result.Skeleton = std::make_shared<bee::Skeleton>(*m_skeletons[node->skin]);
        }
        else
        {
            result.Skeleton = nullptr;
        }

        const auto gltfMaterial = m_model.meshes[node->mesh].primitives[0].material;
        if (gltfMaterial != -1) result.Material = m_materials[gltfMaterial];
    }

    return result;
}
