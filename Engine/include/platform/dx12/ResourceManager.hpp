#pragma once

#include <wrl.h>
using namespace Microsoft::WRL;

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

#include <queue>
#include <vector>
#include <visit_struct/visit_struct.hpp>

#include "cereal/cereal.hpp"
#include "platform/dx12/DeviceManager.hpp"
#include "platform/dx12/image_dx12.h"
#include "tinygltf/tiny_gltf.h"
#include "tools/serializable.hpp"
#include "tools/serialization.hpp"
#include "tools/serialize_dx12.hpp"

struct Light1
{
    DirectX::XMFLOAT4 light_position_dir;  // 16
    DirectX::XMFLOAT4 light_color;         // 16
    float intensity = 1.0f;                // 4
    int type = 0;                          // 4    //0 for directional, 1 for point
    int has_shadows = 0;
    int dummy;
};

struct Vertex
{
    Vertex(float x, float y, float z)
        : pos(x, y, z),
          texCoord(0, 0),
          normal(0, 0, 0),
          tangent(0, 0, 0),
          bitangent(0, 0, 0),
          jointids(0, 0, 0, 0),
          weights(0, 0, 0, 0)
    {
    }
    Vertex(float x, float y, float z, float u, float v)
        : pos(x, y, z),
          texCoord(u, v),
          normal(0, 0, 0),
          tangent(0, 0, 0),
          bitangent(0, 0, 0),
          jointids(0, 0, 0, 0),
          weights(0, 0, 0, 0)
    {
    }

    Vertex(float x, float y, float z, float u, float v, float x_n, float y_n, float z_n, float x_t, float y_t, float z_t,
           float x_b, float y_b, float z_b)
        : pos(x, y, z),
          texCoord(u, v),
          normal(x_n, y_n, z_n),
          tangent(x_t, y_t, z_t),
          bitangent(x_b, y_b, z_b),
          jointids(0, 0, 0, 0),
          weights(0, 0, 0, 0)
    {
    }

    Vertex(float x, float y, float z, float u, float v, float x_n, float y_n, float z_n, float x_t, float y_t, float z_t,
           float x_b, float y_b, float z_b, int x_j, int y_j, int z_j, int w_j, float x_w, float y_w, float z_w, float w_w)
        : pos(x, y, z),
          texCoord(u, v),
          normal(x_n, y_n, z_n),
          tangent(x_t, y_t, z_t),
          bitangent(x_b, y_b, z_b),
          jointids(x_j, y_j, z_j, w_j),
          weights(x_w, y_w, z_w, w_w)
    {
    }
    Vertex()
        : pos(0, 0, 0),
          texCoord(0, 0),
          normal(0, 0, 0),
          tangent(0, 0, 0),
          bitangent(0, 0, 0),
          jointids(0, 0, 0, 0),
          weights(0, 0, 0, 0)
    {
    }
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 texCoord;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT3 bitangent;
    DirectX::XMUINT4 jointids;
    DirectX::XMFLOAT4 weights;
};

struct ConstantBufferPerObject
{
    DirectX::XMFLOAT4X4 wvpMat;
    DirectX::XMFLOAT4X4 wMat;
    //  DirectX::XMFLOAT4 camera_position;
};

struct ConstantBufferCamera
{
    DirectX::XMFLOAT4X4 viewMat;
    DirectX::XMFLOAT4X4 projMat;
    int ray_tracing_enabled = 1;
    int light_count;
    int unlit_scene;
    float dummy;
    DirectX::XMFLOAT3 ambient_light;
    //   DirectX::XMFLOAT4 camera_position;
};

struct ConstantBufferImageEffects
{
    DirectX::XMFLOAT4X4 viewMat;
    DirectX::XMFLOAT4X4 projMat;
    DirectX::XMFLOAT4 hsvc = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // hue, saturation, value, contrast

    DirectX::XMFLOAT2 resolution;
    int frameCount;
    float grainAmount = 0.1f;

    DirectX::XMFLOAT4 vignetteColor;

    DirectX::XMFLOAT2 vignetteSettings = DirectX::XMFLOAT2(25.0f, 0.5f);  // x = strength, y = radius
    float time;
    float fogOffset = 15.0f;

    float airDensity = 0.1f;
    DirectX::XMFLOAT3 fogColor = DirectX::XMFLOAT3(0.8, 0.9, 1.0);

    DirectX::XMFLOAT3 sunDirection;

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(hsvc), CEREAL_NVP(vignetteColor), CEREAL_NVP(vignetteSettings), CEREAL_NVP(grainAmount),
                CEREAL_NVP(fogOffset), CEREAL_NVP(airDensity), CEREAL_NVP(fogColor));
    }
};

struct ConstantBufferBloom
{
    int bloom_sigma = 16;
    int kernel_size = 32;
    float bloom_multiplier;
    float bloom_threshold;

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(bloom_sigma), CEREAL_NVP(kernel_size), CEREAL_NVP(bloom_multiplier), CEREAL_NVP(bloom_threshold));
    }
};

struct ConstantBufferMaterial
{
    DirectX::XMFLOAT4 baseColorFactor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // 16

    DirectX::XMFLOAT4 emissiveFactor = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);  // 16

    float normalTextureScale = 0.0f;        // 4
    float occlusionTextureStrength = 0.0f;  // 4
    float metallicFactor = 0.0f;            // 4
    float roughnessFactor = 1.0f;           // 4

    int BaseTexture = -1;       // 4
    int EmissiveTexture = -1;   // 4
    int NormalTexture = -1;     // 4
    int OcclusionTexture = -1;  // 4

    int MetallicRoughnessTexture = -1;  // 4
    bool IsUnlit = false;
    int padding1 = -1;  // 4
    int padding2 = -1;  // 4
};
struct MeshData
{
    int instance_num = 0;         // 4
    int instance_joints_num = 0;  // 4
    int joints_num = 0;           // 4
};

