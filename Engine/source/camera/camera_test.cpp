#include "camera/camera_test.hpp"

#include "camera/camera_rts_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/resource.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"
#include "rendering/debug_render.hpp"
#include "rendering/image.hpp"
#include "rendering/mesh.hpp"
#include "rendering/model.hpp"
#include "rendering/render_components.hpp"

using namespace bee;
using namespace std;

// This and the CreatePlane function are here only to make a plane that is used for orientation while testing the camera
std::shared_ptr<Mesh> CreatePlaneMesh(float size)
{
    auto mesh = Engine.Resources().Create<Mesh>();

    auto indices = std::vector<uint16_t>{0, 1, 2, 0, 2, 3};
    mesh->SetIndices(indices);

    auto positions = vector<glm::vec3>{glm::vec3(-size, -size, 0.0f), glm::vec3(size, -size, 0.0f), glm::vec3(size, size, 0.0f),
                                       glm::vec3(-size, size, 0.0f)};
    mesh->SetAttribute(Mesh::Attribute::Position, positions);

    auto normals = vector<glm::vec3>{glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                                     glm::vec3(0.0f, 0.0f, 1.0f)};
    mesh->SetAttribute(Mesh::Attribute::Normal, normals);

    auto uvs = vector<glm::vec2>{glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f)};
    mesh->SetAttribute(Mesh::Attribute::Texture, uvs);

    return mesh;
}

void CreatePlane(float size, float scale, const std::string& path)
{
    auto plane = Engine.ECS().CreateEntity();

    auto& transform = Engine.ECS().CreateComponent<Transform>(plane);
    transform.Name = "Plane";
    transform.Scale = glm::vec3(scale, scale, 1.0f);

    auto mesh = CreatePlaneMesh(size);
    auto material = std::make_shared<Material>();
    if (!path.empty())
    {
        auto image = Engine.Resources().Load<Image>(path);
        auto sampler = std::make_shared<Sampler>();
        sampler->MinFilter = Sampler::Filter::Nearest;
        sampler->MagFilter = Sampler::Filter::Nearest;
        sampler->WrapS = Sampler::Wrap::Repeat;
        sampler->WrapT = Sampler::Wrap::Repeat;

        auto texture = std::make_shared<Texture>(image, sampler);
        material->BaseColorTexture = texture;
        material->UseBaseTexture = true;
        material->IsUnlit = true;
    }

    else
    {
        material->BaseColorFactor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        material->IsUnlit = true;
    }

    auto& meshRenderer = Engine.ECS().CreateComponent<MeshRenderer>(plane, mesh, material);
}

void CameraTest::Init()
{
#ifdef BEE_PLATFORM_PC
    auto& renderer = Engine.ECS().CreateSystem<bee::RenderPipeline>();
#endif
    // creating a ligth source
    auto light = Engine.Resources().Load<Model>("models/LightsOnly.gltf");
    light->Instantiate();

    // adding a model to the scene
    auto model = Engine.Resources().Load<Model>("models/Mecha.gltf");
    model->Instantiate();

    CreatePlane(1.0f, 30.0f, "");

    Engine.ECS().CreateSystem<CameraSystemRTS>();
}
