#include "platform/dx12/ResourceManager.hpp"

#include <iostream>

#include "..\..\external\tinygltf\stb_image.h"
#include "platform/dx12/mesh_dx12.h"
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "rendering/model.hpp"
#include "rendering/render_components.hpp"
#include <platform/dx12/skeleton.hpp>
#include "particle_system/particle_system.hpp"
#include "core/input.hpp"
#include "light_system/light_system.hpp"
#include <execution>
//#include "Dx12NiceRenderer\Renderer\include\image_dx12.h"
#include <glm/gtx/matrix_decompose.hpp>

#include "level_editor/level_editor_components.hpp"
#include "rendering/debug_render.hpp"


ResourceManager::ResourceManager(DeviceManager* device_manager)
{
    m_device_manager = device_manager;
    
    {
        uint32_t nbMatrix = 4;  // view, perspective, viewInv, perspectiveInv
        m_device_manager->m_CameraBufferSize = nbMatrix * sizeof(DirectX::XMMATRIX);

        // Create the constant buffer for all matrices


        nv_helpers_dx12::CreateBuffer(m_device_manager->m_CameraBuffer,
                m_device_manager->GetDevice().Get(), m_device_manager->m_CameraBufferSize, D3D12_RESOURCE_FLAG_NONE,
                D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

        // Create a descriptor heap that will be used by the rasterization shaders

        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = 1;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            ThrowIfFailed(
                m_device_manager->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_device_manager->m_ConstHeap)));
        }

        // Describe and create the constant buffer view.
         /*  D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
           cbvDesc.BufferLocation = m_device_manager->m_CameraBuffer->GetGPUVirtualAddress();
           cbvDesc.SizeInBytes = m_device_manager->m_CameraBufferSize;*/

        // Get a handle to the heap memory on the CPU side, to be able to write the
        // descriptors directly
       /* D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_device_manager->m_ConstHeap->GetCPUDescriptorHandleForHeapStart();
        m_device_manager->GetDevice()->CreateConstantBufferView(&cbvDesc, srvHandle);*/
    }

    for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    {
        CreateCommittedResource(m_ConstantBufferUploadHeapsMaterials[i], L"Constant Buffer Materials Upload Resource Heap",
                                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
                                D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(m_ConstantBufferUploadHeapsMaterials[i]->Map(0, &readRange,
                                                                   reinterpret_cast<void**>(&m_CbvGPUAddressMaterials[i])));

    }

     for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    {
         CreateCommittedResource(m_ConstantBufferUploadHeapsPerMesh[i], L"Constant Buffer Per Mesh Upload Resource Heap",
                                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
                                D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(
            m_ConstantBufferUploadHeapsPerMesh[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_CbvGPUAddressPerMesh[i])));
    }
    


    for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    {
        CreateCommittedResource(m_ConstantBufferUploadHeaps[i], L"Constant Buffer Upload Resource Heap",
                                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(256*1024 * 64),
                                D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(m_ConstantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_CbvGPUAddress[i])));

        
    }

    for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    {
        CreateCommittedResource(m_ConstantBufferUploadHeapsCamera[i], L"Constant Buffer Upload Resource Heap",
                                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer( 1024 * 64),
                                D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(
            m_ConstantBufferUploadHeapsCamera[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_CbvGPUAddressCamera[i])));

    }


    for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    {
        CreateCommittedResource(m_ConstantBufferUploadHeapsImageEffects[i], L"Constant Buffer Upload Resource Heap Image Effects",
                                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer( 1024 * 64),
                                D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(
            m_ConstantBufferUploadHeapsImageEffects[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_CbvGPUAddressImageEffects[i])));

    }
    for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    {
        CreateCommittedResource(m_ConstantBufferUploadHeapsBloom[i],
                                L"Constant Buffer Upload Resource Heap Bloom",
                                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
                                D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(m_ConstantBufferUploadHeapsBloom[i]->Map(
            0, &readRange, reinterpret_cast<void**>(&m_CbvGPUAddressBloom[i])));
    }


    
    for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    {
        CreateCommittedResource(m_ConstantBufferUploadHeapsJoints[i], L"Constant Buffer Upload Resource Heap",
                                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(32*1024 * 64),
                                D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(
            m_ConstantBufferUploadHeapsJoints[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_CbvGPUAddressJoints[i])));

      
    }

    for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    {
        CreateCommittedResource(m_ConstantBufferUploadHeapsForLights[i], L" Lights Constant Buffer Upload Resource Heap",
                                CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer( 1024 * 64),
                                D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        // Mapping and updating data
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(m_ConstantBufferUploadHeapsForLights[i]->Map(0, &readRange,
                                                                   reinterpret_cast<void**>(&m_CbvGPUAddressForLights[i])));

    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;  // One for SRV, one for UAV
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

   
    ThrowIfFailed(m_device_manager->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_mipMapHeap)));


    //for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
    //{
    //    CreateCommittedResource(m_ConstantBufferUploadHeapsDebug[i], L"Constant Buffer Debug Resource Heap",
    //                            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
    //                            D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

    //    CD3DX12_RANGE readRange(0, 0);

    //    ThrowIfFailed(
    //        m_ConstantBufferUploadHeapsDebug[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_CbvGPUAddressDebug[i])));

    //    ZeroMemory(&m_Cb_Debug_View, sizeof(m_Cb_Debug_View));

    //    memcpy(m_CbvGPUAddressDebug[i], &m_Cb_Debug_View, sizeof(m_Cb_Debug_View));
    //}

 /*   Light1 temp_light;

    temp_light.type = 0;

    temp_light.light_position_dir.x = 1;
    temp_light.light_position_dir.y = -1;
    temp_light.light_position_dir.z = -1;

    temp_light.light_color.x = 1;
    temp_light.light_color.y = 1;
    temp_light.light_color.z = 1;
    temp_light.light_color.w = 1;
    temp_light.intensity = 5;

    m_lights.push_back(temp_light);

    temp_light.type = 0;

    temp_light.light_position_dir.x = -1;
    temp_light.light_position_dir.y = 1;
    temp_light.light_position_dir.z = 1;

    temp_light.light_color.x = 1;

    temp_light.light_color.y = 1;

    temp_light.light_color.z = 1;

    temp_light.light_color.w = 1;

    temp_light.intensity = 1;

    m_lights.push_back(temp_light);*/

   


}


