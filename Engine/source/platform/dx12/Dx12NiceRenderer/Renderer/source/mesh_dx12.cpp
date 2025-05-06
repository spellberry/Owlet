#include "platform/dx12/mesh_dx12.h"

#include <iostream>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "rendering/model.hpp"
using namespace bee;

static uint32_t CalculateDataTypeSize(tinygltf::Accessor const& accessor) noexcept;
void Mesh::ResetBLAS() 
{
    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();

    if (m_BLASes.pResult)
    {
        Renderer.GetResourceManager()->m_deletion_queue.push(m_BLASes.pResult);
        Renderer.GetResourceManager()->m_deletion_queue.push(m_BLASes.pScratch);


        m_BLASes.pResult = nullptr;
        m_BLASes.pScratch = nullptr;

    }
    AccelerationStructureBuffers bottomLevelBuffers =
        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateBottomLevelAS(
            {{m_vertexBuffer.Get(), m_num_verts}}, {{m_indexBuffer.Get(), m_num_indices}});
    

    m_BLASes = bottomLevelBuffers;
    b_resetBLAS = false;
}
Mesh::Mesh() : Resource(ResourceType::Mesh)
{
    m_generated = true;

    std::vector<Vertex> vList;
    std::vector<DWORD> iList;

    Vertex v;
    DWORD i;
    vList.push_back(v);
    iList.push_back(i);
    CreateVertexBuffer(vList);
    CreateIndexBuffer(iList);

    m_path = "Generated Mesh " + std::to_string(Resource::GetNextGeneratedID());

    m_mesh_index = Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->m_mesh_count;
    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->m_mesh_count++;

      b_resetBLAS = true;
   
   /* AccelerationStructureBuffers bottomLevelBuffers =
        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateBottomLevelAS(
            {{m_vertexBuffer.Get(),m_num_verts}},
                            {{m_indexBuffer.Get(),m_num_indices}});*/

 //    m_BLASBuffer = bottomLevelBuffers.pResult;

}
Mesh::Mesh(const Model& model, int index) : Resource(ResourceType::Mesh)
{
    const auto& document = model.GetDocument();
    auto mesh = document.meshes[index];

    m_path = GetPath(model, index);

    assert(!mesh.primitives.empty());
    auto primitive = mesh.primitives[0];

    std::vector<Vertex> vList;
    std::vector<DWORD> iList;

    tinygltf::Accessor accessor = document.accessors[primitive.indices];
    tinygltf::BufferView bufferView = document.bufferViews[accessor.bufferView];

    int previousPrimOffset = vList.size();

    auto posAttrib = primitive.attributes.find("POSITION");

    if (posAttrib != primitive.attributes.end())
    {
        const tinygltf::Accessor& Paccessor = document.accessors[posAttrib->second];

        const tinygltf::BufferView& PbufferView = document.bufferViews[Paccessor.bufferView];

        const tinygltf::Buffer& buffer = document.buffers[PbufferView.buffer];

        const float* positions = reinterpret_cast<const float*>(&buffer.data[PbufferView.byteOffset + Paccessor.byteOffset]);
        Vertex vertex;

        for (size_t i = 0; i < Paccessor.count; ++i)
        {
            vertex = Vertex(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            vList.push_back(vertex);
        }
    }
    auto texAttrib = primitive.attributes.find("TEXCOORD_0");
    if (texAttrib != primitive.attributes.end())
    {
        const tinygltf::Accessor& accessor = document.accessors[texAttrib->second];

        const tinygltf::BufferView& bufferView = document.bufferViews[accessor.bufferView];

        const tinygltf::Buffer& buffer = document.buffers[bufferView.buffer];

        const float* tex = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

        for (size_t i = 0; i < accessor.count; ++i)
        {
            vList[i].texCoord.x = tex[i * 2 + 0];
            vList[i].texCoord.y = tex[i * 2 + 1];
        }
    }

    auto normalAttrib = primitive.attributes.find("NORMAL");

    if (normalAttrib != primitive.attributes.end())
    {
        const tinygltf::Accessor& accessor = document.accessors[normalAttrib->second];

        const tinygltf::BufferView& bufferView = document.bufferViews[accessor.bufferView];

        const tinygltf::Buffer& buffer = document.buffers[bufferView.buffer];

        const float* norm = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

        for (size_t i = 0; i < accessor.count; ++i)
        {
            vList[i].normal.x = norm[i * 3 + 0];
            vList[i].normal.y = norm[i * 3 + 1];
            vList[i].normal.z = norm[i * 3 + 2];
        }
    }

     auto jointsAttrib = primitive.attributes.find("JOINTS_0");

    if (jointsAttrib != primitive.attributes.end())
    {
        const tinygltf::Accessor& accessor = document.accessors[jointsAttrib->second];

        const tinygltf::BufferView& bufferView = document.bufferViews[accessor.bufferView];

        const tinygltf::Buffer& buffer = document.buffers[bufferView.buffer];

     //   const int8_t* joints = reinterpret_cast<const int8_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

         switch (accessor.componentType)
        {
            case 5121:
            {  
                const uint8_t* joints =
                    reinterpret_cast<const uint8_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    vList[i].jointids.x = joints[i * 4 + 0];
                    vList[i].jointids.y = joints[i * 4 + 1];
                    vList[i].jointids.z = joints[i * 4 + 2];
                    vList[i].jointids.w = joints[i * 4 + 3];

                  /*  std::cout << "ID: " << vList[i].jointids.x << " " << vList[i].jointids.y << " " << vList[i].jointids.z
                              << " "
                              << vList[i].jointids.w << std::endl;*/
                }
                break;
            }
            case 5123:
            {  
                const uint16_t* joints =
                    reinterpret_cast<const uint16_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    vList[i].jointids.x = joints[i *4 + 0];
                    vList[i].jointids.y = joints[i * 4 + 1];
                    vList[i].jointids.z = joints[i *4 + 2];
                    vList[i].jointids.w = joints[i *4 + 3];

                    
                  /*  std::cout << "ID: " << vList[i].jointids.x << " " << vList[i].jointids.y << " " << vList[i].jointids.z
                              << " "
                              << vList[i].jointids.w << std::endl;*/
                }
                break;
            }
          
            default:
                std::cerr << "Unsupported component type for JOINTS_0: " << accessor.componentType << std::endl;
                break;
        }



       
    }


    auto weightsAttrib = primitive.attributes.find("WEIGHTS_0");

    if (weightsAttrib != primitive.attributes.end())
    {
        const tinygltf::Accessor& accessor = document.accessors[weightsAttrib->second];

        const tinygltf::BufferView& bufferView = document.bufferViews[accessor.bufferView];

        const tinygltf::Buffer& buffer = document.buffers[bufferView.buffer];

        const float* weights = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

        for (size_t i = 0; i < accessor.count; ++i)
        {
            vList[i].weights.x = weights[i * 4 + 0];
            vList[i].weights.y = weights[i * 4 + 1];
            vList[i].weights.z = weights[i * 4 + 2];
            vList[i].weights.w = weights[i * 4 + 3];

         /*   std::cout << "Weights: " << vList[i].weights.x << " " << vList[i].weights.y << " " << vList[i].weights.z << " "
                      << vList[i].weights.w << std::endl;*/

        }
    }





    const tinygltf::Buffer& indexBuffer = document.buffers[bufferView.buffer];

    for (size_t i = 0; i < accessor.count; ++i)
    {
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        {
            const uint32_t* buf =
                reinterpret_cast<const uint32_t*>(&indexBuffer.data[bufferView.byteOffset + accessor.byteOffset]);
            iList.push_back(buf[i] + previousPrimOffset);
        }
        else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            const uint16_t* buf =
                reinterpret_cast<const uint16_t*>(&indexBuffer.data[bufferView.byteOffset + accessor.byteOffset]);
            iList.push_back(buf[i] + previousPrimOffset);
        }
        else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            const uint8_t* buf =
                reinterpret_cast<const uint8_t*>(&indexBuffer.data[bufferView.byteOffset + accessor.byteOffset]);
            iList.push_back(buf[i] + previousPrimOffset);
        }
    }

    for (size_t i = 0; i < iList.size(); i += 3)
    {
        Vertex& v0 = vList[iList[i]];
        Vertex& v1 = vList[iList[i + 1]];
        Vertex& v2 = vList[iList[i + 2]];

        DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&v1.pos), DirectX::XMLoadFloat3(&v0.pos));
        DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&v2.pos), DirectX::XMLoadFloat3(&v0.pos));

        DirectX::XMVECTOR deltaUV1 =
            DirectX::XMVectorSubtract(DirectX::XMLoadFloat2(&v1.texCoord), DirectX::XMLoadFloat2(&v0.texCoord));
        DirectX::XMVECTOR deltaUV2 =
            DirectX::XMVectorSubtract(DirectX::XMLoadFloat2(&v2.texCoord), DirectX::XMLoadFloat2(&v0.texCoord));

        float f = 1.0f / (DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetY(deltaUV2) -
                          DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetX(deltaUV2));

        DirectX::XMVECTOR tangent = DirectX::XMVectorScale(
            DirectX::XMVectorSubtract(
                DirectX::XMVectorMultiply(DirectX::XMVectorReplicate(DirectX::XMVectorGetY(deltaUV2)), edge1),
                DirectX::XMVectorMultiply(DirectX::XMVectorReplicate(DirectX::XMVectorGetY(deltaUV1)), edge2)),
            f);

        DirectX::XMVECTOR bitangent = DirectX::XMVectorScale(
            DirectX::XMVectorSubtract(
                DirectX::XMVectorMultiply(DirectX::XMVectorReplicate(DirectX::XMVectorGetX(deltaUV1)), edge2),
                DirectX::XMVectorMultiply(DirectX::XMVectorReplicate(DirectX::XMVectorGetX(deltaUV2)), edge1)),
            f);

        DirectX::XMFLOAT3 t, b;
        DirectX::XMStoreFloat3(&t, tangent);
        DirectX::XMStoreFloat3(&b, bitangent);

        v0.tangent = t;
        v1.tangent = t;
        v2.tangent = t;

        v0.bitangent = b;
        v1.bitangent = b;
        v2.bitangent = b;

        /* {
             DWORD temp = iList[i + 1];
             iList[i + 1] = iList[i + 2];
             iList[i + 2] = temp;
         }*/
    }
    for (Vertex& vertex : vList)
    {
        DirectX::XMVECTOR t = XMLoadFloat3(&vertex.tangent);
        DirectX::XMVECTOR b = XMLoadFloat3(&vertex.bitangent);

        t = DirectX::XMVector3Normalize(t);
        b = DirectX::XMVector3Normalize(b);

        XMStoreFloat3(&vertex.tangent, t);
        XMStoreFloat3(&vertex.bitangent, b);
    }


    // m_count = static_cast<uint32_t>(accessor.count);
    // m_indexType = static_cast<uint32_t>(accessor.componentType);

    UINT back_index = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex();
    bool close = false;

    if (Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed)
    {
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->WaitForPreviousFrame();
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]++;

        auto commandAllocator = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandAllocators[back_index];

        commandAllocator->Reset();
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Reset(
            commandAllocator.Get(), Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_PipelineStateObject.Get());
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed = false;
        close = true;
    }

    CreateVertexBuffer(vList);
    CreateIndexBuffer(iList);







    if (close)
    {
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Close();

        ID3D12CommandList* ppCommandLists[] = {
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get()};

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->ExecuteCommandLists(
            _countof(ppCommandLists), ppCommandLists);
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed = true;
        UINT back_index = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex();

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]++;

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->Signal(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_Fence[back_index].Get(),Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]);
    }

    m_mesh_index = Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->m_mesh_count;
    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->m_mesh_count++;

      b_resetBLAS = true;
 /*   AccelerationStructureBuffers bottomLevelBuffers =
        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateBottomLevelAS(
            {{m_vertexBuffer.Get(), m_num_verts}}, {{m_indexBuffer.Get(), m_num_indices}});
  
    m_BLASBuffer = bottomLevelBuffers.pResult;*/

    // Index buffer
    // Get index data from GLTF
    /* const auto& accessor = document.accessors[primitive.indices];
     const auto& view = document.bufferViews[accessor.bufferView];
     const auto& buffer = document.buffers[view.buffer];*/

    // m_count = static_cast<uint32_t>(accessor.count);
    //   m_indexType = static_cast<uint32_t>(accessor.componentType);
    //   auto typeSize = CalculateDataTypeSize(accessor);
}
#include <iostream>
Mesh::~Mesh() 
{ 
//    std::cout << "delete";
}
std::string Mesh::GetPath(const Model& model, int index)
{
    const auto& mesh = model.GetDocument().meshes[index];
    return model.GetPath() + " | Mesh-" + std::to_string(index) + ": " + mesh.name;
}
void Mesh::SetAttribute(std::vector<Vertex>& data)
{
    UINT back_index = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex();
    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();

    if (m_num_verts != data.size())
    {
        //  Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->Flush();
        //  m_vertexBuffer->Release();
        //    vBufferUploadHeap->Release();

        /* if (m_num_verts>1)
         {
             Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Close();

             ID3D12CommandList* ppCommandLists[] = {
                 Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get()};

             Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->ExecuteCommandLists(
                 _countof(ppCommandLists), ppCommandLists);

             UINT back_index = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex();

             Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]++;

             Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->Signal(
                 Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_Fence[back_index].Get(),
                 Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]);


         }*/
        Renderer.GetResourceManager()->m_deletion_queue.push(m_vertexBuffer);

        Renderer.GetResourceManager()->m_deletion_queue.push(vBufferUploadHeap);

      //  Renderer.GetResourceManager()->m_deletion_queue.push(m_BLASBuffer);

        m_vertexBuffer = nullptr;
        vBufferUploadHeap = nullptr;
    //    m_BLASBuffer = nullptr;

      CreateVertexBuffer(data);
        b_resetBLAS = true;
         /*  AccelerationStructureBuffers bottomLevelBuffers =
            Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateBottomLevelAS(
                {{m_vertexBuffer.Get(), m_num_verts}}, {{m_indexBuffer.Get(), m_num_indices}});

        m_BLASBuffer = bottomLevelBuffers.pResult;*/

        return;
    }

    // UINT back_index = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex();
    bool close = false;

     if (Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed)
    {
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->WaitForPreviousFrame();
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]++;

        auto commandAllocator = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandAllocators[back_index];

        commandAllocator->Reset();
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Reset(
            commandAllocator.Get(), Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_PipelineStateObject.Get());
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed = false;
        close = true;
    }


    DWORD count = data.size();
    int vBufferSize = sizeof(Vertex) * count;

    m_num_verts = count;

    {
      
        CD3DX12_RESOURCE_BARRIER transitionBarrier =

            CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                 D3D12_RESOURCE_STATE_COPY_DEST);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<const BYTE*>(data.data());
    vertexData.RowPitch = vBufferSize;
    vertexData.SlicePitch = vBufferSize;

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(),
                       m_vertexBuffer.Get(), vBufferUploadHeap.Get(), 0, 0, 1, &vertexData);

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =

            CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vBufferSize;

    b_resetBLAS = true;

    /* Renderer.GetResourceManager()->m_deletion_queue.push(m_BLASBuffer);

      m_BLASBuffer = nullptr;


        
      AccelerationStructureBuffers bottomLevelBuffers =
          Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateBottomLevelAS(
              {{m_vertexBuffer.Get(), m_num_verts}}, {{m_indexBuffer.Get(), m_num_indices}});

      m_BLASBuffer = bottomLevelBuffers.pResult;*/



    if (close)
    {
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Close();

        ID3D12CommandList* ppCommandLists[] = {
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get()};

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->ExecuteCommandLists(
            _countof(ppCommandLists), ppCommandLists);
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed = true;
        UINT back_index = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex();

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]++;

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->Signal(
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_Fence[back_index].Get(),
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]);
    }


}
void Mesh::SetIndices(std::vector<DWORD>& data)
{
    bool ok = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed;

    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();

    if (m_num_indices != data.size())
    {
        

        Renderer.GetResourceManager()->m_deletion_queue.push(m_indexBuffer);
        Renderer.GetResourceManager()->m_deletion_queue.push(iBufferUploadHeap);
      // Renderer.GetResourceManager()->m_deletion_queue.push(m_BLASBuffer);


        m_indexBuffer = nullptr;
        iBufferUploadHeap = nullptr;
       // m_BLASBuffer = nullptr;


        CreateIndexBuffer(data);

           b_resetBLAS = true;
        /*AccelerationStructureBuffers bottomLevelBuffers =
            Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateBottomLevelAS(
                {{m_vertexBuffer.Get(), m_num_verts}}, {{m_indexBuffer.Get(), m_num_indices}});

        m_BLASBuffer = bottomLevelBuffers.pResult*/;
       
        return;
    }
  

     bool close = false;

  

    int iBufferSize = sizeof(DWORD) * data.size();

    m_num_indices = data.size();

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =

            CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                 D3D12_RESOURCE_STATE_COPY_DEST);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    D3D12_SUBRESOURCE_DATA indexData = {};
    indexData.pData = reinterpret_cast<const BYTE*>(data.data());
    indexData.RowPitch = iBufferSize;
    indexData.SlicePitch = iBufferSize;

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(), m_indexBuffer.Get(),
                       iBufferUploadHeap.Get(), 0, 0, 1, &indexData);

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =

            CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_indexBufferView.SizeInBytes = iBufferSize;

    // Renderer.GetResourceManager()->m_deletion_queue.push(m_BLASBuffer);

    //m_BLASBuffer = nullptr;
    b_resetBLAS = true;
    //AccelerationStructureBuffers bottomLevelBuffers =
    //    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateBottomLevelAS(
    //        {{m_vertexBuffer.Get(), m_num_verts}}, {{m_indexBuffer.Get(), m_num_indices}});

    //m_BLASBuffer = bottomLevelBuffers.pResult;

}

