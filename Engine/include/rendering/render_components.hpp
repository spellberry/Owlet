#pragma once

#include <glm/glm.hpp>
#include "cereal/cereal.hpp"
#include "tools/serialize_glm.h"
#include <memory>
#include <visit_struct/visit_struct.hpp>
#include "tools/serializable.hpp"
//#include <Dx12NiceRenderer\Renderer\include\skeleton.hpp>
//#include "Dx12NiceRenderer/Renderer/include/ResourceManager.hpp"
#include <DirectXMath.h>



#include <string>

#include "core/resource.hpp"

namespace bee
{

class Model;
class Mesh;
struct Skeleton;
class AnimationInstance;
struct Texture;
class Image;

struct ConstantBufferData
{
    // Constructor with default values
    ConstantBufferData(DirectX::XMFLOAT4X4 wvpMat = DirectX::XMFLOAT4X4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                                                        0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
                       DirectX::XMFLOAT4X4 wMat = DirectX::XMFLOAT4X4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                                                      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
                       float red = 1.0f, float green = 1.0f, float blue = 1.0f, float opacity = 1.0f, int materialIndex = 0,
                       float paddin1 = 0, int paddin2 = 0, int paddin3 = 0)
        : wvpMat(wvpMat),
          wMat(wMat),
          red(red),
          green(green),
          blue(blue),
          opacity(opacity),
          materialIndex(materialIndex),
          paddin1(paddin1),
          paddin2(paddin2),
          paddin3(paddin3)
    {
    }

    DirectX::XMFLOAT4X4 wvpMat;
    DirectX::XMFLOAT4X4 wMat;
   // DirectX::XMFLOAT4 camera_position;

  
    float red = 1;
    float green = 1.0f;
    float blue = 1.0f;

    float opacity = 1.0f;

    int materialIndex;

    float paddin1;
    int paddin2;
    int paddin3;
};

class Material : public Resource
{
public:
    Material(const Model& model, int index);
    Material();
    Material(const std::string& filename);
    Material(const Material& newMat);

    std::string Name = "Material";
    
    glm::vec4 BaseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // 16
    bool UseBaseTexture = false;                                    // 4

    glm::vec3 EmissiveFactor = glm::vec3(0.0f, 0.0f, 0.0f);  // 12
    bool UseEmissiveTexture = false;                         // 4

    float NormalTextureScale = 0.0f;  // 4
    bool UseNormalTexture = false;    // 4

    float OcclusionTextureStrength = 0.0f;  // 4
    bool UseOcclusionTexture = false;       // 4

    bool UseMetallicRoughnessTexture = false;  // 4
    float MetallicFactor = 0.0f;               // 4
    float RoughnessFactor = 1.0f;              // 4

    bool IsUnlit = false;  // 4
    bool ReceiveShadows = true;

    unsigned int material_index=0;

    std::shared_ptr<Texture> BaseColorTexture;
    std::string BaseColorTexturePath;
    std::shared_ptr<Texture> EmissiveTexture;
    std::string EmissiveTexturePath;
    std::shared_ptr<Texture> NormalTexture;
    std::string NormalTexturePath;
    std::shared_ptr<Texture> OcclusionTexture;
    std::string OcclusionTexturePath;
    std::shared_ptr<Texture> MetallicRoughnessTexture;
    std::string MetallicRoughnessTexturePath;


    template <class Archive>
        void serialize(Archive& archive) {
        archive(CEREAL_NVP(Name),CEREAL_NVP(BaseColorFactor), CEREAL_NVP(UseBaseTexture),CEREAL_NVP(EmissiveFactor),
                CEREAL_NVP(UseEmissiveTexture), CEREAL_NVP(NormalTextureScale), CEREAL_NVP(UseNormalTexture),
                CEREAL_NVP(OcclusionTextureStrength), CEREAL_NVP(UseOcclusionTexture), CEREAL_NVP(UseMetallicRoughnessTexture),
                CEREAL_NVP(MetallicFactor), CEREAL_NVP(RoughnessFactor), CEREAL_NVP(IsUnlit), CEREAL_NVP(ReceiveShadows),CEREAL_NVP(BaseColorTexturePath),
                CEREAL_NVP(EmissiveTexturePath),CEREAL_NVP(NormalTexturePath),CEREAL_NVP(OcclusionTexturePath),CEREAL_NVP(MetallicRoughnessTexturePath)
            );
    }
    
};

struct Light : public Serializable
{
    enum class Type
    {
        Point,
        Directional,
        Spot
    };
    Light() = default;
    Light(const Model& model, int index);
    Light(const glm::vec3& color, float intensity, float range, Type type)
        : Color(color), Intensity(intensity), Range(range), Type(type) {}
    glm::vec3 Color = {};
    float Intensity = 0;
    float Range = 0;
    float ShadowExtent = 0.2f;
    bool CastShadows = false;
    Type Type = Type::Point;   

    template <class Archive>
    void save(Archive& archive) const
    {
        archive(CEREAL_NVP(Color), CEREAL_NVP(Intensity), CEREAL_NVP(Range), CEREAL_NVP(ShadowExtent), CEREAL_NVP(CastShadows),
                CEREAL_NVP(Type));
    }

    template <class Archive>
    void load(Archive& archive)
    {
        archive(CEREAL_NVP(Color), CEREAL_NVP(Intensity), CEREAL_NVP(Range), CEREAL_NVP(ShadowExtent), CEREAL_NVP(CastShadows),
                CEREAL_NVP(Type));
    }
};

struct Camera
{
    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 VP;

    // euler Angles
    float Yaw;
    float Pitch;

    glm::vec3 Front;
    glm::vec3 Right;
    glm::vec3 Up;
};

struct MeshRenderer
{
    std::shared_ptr<bee::Mesh> Mesh;
    std::shared_ptr<bee::Material> Material;
    std::shared_ptr<bee::Skeleton> Skeleton;

    ConstantBufferData constant_data;

   // bool b_HasSkeleton;
    MeshRenderer() {}

     MeshRenderer(std::shared_ptr<bee::Mesh> mesh, std::shared_ptr<bee::Material> material)
    {
        Mesh = mesh;
        Material = material;
        Skeleton = nullptr;
    }

    MeshRenderer(std::shared_ptr<bee::Mesh> mesh, std::shared_ptr<bee::Material> material,
                 std::shared_ptr<bee::Skeleton> skeleton)
    { 
        Mesh = mesh;
        Material = material;
        Skeleton = skeleton;
    }

};

struct Sampler
{
    Sampler(const Model& model, int index);
    Sampler();

    enum class Filter
    {
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapNearest,
        NearestMipmapLinear,
        LinearMipmapLinear
    };

    enum class Wrap
    {
        Repeat,
        ClampToEdge,
        MirroredRepeat
    };

    Filter MagFilter = Filter::Nearest;
    Filter MinFilter = Filter::Nearest;
    Wrap WrapS = Wrap::ClampToEdge;
    Wrap WrapT = Wrap::ClampToEdge;

private:
    Filter GetFilter(int filter);
    Wrap GetWrap(int wrap);
};

struct Texture
{
    Texture(const Model& model, int index);
    Texture(std::shared_ptr<Image> image, std::shared_ptr<Sampler> sampler)
        : Image(image), Sampler(sampler) {}
    std::shared_ptr<Image> Image;
    std::shared_ptr<Sampler> Sampler;
};

}  // namespace bee

VISITABLE_STRUCT(bee::Light, Color, Intensity, Range, ShadowExtent, CastShadows);