void ResourceManager::InstanceCounterUpdate(std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>>& vector,DirectX::XMMATRIX& viewMat,DirectX::XMMATRIX& projMat)
{
    DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(viewMat, projMat);

    for (auto& [e, renderer, transform] : vector)
   {
       if (renderer.Mesh->m_mesh_index < m_instance_counter_local.size())
       {
           m_instance_counter_local[renderer.Mesh->m_mesh_index] = m_instance_counter_local[renderer.Mesh->m_mesh_index] + 1;

           if (renderer.Skeleton) m_mesh_joints_local[renderer.Mesh->m_mesh_index] = renderer.Skeleton->m_Joints.size();
       }
       else
       {
           for (int i = 0; i <= renderer.Mesh->m_mesh_index; i++)
           {
               m_instance_counter.push_back(0);
               m_instance_counter_local.push_back(0);

               //  if (renderer.Skeleton)
               m_mesh_joints_local.push_back(0);
           }
           m_instance_counter_local[renderer.Mesh->m_mesh_index] = m_instance_counter_local[renderer.Mesh->m_mesh_index] + 1;

           if (renderer.Skeleton) m_mesh_joints_local[renderer.Mesh->m_mesh_index] = renderer.Skeleton->m_Joints.size();
       }
       int i = renderer.Mesh->m_mesh_index;

       // if(!bee::Engine.ECS().Registry.valid(e))continue;
       DirectX::XMMATRIX wvpMat;
       DirectX::XMMATRIX worldMat =  ConvertGLMToDXMatrix(transform.WorldMatrix);

       if (bee::Engine.ECS().Registry.all_of<bee::ParticleComponent>(e) &&
           bee::Engine.ECS().Registry.get<bee::ParticleComponent>(e).type == bee::ParticleType::Billboard)
       {
           


           DirectX::XMMATRIX invViewMatrix = DirectX::XMMatrixInverse(nullptr, viewMat);
           
           DirectX::XMVECTOR originalPosition = worldMat.r[3];

           DirectX::XMVECTOR scale = DirectX::XMVectorSet(DirectX::XMVector3Length(worldMat.r[0]).m128_f32[0],  // Scale X
                                                          DirectX::XMVector3Length(worldMat.r[1]).m128_f32[0],  // Scale Y
                                                          DirectX::XMVector3Length(worldMat.r[2]).m128_f32[0],  // Scale Z
                                                          1.0f);

           DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScalingFromVector(scale);

           DirectX::XMMATRIX rotationMatrix = invViewMatrix;
           rotationMatrix.r[3] = DirectX::XMVectorSet(0, 0, 0, 1);

           DirectX::XMMATRIX billboardWorldMatrix =
               scaleMatrix * rotationMatrix * DirectX::XMMatrixTranslationFromVector(originalPosition);


           wvpMat = billboardWorldMatrix * viewProj;


       }
       else
       {
           wvpMat = worldMat * viewProj;
       }

       DirectX::XMMATRIX transposed = DirectX::XMMatrixTranspose(wvpMat);
       DirectX::XMStoreFloat4x4(&renderer.constant_data.wvpMat, transposed);
       DirectX::XMStoreFloat4x4(&renderer.constant_data.wMat, DirectX::XMMatrixTranspose(worldMat));
       if (renderer.Material)
       renderer.constant_data.materialIndex = renderer.Material->material_index;
      
        m_instance_counter[renderer.Mesh->m_mesh_index] = m_instance_counter_local[renderer.Mesh->m_mesh_index];
    }
}