void Mesh::SetAttribute(Attribute attribute, size_t count, void* data) {}

void Mesh::SetIndices(std::vector<uint16_t>& data) {}

void Mesh::CreateVertexBuffer(const std::vector<Vertex>& vertex_list)
{
    int vBufferSize = sizeof(Vertex) * vertex_list.size();

    m_num_verts = vertex_list.size();
    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
        m_vertexBuffer, L"Vertex Buffer Resource Heap", CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, nullptr);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&vBufferUploadHeap));
    vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<const BYTE*>(vertex_list.data());
    vertexData.RowPitch = vBufferSize;
    vertexData.SlicePitch = vBufferSize;

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(),
                       m_vertexBuffer.Get(), vBufferUploadHeap.Get(), 0, 0, 1, &vertexData);

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =
          
            CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    D3D12_DISCARD_REGION discardRegion = {};
    discardRegion.NumRects = 0;      // Setting NumRects to 0 discards the entire resource.
    discardRegion.pRects = nullptr;  // This is ignored when NumRects is 0.
    discardRegion.FirstSubresource = 0;
    discardRegion.NumSubresources = 1;
    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->DiscardResource(vBufferUploadHeap.Get(),
                                                                                                   &discardRegion);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vBufferSize;
}

void Mesh::CreateIndexBuffer(const std::vector<DWORD>& index_list)
{
    int iBufferSize = sizeof(DWORD) * index_list.size();

    m_num_indices = index_list.size();

    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
        m_indexBuffer, L"Index Buffer Resource Heap", CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, nullptr);

    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * m_num_verts);
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&iBufferUploadHeap));

        iBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");
    }

    D3D12_SUBRESOURCE_DATA indexData = {};
    indexData.pData = reinterpret_cast<const BYTE*>(index_list.data());
    indexData.RowPitch = iBufferSize;
    indexData.SlicePitch = iBufferSize;

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(), m_indexBuffer.Get(),
                       iBufferUploadHeap.Get(), 0, 0, 1, &indexData);

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

  
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_indexBufferView.SizeInBytes = iBufferSize;
}

static uint32_t CalculateDataTypeSize(tinygltf::Accessor const& accessor) noexcept
{
    uint32_t elementSize = 0;
    switch (accessor.componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            elementSize = 1;
            break;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            elementSize = 2;
            break;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
        case TINYGLTF_COMPONENT_TYPE_INT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            elementSize = 4;
            break;
        default:
            assert(false);
    }

    switch (accessor.type)
    {
        case TINYGLTF_TYPE_MAT2:
            return 4 * elementSize;
        case TINYGLTF_TYPE_MAT3:
            return 9 * elementSize;
        case TINYGLTF_TYPE_MAT4:
            return 16 * elementSize;
        case TINYGLTF_TYPE_SCALAR:
            return elementSize;
        case TINYGLTF_TYPE_VEC2:
            return 2 * elementSize;
        case TINYGLTF_TYPE_VEC3:
            return 3 * elementSize;
        case TINYGLTF_TYPE_VEC4:
            return 4 * elementSize;
        default:
            assert(false);
    }

    return 0;
}