namespace bee
{
class Mesh;
struct MeshRenderer;
struct Transform;
}  // namespace bee
class ResourceManager
{
public:
    friend class bee::RenderPipeline;

    void CreateCommittedResource(ComPtr<ID3D12Resource>& buffer, LPCWSTR name, CD3DX12_HEAP_PROPERTIES heap_props,
                                 CD3DX12_RESOURCE_DESC res_desc, D3D12_HEAP_FLAGS flags, D3D12_RESOURCE_STATES state,
                                 const D3D12_CLEAR_VALUE* clear_value);

    unsigned int UpdateTextureDescriptorHeap(bee::Image* image);

    unsigned int UpdateImGuiDescriptorHeap(bee::Image* image);
    unsigned int UpdateImGuiDescriptorHeap(bee::Image* image, int index);

    AccelerationStructureBuffers CreateBottomLevelAS(
        std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
        std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers = {});

    unsigned int m_mesh_count = 0;

    std::queue<ComPtr<ID3D12Resource>> m_deletion_queue;
    size_t ID;
    ComPtr<ID3D12DescriptorHeap> m_TexturesDescriptorHeap;

    ComPtr<ID3D12DescriptorHeap> m_mipMapHeap;

    void QueueImageLoading(bee::Image* image);
    std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>> drawables;
    std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>> particleDrawables;
    std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>> foliageDrawables;
private:
    ResourceManager(DeviceManager* device_manager);

    ~ResourceManager();

    void CleanUp();

    void FinishResourceInit();

    ConstantBufferCamera m_cameraCB;

    void BeginFrame(DirectX::XMFLOAT4X4 view, DirectX::XMFLOAT4X4 projection);

    DeviceManager* m_device_manager = nullptr;

    //   int ConstantBufferPerMaterialtAlignedSize = (sizeof(ConstantBufferMaterial) + 255) & ~255;

    int ConstantBufferPerMeshAlignedSize = (sizeof(MeshData) + 255) & ~255;

    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeaps[DeviceManager::m_NumFrames];

    UINT8* m_CbvGPUAddress[DeviceManager::m_NumFrames];

    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeapsCamera[DeviceManager::m_NumFrames];

    UINT8* m_CbvGPUAddressCamera[DeviceManager::m_NumFrames];

    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeapsJoints[DeviceManager::m_NumFrames];

    UINT8* m_CbvGPUAddressJoints[DeviceManager::m_NumFrames];

    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeapsForLights[DeviceManager::m_NumFrames];

    UINT8* m_CbvGPUAddressForLights[DeviceManager::m_NumFrames];

    Light1 m_light_cb;

    ConstantBufferMaterial m_Cb_Materials;

    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeapsMaterials[DeviceManager::m_NumFrames];

    UINT8* m_CbvGPUAddressMaterials[DeviceManager::m_NumFrames];

    MeshData m_cb_Mesh_Data;

    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeapsPerMesh[DeviceManager::m_NumFrames];

    UINT8* m_CbvGPUAddressPerMesh[DeviceManager::m_NumFrames];

    ConstantBufferImageEffects m_image_effects_cb;
    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeapsImageEffects[DeviceManager::m_NumFrames];

    UINT8* m_CbvGPUAddressImageEffects[DeviceManager::m_NumFrames];

    ConstantBufferBloom m_bloom_cb;
    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeapsBloom[DeviceManager::m_NumFrames];
    UINT8* m_CbvGPUAddressBloom[DeviceManager::m_NumFrames];

    unsigned int m_texture_descriptor_count = 0;
    unsigned int m_tex_descriptor_increment = 256;

    std::vector<ComPtr<ID3D12Resource>> m_perInstanceConstantBuffers;
    std::vector<ComPtr<ID3D12Resource>> m_wolrMatrixConstantBuffers;

    ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;

    void CreateShaderResourceHeap();
    void CreateShaderBindingTable();

    void CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances,
                          AccelerationStructureBuffers& TLAS, nv_helpers_dx12::TopLevelASGenerator& generator, bool update);

    void CreateAccelerationStructures();

    std::vector<unsigned int> m_instance_counter;

    bee::Mesh* m_interMesh;

    bee::Mesh* m_particleMesh;

    bool b_unlit = false;

    int m_frameCounter = 0;
    int m_sortCounter = 0;

    std::vector<Light1> m_lights;
    int m_light_count;

    void InstanceCounterUpdate(std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>>& vector,
                               DirectX::XMMATRIX& viewMat, DirectX::XMMATRIX& projMat);
    std::vector<unsigned int> m_instance_counter_local;
    std::vector<unsigned int> m_mesh_joints_local;

    void LoadInMemory(std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>>& vector);

    inline bool isPointInFrustum(DirectX::XMVECTOR& posVector, const DirectX::XMMATRIX& viewProj)
    {
        // Transform the point
        posVector = DirectX::XMVector3TransformCoord(posVector, viewProj);

        // Extract the transformed coordinates directly into local variables
        const float x = DirectX::XMVectorGetX(posVector);
        const float y = DirectX::XMVectorGetY(posVector);
        const float z = DirectX::XMVectorGetZ(posVector);

        // Perform the frustum check
        return (std::abs(x) <= 1.0f) && (std::abs(y) <= 1.0f) && (z >= 0.0f) && (z <= 1.0f);
    }

   

    std::vector<bee::Image*> m_queuedImages;
};