void ResourceManager::LoadInMemory(std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>>& vector)
{
    for (const auto& [e, renderer, transform] : vector)
    {
        int offset = 0;
        for (int i = 0; i < renderer.Mesh->m_mesh_index; i++)
        {
            offset += m_instance_counter[i];
        }


        int j = offset + m_instance_counter_local[renderer.Mesh->m_mesh_index] - 1;

         int offsetBones = 0;
        int joints;
        if (renderer.Skeleton)
        {
            for (int i = 0; i < renderer.Mesh->m_mesh_index; i++)
            {
                offsetBones += m_mesh_joints_local[i] * m_instance_counter[i];
            }
            joints = offsetBones + m_mesh_joints_local[renderer.Mesh->m_mesh_index] *
                                  (m_instance_counter_local[renderer.Mesh->m_mesh_index] - 1);

        }
        
        m_instance_counter_local[renderer.Mesh->m_mesh_index]--;
        
        memcpy(m_CbvGPUAddress[m_device_manager->GetCurrentBufferIndex()] + j * sizeof(renderer.constant_data),
               &renderer.constant_data, sizeof(renderer.constant_data));


        if (renderer.Material)
        {
            m_Cb_Materials.baseColorFactor = ConvertGLMToDXFLOAT4(renderer.Material->BaseColorFactor);
            m_Cb_Materials.emissiveFactor = ConvertGLMToDXFLOAT4(renderer.Material->EmissiveFactor, 1);
            m_Cb_Materials.normalTextureScale = renderer.Material->NormalTextureScale;
            m_Cb_Materials.occlusionTextureStrength = renderer.Material->OcclusionTextureStrength;
            m_Cb_Materials.metallicFactor = renderer.Material->MetallicFactor;
            m_Cb_Materials.roughnessFactor = renderer.Material->RoughnessFactor;

            m_Cb_Materials.BaseTexture = -1;
            if (renderer.Material->UseBaseTexture)
                m_Cb_Materials.BaseTexture = renderer.Material->BaseColorTexture->Image->GetTextureId();

            m_Cb_Materials.EmissiveTexture = -1;
            if (renderer.Material->UseEmissiveTexture)
                m_Cb_Materials.EmissiveTexture = renderer.Material->EmissiveTexture->Image->GetTextureId();

            m_Cb_Materials.NormalTexture = -1;
            if (renderer.Material->UseNormalTexture)
                m_Cb_Materials.NormalTexture = renderer.Material->NormalTexture->Image->GetTextureId();

          /*  m_Cb_Materials.OcclusionTexture = -1;
            if (renderer.Material->UseOcclusionTexture)
                m_Cb_Materials.OcclusionTexture = renderer.Material->OcclusionTexture->Image->GetTextureId();*/

            m_Cb_Materials.MetallicRoughnessTexture = -1;
            if (renderer.Material->UseMetallicRoughnessTexture)
                m_Cb_Materials.MetallicRoughnessTexture = renderer.Material->MetallicRoughnessTexture->Image->GetTextureId();

           /* m_Cb_Materials.instance_num = offset;
            if (renderer.Skeleton)
            {
                m_Cb_Materials.joints_num = renderer.Skeleton->m_Joints.size();

                m_Cb_Materials.instance_joints_num = offsetBones;
            }*/
            m_Cb_Materials.IsUnlit = renderer.Material->IsUnlit;
        }

        int i = renderer.Mesh->m_mesh_index;

         int m = renderer.Material->material_index;


        memcpy(m_CbvGPUAddressMaterials[m_device_manager->GetCurrentBufferIndex()] + m * sizeof(m_Cb_Materials),
               &m_Cb_Materials, sizeof(m_Cb_Materials));


        m_cb_Mesh_Data.instance_num = offset;

        if (renderer.Skeleton)
        {
            m_cb_Mesh_Data.joints_num = renderer.Skeleton->m_Joints.size();

            m_cb_Mesh_Data.instance_joints_num = offsetBones;
        }


        memcpy(m_CbvGPUAddressPerMesh[m_device_manager->GetCurrentBufferIndex()] + i * ConstantBufferPerMeshAlignedSize,
               &m_cb_Mesh_Data, sizeof(m_cb_Mesh_Data));



        if (renderer.Skeleton)
        {
          

            for (int joint = 0; joint < renderer.Skeleton->m_Joints.size(); joint++)
            {
                
                memcpy(m_CbvGPUAddressJoints[m_device_manager->GetCurrentBufferIndex()] + joint * sizeof(DirectX::XMFLOAT4X4) +
                           joints * sizeof(DirectX::XMFLOAT4X4),
                       &renderer.Skeleton->m_ComputedJointMatrices[joint], sizeof(DirectX::XMFLOAT4X4));
            }
           
        }
    }


}

