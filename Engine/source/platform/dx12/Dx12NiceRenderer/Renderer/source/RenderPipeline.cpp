#include "platform/dx12/RenderPipeline.hpp"

#include <imgui/imgui.h>
#include "pix.h"

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <platform/dx12/skeletal_animation.hpp>
#include <platform/dx12/skeleton.hpp>

#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"
#include "particle_system/particle_system.hpp"
#include "platform/dx12/Helpers.hpp"
#include "platform/dx12/Helpers.hpp"
#include "platform/dx12/mesh_dx12.h"
#include "rendering/render_components.hpp"

bee::RenderPipeline::RenderPipeline()
{
    Title = "Renderer";
    m_device_manager = new DeviceManager();
    m_resource_manager = new ResourceManager(m_device_manager);
    LoadSettings();

}
#include <iostream>
bee::RenderPipeline::~RenderPipeline()
{
    m_device_manager->FlushBuffers();

    delete m_resource_manager;

    delete m_device_manager;
    
    m_device_manager = nullptr;
    m_resource_manager = nullptr;
}

#include "rendering/debug_render.hpp"
void bee::RenderPipeline::Render()
{
    //lets get it here so we dont need to get everywhere
    const auto& drawables = m_resource_manager->drawables;
    const auto& foliageDrawables = m_resource_manager->foliageDrawables;
    m_draw_call_counter = 0;

    if (do_init)
    {
        m_resource_manager->FinishResourceInit();
        m_device_manager->FinishInit();
        do_init = false;
    }

    glm::mat4 transform = glm::mat4(1.0f);
    DirectX::XMFLOAT4X4 temp_mat;
    DirectX::XMStoreFloat4x4(&temp_mat, ConvertGLMToDXMatrix(transform));

    auto cameras = Engine.ECS().Registry.view<Transform, Camera>();
    DirectX::XMFLOAT4X4 view_matrix;
    DirectX::XMFLOAT4X4 projection_matrix;

    m_resource_manager->CleanUp();
    m_device_manager->BeginFrame();

    for (auto e : cameras)
    {
        const auto& camera = cameras.get<Camera>(e);
        const auto& cameraTransform = cameras.get<Transform>(e);

        const glm::mat4 view = inverse(cameraTransform.WorldMatrix);

        DirectX::XMStoreFloat4x4(&view_matrix, ConvertGLMToDXMatrix(view));
        DirectX::XMStoreFloat4x4(&projection_matrix, ConvertGLMToDXMatrix(camera.Projection));
    }

    m_resource_manager->BeginFrame(view_matrix, projection_matrix);

    auto commandAllocator = m_device_manager->m_CommandAllocators[m_device_manager->GetCurrentBufferIndex()];

    auto backBuffer = m_device_manager->m_BackBuffers[m_device_manager->GetCurrentBufferIndex()];
    m_device_manager->m_command_list_closed = false;
    commandAllocator->Reset();
    m_device_manager->GetCommandList()->Reset(commandAllocator.Get(), m_device_manager->m_PipelineStateObject.Get());

    if (m_device_manager->b_rayTracingEnabled)
    {
        // lasa-l asa ca te ard, kind regards Bogdan si Rares
        bool upadte = true;//this is just funny, it does nothing

       

        
            m_device_manager->m_instances.clear();
            for (auto& [e, renderer, transform] : drawables)
            {
                if (!renderer.Material->ReceiveShadows) continue;
               
                DirectX::XMMATRIX worldMat;


               const glm::mat4 glmWorldMat = transform.WorldMatrix;
                worldMat = ConvertGLMToDXMatrix(glmWorldMat);
                if (!renderer.Skeleton)
                {
                    if (renderer.Mesh->b_resetBLAS)
                    {
                        renderer.Mesh->ResetBLAS();
                        renderer.Mesh->b_resetBLAS = false;
                    }
                    m_device_manager->m_instances.push_back({renderer.Mesh->m_BLASes.pResult, worldMat});
                }
                else
                {
                    const float posX = glmWorldMat[3][0];
                    const float posY = glmWorldMat[3][1];
                    const float posZ = glmWorldMat[3][2];

                    worldMat = DirectX::XMMatrixTranslation(posX, posY, posZ);
                    m_device_manager->m_instances.push_back({m_resource_manager->m_interMesh->m_BLASes.pResult, worldMat});
                }

            }
            for (auto& [e, renderer, transform] : foliageDrawables)
            {
                if (!renderer.Material->ReceiveShadows) continue;
                
                DirectX::XMMATRIX worldMat;

                const glm::mat4 glmWorldMat = transform.WorldMatrix;
                worldMat = ConvertGLMToDXMatrix(glmWorldMat);
                if (!renderer.Skeleton)
                {
                    if (renderer.Mesh->b_resetBLAS)
                    {
                        renderer.Mesh->ResetBLAS();
                        renderer.Mesh->b_resetBLAS = false;
                    }
                    m_device_manager->m_instances.push_back({renderer.Mesh->m_BLASes.pResult, worldMat});
                }
                else
                {
                    const float posX = glmWorldMat[3][0];
                    const float posY = glmWorldMat[3][1];
                    const float posZ = glmWorldMat[3][2];

                    worldMat = DirectX::XMMatrixTranslation(posX, posY, posZ);
                    m_device_manager->m_instances.push_back({m_resource_manager->m_interMesh->m_BLASes.pResult, worldMat});
                }
            }
            if (!m_device_manager->m_instances.empty())
            {
                m_resource_manager->m_deletion_queue.push(m_device_manager->m_topLevelASBuffers1.pResult);
                m_device_manager->m_topLevelASBuffers1.pResult = nullptr;
                m_resource_manager->m_deletion_queue.push(m_device_manager->m_topLevelASBuffers1.pScratch);
                m_device_manager->m_topLevelASBuffers1.pScratch = nullptr;
                m_resource_manager->m_deletion_queue.push(m_device_manager->m_topLevelASBuffers1.pInstanceDesc);
                m_device_manager->m_topLevelASBuffers1.pInstanceDesc = nullptr;

                m_resource_manager->CreateTopLevelAS(m_device_manager->m_instances, m_device_manager->m_topLevelASBuffers1,
                                                     m_device_manager->m_topLevelASGenerator, false);

                UINT descriptorSize =
                    m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_resource_manager->m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

                srvHandle.ptr += (7 * descriptorSize);

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.RaytracingAccelerationStructure.Location =
                    m_device_manager->m_topLevelASBuffers1.pResult->GetGPUVirtualAddress();

                m_device_manager->m_Device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

                m_resource_manager->m_deletion_queue.push(m_device_manager->m_sbtStorage);
                m_device_manager->m_sbtStorage = nullptr;

                m_resource_manager->CreateShaderBindingTable();
            }
       
    }


     for (int i = 0; i < m_resource_manager->m_queuedImages.size(); i++)
    {
         m_resource_manager->m_queuedImages[i]->WaitForLoad();
    }
    for (int i = 0; i < m_resource_manager->m_queuedImages.size(); i++)
    {
        m_resource_manager->m_queuedImages[i]->UploadGPU();
    }
    m_resource_manager->m_queuedImages.clear();


    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv1(
        m_device_manager->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_device_manager->GetCurrentBufferIndex(),
        m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(
        m_device_manager->m_DepthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    m_device_manager->GetCommandList()->RSSetViewports(1, &m_device_manager->m_Viewport);

    m_device_manager->GetCommandList()->RSSetScissorRects(1, &m_device_manager->m_ScissorRect);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT,
                                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);

    {
        CD3DX12_RESOURCE_BARRIER barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
            m_device_manager->m_posBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
        CD3DX12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
            m_device_manager->m_normalBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
        CD3DX12_RESOURCE_BARRIER barrier3 = CD3DX12_RESOURCE_BARRIER::Transition(
            m_device_manager->m_colorBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
        CD3DX12_RESOURCE_BARRIER barrier4 =
            CD3DX12_RESOURCE_BARRIER::Transition(m_device_manager->m_materialBuffer.Get(),
                                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

        {
            D3D12_RESOURCE_BARRIER barriers[5] = {barrier, barrier1, barrier2, barrier3, barrier4};

            m_device_manager->GetCommandList()->ResourceBarrier(5, barriers);
        }
    }

    m_device_manager->GetCommandList()->ClearRenderTargetView(rtv1, m_clear_color, 0, nullptr);

    m_device_manager->GetCommandList()->ClearDepthStencilView(
        m_device_manager->m_DepthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0,
        0, nullptr);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvO(
        m_device_manager->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 7,
        m_device_manager->m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv[4];

        rtv[0] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_device_manager->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 3,
                                               m_device_manager->m_RTVDescriptorSize);
        rtv[1] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_device_manager->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 4,
                                               m_device_manager->m_RTVDescriptorSize);
        rtv[2] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_device_manager->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 5,
                                               m_device_manager->m_RTVDescriptorSize);
        rtv[3] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_device_manager->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 6,
                                               m_device_manager->m_RTVDescriptorSize);
        FLOAT clearColor0[] = {0.0f, 0.0f, 0.0f, 0.0f};
      //  m_device_manager->GetCommandList()->ClearRenderTargetView(rtv[0], clearColor0, 0, nullptr);
       // m_device_manager->GetCommandList()->ClearRenderTargetView(rtv[1], clearColor0, 0, nullptr);
        m_device_manager->GetCommandList()->ClearRenderTargetView(rtv[2], clearColor0, 0, nullptr);
     //   m_device_manager->GetCommandList()->ClearRenderTargetView(rtv[3], clearColor0, 0, nullptr);
        m_device_manager->GetCommandList()->OMSetRenderTargets(_countof(rtv), rtv, FALSE, &dsvHandle);
    }

     m_device_manager->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      m_device_manager->GetCommandList()->SetGraphicsRootSignature(m_device_manager->m_terrainRootSignature.Get());
    m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_terrainPipelineStateObject.Get());


    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress1 =
        m_resource_manager->m_ConstantBufferUploadHeaps[m_device_manager->GetCurrentBufferIndex()]->GetGPUVirtualAddress();

    m_device_manager->GetCommandList()->SetGraphicsRootShaderResourceView(2, gpuAddress1);

    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress2 =
        m_resource_manager->m_ConstantBufferUploadHeapsJoints[m_device_manager->GetCurrentBufferIndex()]
            ->GetGPUVirtualAddress();

    m_device_manager->GetCommandList()->SetGraphicsRootShaderResourceView(3, gpuAddress2);

    ID3D12DescriptorHeap* descriptorHeaps[] = {m_resource_manager->m_TexturesDescriptorHeap.Get()};
    m_device_manager->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStartHandle(
        m_resource_manager->m_TexturesDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    UINT descriptorSize =
        m_device_manager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_device_manager->GetCommandList()->SetGraphicsRootShaderResourceView(
        1, m_resource_manager->m_ConstantBufferUploadHeapsMaterials[m_device_manager->GetCurrentBufferIndex()]
               ->GetGPUVirtualAddress());

    int albedo_texture_index = 0;
    gpuStartHandle.Offset(albedo_texture_index, descriptorSize);
    m_device_manager->GetCommandList()->SetGraphicsRootDescriptorTable(4, gpuStartHandle);

    int j = 0;

    //terrain
    {
        const MeshRenderer& terrainRenderer = std::get<1>(drawables[0]);

        j = terrainRenderer.Mesh->m_mesh_index;

        m_device_manager->GetCommandList()->IASetVertexBuffers(0, 1, &terrainRenderer.Mesh->m_vertexBufferView);
        m_device_manager->GetCommandList()->IASetIndexBuffer(&terrainRenderer.Mesh->m_indexBufferView);
        
        m_device_manager->GetCommandList()->SetGraphicsRootConstantBufferView(
            0, m_resource_manager->m_ConstantBufferUploadHeapsPerMesh[m_device_manager->GetCurrentBufferIndex()]
                       ->GetGPUVirtualAddress() +
                   j * m_resource_manager->ConstantBufferPerMeshAlignedSize);

      m_device_manager->GetCommandList()->DrawIndexedInstanced(terrainRenderer.Mesh->m_num_indices,
        //   m_device_manager->GetCommandList()->DrawIndexedInstanced(renderer.Mesh->m_num_indices,
                                                                 1, 0, 0, 0);
        m_draw_call_counter++;
        m_resource_manager->m_instance_counter[j] = 0;

    }
   

    m_device_manager->GetCommandList()->SetGraphicsRootSignature(m_device_manager->m_RootSignature.Get());
    m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_PipelineStateObject.Get());

    for(int i=1;i<drawables.size();i++)
    {
        const auto& [e, renderer, transform] = drawables[i];
        
        j = renderer.Mesh->m_mesh_index;
        if (m_resource_manager->m_instance_counter[j] == 0) continue;

        m_device_manager->GetCommandList()->IASetVertexBuffers(0, 1, &renderer.Mesh->m_vertexBufferView);
        m_device_manager->GetCommandList()->IASetIndexBuffer(&renderer.Mesh->m_indexBufferView);

        m_device_manager->GetCommandList()->SetGraphicsRootConstantBufferView(
            0, m_resource_manager->m_ConstantBufferUploadHeapsPerMesh[m_device_manager->GetCurrentBufferIndex()]
                       ->GetGPUVirtualAddress() +
                   j * m_resource_manager->ConstantBufferPerMeshAlignedSize);

        m_device_manager->GetCommandList()->DrawIndexedInstanced(renderer.Mesh->m_num_indices,
                                                                 m_resource_manager->m_instance_counter[j], 0, 0, 0);
        m_draw_call_counter++;
        m_resource_manager->m_instance_counter[j] = 0;
    }

    m_device_manager->GetCommandList()->SetGraphicsRootSignature(m_device_manager->m_foliageRootSignature.Get());
    m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_foliagePipelineStateObject.Get());

    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
    const float value = elapsed / 1000.0f;  // Convert milliseconds to seconds
    m_device_manager->GetCommandList()->SetGraphicsRoot32BitConstants(3, 1, &value, 0);

    for (const auto& [e, renderer, transform] : foliageDrawables)
    {
       
        j = renderer.Mesh->m_mesh_index;
        if (m_resource_manager->m_instance_counter[j] == 0) continue;
        PIXBeginEvent( m_device_manager->GetCommandList().Get(), PIX_COLOR(255,0,0),"Foliage Drawables");

        m_device_manager->GetCommandList()->IASetVertexBuffers(0, 1, &renderer.Mesh->m_vertexBufferView);
        m_device_manager->GetCommandList()->IASetIndexBuffer(&renderer.Mesh->m_indexBufferView);

        m_device_manager->GetCommandList()->SetGraphicsRootConstantBufferView(
            0, m_resource_manager->m_ConstantBufferUploadHeapsPerMesh[m_device_manager->GetCurrentBufferIndex()]
                       ->GetGPUVirtualAddress() +
                   j * m_resource_manager->ConstantBufferPerMeshAlignedSize);

        m_device_manager->GetCommandList()->DrawIndexedInstanced(renderer.Mesh->m_num_indices,
                                                                 m_resource_manager->m_instance_counter[j], 0, 0, 0);
        PIXEndEvent(m_device_manager->GetCommandList().Get());
        m_draw_call_counter++;
        m_resource_manager->m_instance_counter[j] = 0;
    }

    CD3DX12_RESOURCE_BARRIER barrierrr9 =
        CD3DX12_RESOURCE_BARRIER::Transition(m_device_manager->m_outputResource.Get(),
                                             D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

   

    CD3DX12_RESOURCE_BARRIER barrierrr5 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_device_manager->m_posBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    CD3DX12_RESOURCE_BARRIER barrierrr6 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_device_manager->m_normalBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    CD3DX12_RESOURCE_BARRIER barrierrr7 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_device_manager->m_colorBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    CD3DX12_RESOURCE_BARRIER barrierrr8 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_device_manager->m_materialBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    {
        D3D12_RESOURCE_BARRIER barriers[5] = {barrierrr5, barrierrr6, barrierrr7, barrierrr8, barrierrr9};

        m_device_manager->GetCommandList()->ResourceBarrier(5, barriers);
    }

    std::vector<ID3D12DescriptorHeap*> heaps = {m_resource_manager->m_srvUavHeap.Get()};
    m_device_manager->GetCommandList()->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

    if (m_device_manager->b_rayTracingEnabled)
    {
        D3D12_DISPATCH_RAYS_DESC desc = {};

        uint32_t rayGenerationSectionSizeInBytes = m_device_manager->m_sbtHelper.GetRayGenSectionSize();
        desc.RayGenerationShaderRecord.StartAddress = m_device_manager->m_sbtStorage->GetGPUVirtualAddress();
        desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

        uint32_t missSectionSizeInBytes = m_device_manager->m_sbtHelper.GetMissSectionSize();

        desc.MissShaderTable.StartAddress =
            m_device_manager->m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
        desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
        desc.MissShaderTable.StrideInBytes = m_device_manager->m_sbtHelper.GetMissEntrySize();

        uint32_t hitGroupsSectionSize = m_device_manager->m_sbtHelper.GetHitGroupSectionSize();

        desc.HitGroupTable.StartAddress =
            m_device_manager->m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes + missSectionSizeInBytes;

        desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
        desc.HitGroupTable.StrideInBytes = m_device_manager->m_sbtHelper.GetHitGroupEntrySize();

        desc.Width = m_device_manager->width ;   
        desc.Height = m_device_manager->height;  
        desc.Depth = 1;

        m_device_manager->GetCommandList()->SetPipelineState1(m_device_manager->m_rtStateObject.Get());

        m_device_manager->GetCommandList()->DispatchRays(&desc);
    }
    {
        m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_DIPipelineStateObject.Get());

        m_device_manager->GetCommandList()->SetComputeRootSignature(m_device_manager->m_DIRootSignature.Get());

        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(
            m_resource_manager->m_srvUavHeap->GetGPUDescriptorHandleForHeapStart(), 0,
            m_device_manager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

        m_device_manager->GetCommandList()->SetComputeRootConstantBufferView(
            0, m_resource_manager->m_ConstantBufferUploadHeapsCamera[m_device_manager->GetCurrentBufferIndex()]
                   ->GetGPUVirtualAddress());

        m_device_manager->GetCommandList()->SetComputeRootShaderResourceView(
            1, m_resource_manager->m_ConstantBufferUploadHeapsForLights[m_device_manager->GetCurrentBufferIndex()]
                   ->GetGPUVirtualAddress());

        m_device_manager->GetCommandList()->SetComputeRootDescriptorTable(2, gpuHandle);

        D3D12_RESOURCE_BARRIER uavBarrier = {};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = m_device_manager->m_shadowBuffer.Get();  // Replace with your actual resource
        m_device_manager->GetCommandList()->ResourceBarrier(1, &uavBarrier);

        const unsigned int numThreadsX = 16;
        const unsigned int numThreadsY = 16;
        const unsigned int numGroupsX = (m_device_manager->width + numThreadsX - 1) / numThreadsX;
        const unsigned int numGroupsY = (m_device_manager->height + numThreadsY - 1) / numThreadsY;

        PIXBeginEvent( m_device_manager->GetCommandList().Get(), PIX_COLOR(255,0,255),"DI compute");
        m_device_manager->GetCommandList()->Dispatch(numGroupsX, numGroupsY, 1);
        PIXEndEvent(m_device_manager->GetCommandList().Get());

    }

    barrierrr9 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_device_manager->m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

    {
        D3D12_RESOURCE_BARRIER barriers[1] = {barrierrr9};

        m_device_manager->GetCommandList()->ResourceBarrier(1, barriers);
    }

    m_device_manager->GetCommandList()->OMSetRenderTargets(1, &rtvO, FALSE, &dsvHandle);

    m_device_manager->GetCommandList()->SetGraphicsRootSignature(m_device_manager->m_particleRootSignature.Get());
    m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_particlePipelineStateObject.Get());

    m_device_manager->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    for (const auto& [e, renderer, transform] : m_resource_manager->particleDrawables)
    {
        j = renderer.Mesh->m_mesh_index;
        if (m_resource_manager->m_instance_counter[j] == 0) continue;

        m_device_manager->GetCommandList()->IASetVertexBuffers(0, 1, &renderer.Mesh->m_vertexBufferView);

        m_device_manager->GetCommandList()->IASetIndexBuffer(&renderer.Mesh->m_indexBufferView);

        m_device_manager->GetCommandList()->SetGraphicsRootConstantBufferView(
            0, m_resource_manager->m_ConstantBufferUploadHeapsPerMesh[m_device_manager->GetCurrentBufferIndex()]
                       ->GetGPUVirtualAddress() +
                   j * m_resource_manager->ConstantBufferPerMeshAlignedSize);

        m_device_manager->GetCommandList()->DrawIndexedInstanced(renderer.Mesh->m_num_indices,
                                                                 m_resource_manager->m_instance_counter[j], 0, 0, 0);
        m_draw_call_counter++;
        m_resource_manager->m_instance_counter[j] = 0;
    }

    m_device_manager->GetCommandList()->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

    {
        m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_BloomCopyPipelineStateObject.Get());

        m_device_manager->GetCommandList()->SetComputeRootSignature(m_device_manager->m_BloomCopyRootSignature.Get());

        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(
            m_resource_manager->m_srvUavHeap->GetGPUDescriptorHandleForHeapStart(), 0,
            m_device_manager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

        m_device_manager->GetCommandList()->SetComputeRootDescriptorTable(1, gpuHandle);

        m_device_manager->GetCommandList()->SetComputeRootConstantBufferView(
            0, m_resource_manager->m_ConstantBufferUploadHeapsBloom[m_device_manager->GetCurrentBufferIndex()]
                   ->GetGPUVirtualAddress());

        const unsigned int numThreadsX = 16;
        const unsigned int numThreadsY = 16;


        unsigned int numGroupsX = (m_device_manager->width + numThreadsX - 1) / numThreadsX;
        unsigned int numGroupsY = (m_device_manager->height + numThreadsY - 1) / numThreadsY;
        PIXBeginEvent( m_device_manager->GetCommandList().Get(), PIX_COLOR(255,0,255),"bloom copy compute");

        m_device_manager->GetCommandList()->Dispatch(numGroupsX, numGroupsY, 1);
        PIXEndEvent(m_device_manager->GetCommandList().Get());

        m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_BloomHPipelineStateObject.Get());

        m_device_manager->GetCommandList()->SetComputeRootSignature(m_device_manager->m_BloomHRootSignature.Get());

        PIXBeginEvent( m_device_manager->GetCommandList().Get(), PIX_COLOR(255,0,255),"bloom horizontal compute");
        m_device_manager->GetCommandList()->Dispatch(numGroupsX, numGroupsY, 1);
        PIXEndEvent(m_device_manager->GetCommandList().Get());


        D3D12_RESOURCE_BARRIER uavBarrier2 = {};
        uavBarrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier2.UAV.pResource = m_device_manager->m_outputResource.Get();  // Replace with your actual resource
        m_device_manager->GetCommandList()->ResourceBarrier(1, &uavBarrier2);

        m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_BloomVPipelineStateObject.Get());

        m_device_manager->GetCommandList()->SetComputeRootSignature(m_device_manager->m_BloomVRootSignature.Get());

        PIXBeginEvent( m_device_manager->GetCommandList().Get(), PIX_COLOR(255,0,255),"bloom vertical compute");
        m_device_manager->GetCommandList()->Dispatch(numGroupsX, numGroupsY, 1);
        PIXEndEvent(m_device_manager->GetCommandList().Get());

        m_device_manager->GetCommandList()->SetPipelineState(m_device_manager->m_ImageEffectsPipelineStateObject.Get());
        m_device_manager->GetCommandList()->SetComputeRootSignature(m_device_manager->m_ImageEffectsRootSignature.Get());

        m_device_manager->GetCommandList()->SetComputeRootConstantBufferView(
            0, m_resource_manager->m_ConstantBufferUploadHeapsImageEffects[m_device_manager->GetCurrentBufferIndex()]
                   ->GetGPUVirtualAddress());

        PIXBeginEvent( m_device_manager->GetCommandList().Get(), PIX_COLOR(255,0,255),"post processing compute");
        m_device_manager->GetCommandList()->Dispatch(numGroupsX, numGroupsY, 1);
        PIXEndEvent(m_device_manager->GetCommandList().Get());
        
    }

}
#ifdef BEE_INSPECTOR
void bee::RenderPipeline::Inspect()
{
    System::Inspect();
    ImGui::Begin(Title.c_str());

    std::string drawcalltext;
    drawcalltext = "Draw call count: " + std::to_string(m_draw_call_counter);

    ImGui::Text(drawcalltext.c_str());

    ImGui::Separator();
    if (ImGui::Button("Save Settings"))
    {
        SaveSettings();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Settings"))
    {
        LoadSettings();
    }
    ImGui::Separator();
    ImGui::Checkbox("Unlit", &m_resource_manager->b_unlit);
    if (ImGui::CollapsingHeader("Image correction"))
    {
        ImGui::ColorEdit3("HSV image filter", &m_resource_manager->m_image_effects_cb.hsvc.x,
                          ImGuiColorEditFlags_InputHSV | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_HDR |
                              ImGuiColorEditFlags_Float);
        ImGui::DragFloat("Contrast", &m_resource_manager->m_image_effects_cb.hsvc.w, 0.01f, 0.0f);
        ImGui::DragFloat("Fil Grain", &m_resource_manager->m_image_effects_cb.grainAmount, 0.005f, 0.0f);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.2f);
        ImGui::ColorPicker3("Ambient Light", &m_resource_manager->m_cameraCB.ambient_light.x);
    }
    if (ImGui::CollapsingHeader("Vignette"))
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.2f);
        ImGui::ColorPicker3("Vignette Color", &m_resource_manager->m_image_effects_cb.vignetteColor.x);
        ImGui::DragFloat("Vignette Radius", &m_resource_manager->m_image_effects_cb.vignetteSettings.y, 0.01f, 0.0f);
        ImGui::DragFloat("Vignette Strength", &m_resource_manager->m_image_effects_cb.vignetteSettings.x, 0.01f, 0.0f);
    }
    if (ImGui::CollapsingHeader("Bloom"))
    {
        ImGui::DragFloat("Bloom threshold", &m_resource_manager->m_bloom_cb.bloom_threshold, 0.01f, 0.0f);
        ImGui::DragInt("Bloom influence", &m_resource_manager->m_bloom_cb.bloom_sigma, 2, 0, 128);
        ImGui::DragInt("Bloom size", &m_resource_manager->m_bloom_cb.kernel_size, 2, 0, 128);
        ImGui::DragFloat("Bloom multiplier", &m_resource_manager->m_bloom_cb.bloom_multiplier, 0.01f, 0.0f);
    }
    if (ImGui::CollapsingHeader("Sky"))
    {
        ImGui::DragFloat("Fog Height Offset", &m_resource_manager->m_image_effects_cb.fogOffset, 0.01f, 0.0f);
        ImGui::DragFloat("Air Density", &m_resource_manager->m_image_effects_cb.airDensity, 0.01f, 0.0f);
        ImGui::ColorPicker3("Fog Color", &m_resource_manager->m_image_effects_cb.fogColor.x);
    }

    // m_resource_manager->m_image_effects_cb.hsvc 

    ImGui::End();
}
#endif

void bee::RenderPipeline::LoadSettings()
{
    if (Engine.FileIO().Exists(FileIO::Directory::None, "graphics.ini"))
    {
        std::ifstream is(Engine.FileIO().GetPath(FileIO::Directory::None, "graphics.ini"));
        /*cereal::JSONInputArchive archive(is);
        archive(m_resource_manager->m_image_effects_cb,
        m_resource_manager->m_cameraCB.bloom_threshold,m_resource_manager->m_bloom_cb);*/

        try
        {
            cereal::JSONInputArchive archive(is);
            archive(m_resource_manager->m_image_effects_cb, m_resource_manager->m_cameraCB.ambient_light,
                    m_resource_manager->m_bloom_cb);
        }
        catch (cereal::Exception& e)  // Catch exceptions specific to cereal
        {
            std::cerr << "Exception caught during graphics settings load: " << e.what() << std::endl;
        }
    }
}

void bee::RenderPipeline::SaveSettings()
{
    std::ofstream os(Engine.FileIO().GetPath(FileIO::Directory::None, "graphics.ini"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(m_resource_manager->m_image_effects_cb), CEREAL_NVP(m_resource_manager->m_cameraCB.ambient_light),
            CEREAL_NVP(m_resource_manager->m_bloom_cb));
}