void ResourceManager::BeginFrame(DirectX::XMFLOAT4X4 view, DirectX::XMFLOAT4X4 projection)
{

   /* for (int i = 0; i < m_queuedImages.size(); i++)
    {
        m_queuedImages[i]->WaitForLoad();
    }
    for (int i = 0; i < m_queuedImages.size(); i++)
    {
        m_queuedImages[i]->UploadGPU();
    }
    m_queuedImages.clear();*/

    DirectX::XMMATRIX viewMat = DirectX::XMLoadFloat4x4(&view);
    DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4(&projection);
    DirectX::XMMATRIX invViewMatrix = DirectX::XMMatrixInverse(nullptr, viewMat);

    //lets update here all the transform matrix;
    const auto& transformView = bee::Engine.ECS().Registry.view<bee::Transform>();
    for (const auto& [e, transform] : transformView.each())
    {
        const glm::mat4 world = transform.World();
        transform.WorldMatrix = world;
        transform.TranslationWorld = glm::vec3(world[3][0], world[3][1], world[3][2]);
    }

    m_lights.clear();
    drawables.clear();
    particleDrawables.clear();
    foliageDrawables.clear();

    DirectX::XMFLOAT4 lightData = DirectX::XMFLOAT4(0,0,0,0);

    int light_count = 0;
   for (const auto& [e, transform, light] : bee::Engine.ECS().Registry.view<bee::Transform, bee::Light>().each())
   {
      
       m_light_cb.intensity = light.Intensity;
       m_light_cb.light_color.x = light.Color.x/255.0f;
       m_light_cb.light_color.y = light.Color.y / 255.0f;
       m_light_cb.light_color.z = light.Color.z / 255.0f;
       
          DirectX::XMMATRIX worldMat;

           worldMat = ConvertGLMToDXMatrix(transform.WorldMatrix);

        DirectX::XMVECTOR lightPosition = worldMat.r[3];

        m_light_cb.has_shadows = light.CastShadows;  // light.CastShadows;
       
     

         DirectX::XMStoreFloat4(&m_light_cb.light_position_dir, lightPosition);


         if (light.CastShadows)
         {
             DirectX::XMStoreFloat3(&m_image_effects_cb.sunDirection, lightPosition);

              DirectX::XMStoreFloat4(&lightData, lightPosition);
             lightData.w = light.ShadowExtent;
         }


         m_light_cb.light_position_dir.w = light.ShadowExtent;



       m_light_cb.type = 1;


       m_lights.push_back(m_light_cb);
     
      /* memcpy(m_CbvGPUAddressForLights[m_device_manager->GetCurrentBufferIndex()] + light_count * sizeof(Light1), &m_light_cb,
              sizeof(Light1));*/
       light_count++;
   }

 DirectX::XMVECTOR det;

    std::vector<DirectX::XMMATRIX> matrices(4);
  //  matrices[0] = DirectX::XMMatrixIdentity() * light_count;  // XMMatrixInverse(&det, viewMat);


    DirectX::XMFLOAT4X4 tempMatrix;
  //  DirectX::XMStoreFloat4x4(&tempMatrix, matrices[0]);

    // Set the specific element
    tempMatrix.m[0][0] = light_count;
    
     tempMatrix.m[0][0] = light_count;
    tempMatrix.m[0][1] = lightData.x;
     tempMatrix.m[0][2] = lightData.y;
    tempMatrix.m[0][3] = lightData.z;
    tempMatrix.m[1][0] = lightData.w;

    // Convert back to XMMATRIX
    matrices[0] = DirectX::XMLoadFloat4x4(&tempMatrix);
    


   matrices[1] = DirectX::XMMatrixIdentity() * m_frameCounter;  // XMMatrixInverse(&det, projMat);
                              //   matrices[2] = DirectX::XMMatrixIdentity() * light_count;

   uint8_t* pData;
   ThrowIfFailed(m_device_manager->m_CameraBuffer->Map(0, nullptr, (void**)&pData));
   memcpy(pData, matrices.data(), m_device_manager->m_CameraBufferSize);
   m_device_manager->m_CameraBuffer->Unmap(0, nullptr);


   if (light_count != m_light_count)
   for (int i = 0; i < m_device_manager->m_NumFrames; ++i)
   {
       memset(m_CbvGPUAddressForLights[i], 0, m_light_count * sizeof(Light1));
   }


    const UINT bufferSize = m_lights.size() * sizeof(Light1);

     memcpy(m_CbvGPUAddressForLights[m_device_manager->GetCurrentBufferIndex()], m_lights.data(), bufferSize);

     m_light_count = light_count;
    

    const auto drawablesView = bee::Engine.ECS().Registry.view<bee::MeshRenderer, bee::Transform>();

    // Assuming originalProjectionMatrix is your previously calculated matrix
    DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(0.55f, 0.55f, 1.0f);  // Scale x and y by 0.5
    DirectX::XMMATRIX modifiedProjectionMatrix = scaleMatrix * projMat;

    DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(viewMat, modifiedProjectionMatrix);
    for(const auto& [e, renderer, transform]: drawablesView.each())
    {
        if(transform.Name == "Terrain Ground")
        {
            //never cull the terrain, what is wrong with you, why would you want this
           // drawables.push_back({e, renderer, transform});
            drawables.insert(drawables.begin(), {e, renderer, transform});
            continue;
        }

        const glm::mat4 WorldMatrix = transform.WorldMatrix;
        const DirectX::XMFLOAT3 position = {WorldMatrix[3][0], WorldMatrix[3][1], WorldMatrix[3][2]};
        DirectX::XMVECTOR posVector = DirectX::XMLoadFloat3(&position);

        if (isPointInFrustum(posVector, viewProj)) 
        {
            const bee::Entity& parent = transform.GetParent();
            if (bee::Engine.ECS().Registry.all_of<bee::ParticleComponent>(parent)) 
            {
                particleDrawables.push_back({parent, renderer, transform});
            } else if(bee::Engine.ECS().Registry.all_of<lvle::FoliageComponent>(parent))
            {
                foliageDrawables.push_back({parent, renderer, transform});
            }else
            {
                drawables.push_back({e, renderer, transform});
            }
        }
    }

    
   
    DirectX::XMVECTOR cameraPos = invViewMatrix.r[3];
    glm::vec3 cPos = {cameraPos.m128_f32[0], cameraPos.m128_f32[1], cameraPos.m128_f32[2]};

    
    // Sort by distance to the camera in descending order
    std::sort(std::execution::par, particleDrawables.begin(), particleDrawables.end(),
     [&cPos](const auto& a, const auto& b) {
         const auto& transformA = std::get<2>(a);
         const auto& transformB = std::get<2>(b);
         const float distA = glm::distance2(cPos, transformA.TranslationWorld);
         const float distB = glm::distance2(cPos, transformB.TranslationWorld);
         return distA < distB;
     });
    
    m_instance_counter.clear();
    m_instance_counter_local.clear();
    m_mesh_joints_local.clear();
    InstanceCounterUpdate(drawables, viewMat, projMat);
    InstanceCounterUpdate(particleDrawables, viewMat, projMat);
    InstanceCounterUpdate(foliageDrawables,viewMat,projMat);
    LoadInMemory(drawables);
    LoadInMemory(particleDrawables);
    LoadInMemory(foliageDrawables);
    
  //  bee::Engine.ECS().GetSystem<>()
    const auto terrainStuff = bee::Engine.ECS().Registry.view<lvle::TerrainDataComponent>();

    for (const auto& [e,terrain] : terrainStuff.each())
    {
        for (int m = 0; m < terrain.m_materials.size(); m++)
        {
            
           
                    m_Cb_Materials.baseColorFactor = ConvertGLMToDXFLOAT4(terrain.m_materials[m]->BaseColorFactor);
            m_Cb_Materials.emissiveFactor = ConvertGLMToDXFLOAT4(terrain.m_materials[m]->EmissiveFactor, 1);
                    m_Cb_Materials.normalTextureScale = terrain.m_materials[m]->NormalTextureScale;
            m_Cb_Materials.occlusionTextureStrength = terrain.m_materials[m]->OcclusionTextureStrength;
                    m_Cb_Materials.metallicFactor = terrain.m_materials[m]->MetallicFactor;
            m_Cb_Materials.roughnessFactor = terrain.m_materials[m]->RoughnessFactor;

            m_Cb_Materials.BaseTexture = -1;
            if (terrain.m_materials[m]->UseBaseTexture)
                m_Cb_Materials.BaseTexture = terrain.m_materials[m]->BaseColorTexture->Image->GetTextureId();

            m_Cb_Materials.EmissiveTexture = -1;
            if (terrain.m_materials[m]->UseEmissiveTexture)
                m_Cb_Materials.EmissiveTexture = terrain.m_materials[m]->EmissiveTexture->Image->GetTextureId();

            m_Cb_Materials.NormalTexture = -1;
            if (terrain.m_materials[m]->UseNormalTexture)
                m_Cb_Materials.NormalTexture = terrain.m_materials[m]->NormalTexture->Image->GetTextureId();

            m_Cb_Materials.OcclusionTexture = -1;
          /*  if (terrain.m_materials[m]->UseOcclusionTexture)
                m_Cb_Materials.OcclusionTexture =  terrain.m_materials[m]->OcclusionTexture->Image->GetTextureId();*/

            m_Cb_Materials.MetallicRoughnessTexture = -1;
            if (terrain.m_materials[m]->UseMetallicRoughnessTexture)
                m_Cb_Materials.MetallicRoughnessTexture =
                    terrain.m_materials[m]->MetallicRoughnessTexture->Image->GetTextureId();

            m_Cb_Materials.IsUnlit = terrain.m_materials[m]->IsUnlit;

            int mat = terrain.m_materials[m]->material_index;

            memcpy(m_CbvGPUAddressMaterials[m_device_manager->GetCurrentBufferIndex()] + mat * sizeof(m_Cb_Materials),
                   &m_Cb_Materials, sizeof(m_Cb_Materials));
        }
    }


    DirectX::XMStoreFloat4x4(&m_cameraCB.viewMat, invViewMatrix);
    DirectX::XMStoreFloat4x4(&m_cameraCB.projMat, XMMatrixInverse(&det, projMat));

     


    if (m_device_manager->b_rayTracingEnabled)
    {
        m_cameraCB.ray_tracing_enabled = 1;
    }
    else
    {
        m_cameraCB.ray_tracing_enabled = 0;
    }

    m_cameraCB.light_count = light_count;
    m_cameraCB.unlit_scene = b_unlit;
    memcpy(m_CbvGPUAddressCamera[m_device_manager->GetCurrentBufferIndex()], &m_cameraCB, sizeof(m_cameraCB));

     static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    const float value = elapsed / 1000.0f;  // Convert milliseconds to seconds

    m_image_effects_cb.time = value;
    DirectX::XMStoreFloat4x4(&m_image_effects_cb.viewMat, viewMat);
    DirectX::XMStoreFloat4x4(&m_image_effects_cb.projMat,  projMat);

    m_image_effects_cb.resolution.x = m_device_manager->width;
    m_image_effects_cb.resolution.y= m_device_manager->height;
    m_image_effects_cb.frameCount  = m_frameCounter;

    memcpy(m_CbvGPUAddressImageEffects[m_device_manager->GetCurrentBufferIndex()], &m_image_effects_cb, sizeof(m_image_effects_cb));
    memcpy(m_CbvGPUAddressBloom[m_device_manager->GetCurrentBufferIndex()], &m_bloom_cb, sizeof(m_bloom_cb));
    
    m_frameCounter++;

    /* for (int i = 0; i < m_queuedImages.size(); i++)
    {
        m_queuedImages[i]->WaitForLoad();
    }
    for (int i = 0; i < m_queuedImages.size(); i++)
    {
        m_queuedImages[i]->UploadGPU();
    }
    m_queuedImages.clear();*/
    
}
void ResourceManager::QueueImageLoading(bee::Image* image)
{ m_queuedImages.push_back(image); }
void ResourceManager::CleanUp()
{
     if (!m_deletion_queue.empty()) m_device_manager->Flush();

     while (!m_deletion_queue.empty())
    {
         ComPtr<ID3D12Resource> resource = m_deletion_queue.front();
         m_deletion_queue.pop();
    
         resource.Reset();
    
      /*   if (resource.Get())
         resource->Release();*/

    }
}
ResourceManager::~ResourceManager()
{
    m_device_manager = nullptr;
    delete m_interMesh;
    m_interMesh = nullptr;
    delete m_particleMesh;
    m_particleMesh = nullptr;
    
}

unsigned int ResourceManager::UpdateTextureDescriptorHeap(bee::Image* image)
{
    /*auto md = bee::Engine.ECS().Registry.view<bee::Texture>();

    for (auto e : md)
    {

    }*/
    //m_queuedImages.push_back(image);

    if (m_TexturesDescriptorHeap)
    {
        if (m_texture_descriptor_count >= m_TexturesDescriptorHeap->GetDesc().NumDescriptors)
        {
            /*  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
               heapDesc.NumDescriptors = m_texture_descriptor_count + m_tex_descriptor_increment;
               heapDesc.Flags = true ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
               heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

               ThrowIfFailed(
                   m_device_manager->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_TempTexturesDescriptorHeap)));
              */

            m_TexturesDescriptorHeap->Release();
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = m_texture_descriptor_count + m_tex_descriptor_increment;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

            ThrowIfFailed(
                m_device_manager->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_TexturesDescriptorHeap)));
        }
    }
    else
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = m_texture_descriptor_count + m_tex_descriptor_increment;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        ThrowIfFailed(m_device_manager->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_TexturesDescriptorHeap)));
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuStartHandle(m_TexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    UINT descriptorSize =
        m_device_manager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuStartHandle.Offset(m_texture_descriptor_count, descriptorSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = image->m_textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = image->m_mip_count;

    m_device_manager->GetDevice()->CreateShaderResourceView(image->m_TextureBuffer.Get(), &srvDesc, cpuStartHandle);

    m_texture_descriptor_count++;
    return m_texture_descriptor_count - 1;
}

unsigned int ResourceManager::UpdateImGuiDescriptorHeap(bee::Image* image,int index)
{
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuStartHandle(m_device_manager->m_ImGui_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    UINT descriptorSize =
        m_device_manager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuStartHandle.Offset(1+index, descriptorSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = image->m_textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    m_device_manager->GetDevice()->CreateShaderResourceView(image->m_TextureBuffer.Get(), &srvDesc, cpuStartHandle);

   // m_texture_descriptor_count++;
    return index;
}

void ResourceManager::FinishResourceInit()
{
    const UINT bufferSize = m_lights.size() * sizeof(Light1);
    
    m_interMesh = new bee::Mesh();

    std::vector<Vertex> tempVertices;


    float width = 0.3f;  // Reduce width for thinness
    float depth = 1.5f;  // Reduce depth for thinness
    float height = 0.3f;

    tempVertices.push_back({-width, -height, -depth});
    tempVertices.push_back({width, -height, -depth});
    tempVertices.push_back({width, height, -depth});
    tempVertices.push_back({-width, height, -depth});
    tempVertices.push_back({-width, -height, depth});
    tempVertices.push_back({width, -height, depth});
    tempVertices.push_back({width, height, depth});
    tempVertices.push_back({-width, height, depth});

    std::vector<DWORD> tempIndices = {// Front face
                                      0, 1, 2, 2, 3, 0,
                                      // Back face
                                      4, 6, 5, 6, 4, 7,
                                      // Left face
                                      4, 3, 7, 4, 0, 3,
                                      // Right face
                                      1, 5, 6, 6, 2, 1,
                                      // Top face
                                      3, 2, 6, 6, 7, 3,
                                      // Bottom face
                                      4, 1, 0, 4, 5, 1};

    m_interMesh->SetAttribute(tempVertices);
    m_interMesh->SetIndices(tempIndices);
    m_interMesh->ResetBLAS();


    m_particleMesh = new bee::Mesh();


     std::vector<Vertex> tempVertices2;

     width = 1.0f;   // Width of the quad
     height = 1.0f;  // Height of the quad

     // Define vertices with position and texture coordinates
     tempVertices2.push_back({-width / 2, -height / 2, 0.0f, 0.0f, 0.0f});  // Bottom-left
     tempVertices2.push_back({width / 2, -height / 2, 0.0f, 1.0f, 0.0f});   // Bottom-right
     tempVertices2.push_back({width / 2, height / 2, 0.0f, 1.0f, 1.0f});    // Top-right
     tempVertices2.push_back({-width / 2, height / 2, 0.0f, 0.0f, 1.0f});   // Top-left

     // Define indices for the quad (two triangles)
     std::vector<DWORD> tempIndices2 = {
         0, 1, 2,  // First triangle
         2, 3, 0   // Second triangle
     };

    m_particleMesh->SetAttribute(tempVertices2);
     m_particleMesh->SetIndices(tempIndices2);
 




    if (m_device_manager->b_rayTracingEnabled)
     {
         CreateAccelerationStructures();
     }
    CreateShaderResourceHeap();

    if (m_device_manager->b_rayTracingEnabled)
    {
    CreateShaderBindingTable();
        
    }

    for (int i = 0; i < m_queuedImages.size(); i++)
    {
         m_queuedImages[i]->WaitForLoad();
    
    }
    for (int i = 0; i < m_queuedImages.size(); i++)
    {
      
        m_queuedImages[i]->UploadGPU();
    }
    m_queuedImages.clear();

}



void ResourceManager::CreateCommittedResource(ComPtr<ID3D12Resource>& buffer, LPCWSTR name, CD3DX12_HEAP_PROPERTIES heap_props,
                                              CD3DX12_RESOURCE_DESC res_desc, D3D12_HEAP_FLAGS flags,
                                              D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clear_value)
{
    // std::cout << name<<std::endl;m_mesh_index

    ThrowIfFailed(m_device_manager->GetDevice()->CreateCommittedResource(&heap_props, flags, &res_desc, state, clear_value,
                                                                         IID_PPV_ARGS(&buffer)));
    buffer->SetName(name);
}



void ResourceManager::CreateShaderResourceHeap()
{
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = 9;  //+ m_textureBuffers.size();
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.Flags = true ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(m_device_manager->m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvUavHeap)));

        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            m_device_manager->m_Device->CreateUnorderedAccessView(m_device_manager->m_outputResource.Get(), nullptr, &uavDesc,
                                                                  srvHandle);

            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            m_device_manager->m_Device->CreateUnorderedAccessView(m_device_manager->m_posBuffer.Get(), nullptr, &uavDesc,
                                                                  srvHandle);

            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
       {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            m_device_manager->m_Device->CreateUnorderedAccessView(m_device_manager->m_normalBuffer.Get(), nullptr, &uavDesc,
                                                                  srvHandle);

            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            m_device_manager->m_Device->CreateUnorderedAccessView(m_device_manager->m_colorBuffer.Get(), nullptr, &uavDesc,
                                                                  srvHandle);

            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            m_device_manager->m_Device->CreateUnorderedAccessView(m_device_manager->m_materialBuffer.Get(), nullptr, &uavDesc,
                                                                  srvHandle);

            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            m_device_manager->m_Device->CreateUnorderedAccessView(m_device_manager->m_shadowBuffer.Get(), nullptr, &uavDesc,
                                                                  srvHandle);

            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

         {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            m_device_manager->m_Device->CreateUnorderedAccessView(m_device_manager->m_bloomBuffer.Get(), nullptr, &uavDesc,
                                                                  srvHandle);

            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        if (m_device_manager->b_rayTracingEnabled)
        {
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.RaytracingAccelerationStructure.Location =
                    m_device_manager->m_topLevelASBuffers1.pResult->GetGPUVirtualAddress();

                m_device_manager->m_Device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
            }

            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = m_device_manager->m_CameraBuffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = m_device_manager->m_CameraBufferSize;
            m_device_manager->m_Device->CreateConstantBufferView(&cbvDesc, srvHandle);

               srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);



        }
       /* for (int t = 0; t < m_textureBuffers.size(); t++)
        {
            srvHandle.ptr +=
                m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1;
            srvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc1.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc1.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc1.Texture2D.MipLevels = 1;
            srvDesc1.Texture2D.PlaneSlice = 0;
            srvDesc1.Texture2D.MostDetailedMip = 0;

            m_device_manager->GetDevice()->CreateShaderResourceView(m_textureBuffers[t].m_TextureBuffer.Get(), &srvDesc1,
                                                                    srvHandle);
        }*/
    }
}

void ResourceManager::CreateShaderBindingTable()
{
    m_device_manager->m_sbtHelper.Reset();

    D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle = m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();

    auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

    m_device_manager->m_sbtHelper.AddRayGenerationProgram(L"RayGen", {heapPointer});

    // m_device_manager->m_sbtHelper.AddMissProgram(L"Miss", {});

    m_device_manager->m_sbtHelper.AddMissProgram(L"ShadowMiss", {});

  //  m_device_manager->m_sbtHelper.AddMissProgram(L"ReflectionMiss", {});

    m_device_manager->m_sbtHelper.AddMissProgram(L"AOMiss", {});

  
     
     /*std::vector<std::tuple<bee::Entity, bee::MeshRenderer, bee::Transform>> drawables;
    for (const auto& [e, renderer, transform] : bee::Engine.ECS().Registry.view<bee::MeshRenderer, bee::Transform>().each())
        drawables.push_back({e, renderer, transform});*/

    for (auto& [e, renderer, transform] : drawables)
    {
        if (!renderer.Material->ReceiveShadows) continue;

        auto VheapPointer = reinterpret_cast<UINT64*>(renderer.Mesh->m_vertexBuffer->GetGPUVirtualAddress());

        auto IheapPointer = reinterpret_cast<UINT64*>(renderer.Mesh->m_indexBuffer->GetGPUVirtualAddress());

        auto CheapPointer = reinterpret_cast<UINT64*>(m_ConstantBufferUploadHeaps[0]->GetGPUVirtualAddress());
        
         /* m_device_manager->m_sbtHelper.AddHitGroup(L"ReflectionHitGroup",
                                                  {VheapPointer, IheapPointer, heapPointer, CheapPointer,
                                    (void*)(m_ConstantBufferUploadHeaps[0]->GetGPUVirtualAddress())});*/

        m_device_manager->m_sbtHelper.AddHitGroup(L"AOHitGroup", {});
        
    }



    uint32_t sbtSize = m_device_manager->m_sbtHelper.ComputeSBTSize();

   
        nv_helpers_dx12::CreateBuffer(m_device_manager->m_sbtStorage,m_device_manager->m_Device.Get(), sbtSize,
                                  D3D12_RESOURCE_FLAG_NONE,
                                      D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
    if (!m_device_manager->m_sbtStorage)
    {
        throw std::logic_error("Could not allocate the shader binding table");
    }

    m_device_manager->m_sbtHelper.Generate(m_device_manager->m_sbtStorage.Get(), m_device_manager->m_rtStateObjectProps.Get());
}

AccelerationStructureBuffers ResourceManager::CreateBottomLevelAS(
    std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
    std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers)
{
    nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

    for (size_t i = 0; i < vVertexBuffers.size(); i++)
    {
        if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
            bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second, sizeof(Vertex),
                                          vIndexBuffers[i].first.Get(), 0, vIndexBuffers[i].second, nullptr, 0, true);

        else
            bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0, vVertexBuffers[i].second, sizeof(Vertex), 0, 0);
    }
   
    UINT64 scratchSizeInBytes = 0;

    UINT64 resultSizeInBytes = 0;

    bottomLevelAS.ComputeASBufferSizes(m_device_manager->m_Device.Get(), false, &scratchSizeInBytes, &resultSizeInBytes);

    AccelerationStructureBuffers buffers;
    nv_helpers_dx12::CreateBuffer(buffers.pScratch,m_device_manager->m_Device.Get(), scratchSizeInBytes,
                                                     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
                                                     nv_helpers_dx12::kDefaultHeapProps);

   nv_helpers_dx12::CreateBuffer(buffers.pResult,
        m_device_manager->m_Device.Get(), resultSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);

    bottomLevelAS.Generate(m_device_manager->m_CommandList.Get(), buffers.pScratch.Get(), buffers.pResult.Get(), false,
                           nullptr);

   

    return buffers;
}

void ResourceManager::CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances,
                                       AccelerationStructureBuffers& TLAS, nv_helpers_dx12::TopLevelASGenerator& generator,
                                       bool update)
{
   
    if (!update)
    {
       // m_device_manager->m_topLevelASGenerator.
        generator.ClearInstances();
        for (size_t i = 0; i < instances.size(); i++)
        {
            generator.AddInstance(instances[i].first.Get(), instances[i].second,
                                                                static_cast<UINT>(i), static_cast<UINT>(i * 2));
        }

        UINT64 scratchSize, resultSize, instanceDescsSize;
        generator.ComputeASBufferSizes(m_device_manager->m_Device.Get(), true, &scratchSize,
                                                                     &resultSize, &instanceDescsSize);

    //    std::cout << scratchSize << " " << resultSize << " " << instanceDescsSize << std::endl;

      nv_helpers_dx12::CreateBuffer(TLAS.pScratch,
            m_device_manager->m_Device.Get(), scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON, nv_helpers_dx12::kDefaultHeapProps);

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(TLAS.pScratch.Get(),
                                                 D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

       nv_helpers_dx12::CreateBuffer(
            TLAS.pResult,
            m_device_manager->m_Device.Get(), resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);

    nv_helpers_dx12::CreateBuffer(TLAS.pInstanceDesc,m_device_manager->m_Device.Get(), instanceDescsSize,
                                                          D3D12_RESOURCE_FLAG_NONE,
                                          D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
    }
 
   generator.Generate(m_device_manager->m_CommandList.Get(), TLAS.pScratch.Get(),
                                                     TLAS.pResult.Get(),
        TLAS.pInstanceDesc.Get(), update, TLAS.pResult.Get());
}
//void ResourceManager::AddBLAS(bee::Mesh mesh)
//{
//
//
//
//}
void ResourceManager::CreateAccelerationStructures()
{
    /*for (int o = 0; o < m_objects.size(); o++)
    {
        for (int m = m_objects[o].m_first_model; m < m_objects[o].m_first_model + m_objects[o].m_model_count; m++)
        {
            AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS(
                {{m_meshes[m_models[m].m_mesh_index].m_vertexBuffer.Get(), m_meshes[m_models[m].m_mesh_index].num_verts}},
                {{m_meshes[m_models[m].m_mesh_index].m_indexBuffer.Get(), m_meshes[m_models[m].m_mesh_index].num_indices}});

            DirectX::XMMATRIX worldMat;

            worldMat = DirectX::XMLoadFloat4x4(&m_models[m].m_WorldMat) * DirectX::XMLoadFloat4x4(&m_objects[o].m_Transform);

            m_device_manager->m_instances.push_back({bottomLevelBuffers.pResult, worldMat});
        };
    }*/
    
    for (const auto& [e, renderer, transform] : bee::Engine.ECS().Registry.view<bee::MeshRenderer, bee::Transform>().each())
        drawables.push_back({e, renderer, transform});

   // std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> instances;

    for (auto& [e, renderer, transform] : drawables)
    {
       // if (!renderer.Material->ReceiveShadows) continue;
          DirectX::XMMATRIX worldMat;

        const glm::mat4 glmWorldMat = transform.WorldMatrix;
          worldMat =   ConvertGLMToDXMatrix(glmWorldMat) ;

        
       
          if (!renderer.Skeleton)
          {
              renderer.Mesh->ResetBLAS();
              m_device_manager->m_instances.push_back({renderer.Mesh->m_BLASes.pResult, worldMat});
          }
          else
          {
             // m_interMesh->ResetBLAS();
              const float posX = glmWorldMat[3][0];
              const float posY = glmWorldMat[3][1];
              const float posZ = glmWorldMat[3][2];

             
              worldMat = DirectX::XMMatrixTranslation(posX, posY, posZ);
              m_device_manager->m_instances.push_back({m_interMesh->m_BLASes.pResult, worldMat});
          }
    }
   
    CreateTopLevelAS(m_device_manager->m_instances, m_device_manager->m_topLevelASBuffers1, m_device_manager->m_topLevelASGenerator,
                     false);


    /*CreateTopLevelAS(m_device_manager->m_instances, m_device_manager->m_topLevelASBuffers2,
                         m_device_manager->m_topLevelASGenerator2, false);*/

    m_device_manager->m_CommandList->Close();
    ID3D12CommandList* ppCommandLists[] = {m_device_manager->m_CommandList.Get()};

    m_device_manager->m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    m_device_manager->m_CommandQueue->Signal(m_device_manager->m_Fence[m_device_manager->m_CurrentBackBufferIndex].Get(),
                                             m_device_manager->m_FenceValue[m_device_manager->m_CurrentBackBufferIndex]);

    m_device_manager->WaitForPreviousFrame();

    m_device_manager->m_FenceValue[m_device_manager->m_CurrentBackBufferIndex]++;

    m_device_manager->m_CommandList->Reset(
        m_device_manager->m_CommandAllocators[m_device_manager->m_CurrentBackBufferIndex].Get(),
        m_device_manager->m_PipelineStateObject.Get());
   
    // m_device_manager->m_bottomLevelAS = bottomLevelBuffers.pResult;
}