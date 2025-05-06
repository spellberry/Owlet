#include "platform/dx12/image_dx12.h"

#include <tinygltf/stb_image.h>  // Implementation of stb_image is in gltf_loader.cpp

#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "core/resources.hpp"
// #include "platform/opengl/open_gl.hpp"
#include <dx12/d3dx12.h>

#include "platform/dx12/Renderpipeline.hpp"
#include "rendering/model.hpp"
#include "tools/log.hpp"
#include "tools/tools.hpp"
//
// #include <future>
// #include <unordered_map>
// #include <mutex>

using namespace bee;
using namespace std;

std::unordered_map<std::string, std::vector<char>> Image::imageCache;
std::mutex Image::cacheMutex;

Image::Image(const Model& model, int index) : Resource(ResourceType::Image)
{
    // UINT back_index = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex();
    // bool close = false;

    // if (Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed)
    //{
    //     Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->WaitForPreviousFrame();
    //     Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]++;

    //    auto commandAllocator = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandAllocators[back_index];

    //    commandAllocator->Reset();
    //    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Reset(
    //        commandAllocator.Get(), Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_PipelineStateObject.Get());
    //    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed = false;
    //    close = true;
    //}

    // const auto& image = model.GetDocument().images[index];
    // if (image.uri.empty())
    //{
    //     if (image.bufferView >= 0)
    //     {
    //         imageData = nullptr;
    //         const auto& view = model.GetDocument().bufferViews[image.bufferView];
    //         const auto& buffer = model.GetDocument().buffers[view.buffer];
    //         auto ptr = &buffer.data.at(view.byteOffset);
    //         imageData = stbi_load_from_memory(ptr, (int)buffer.data.size(), &m_width, &m_height, &m_channels, 4);

    //        //{
    //         //    std::lock_guard<std::mutex> lock(cacheMutex);
    //         //   imageCache[model.GetPath()] = buffer.data.at(view.byteOffset);  // Cache the buffer
    //         //}

    //         //// Load image asynchronously
    //         ////    loadFuture = std::async(std::launch::async, &Image::LoadImage, this, buffer);
    //         //loadFuture = std::async(std::launch::async,
    //         //                        [this, buffer]()
    //         //                        {
    //         //                            return stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.data()),
    //         //                                                         (int)buffer.size(), &m_width, &m_height, &m_channels, 4);
    //         //                        });


    //         if (imageData)
    //         {
    //            /*  CreateTextureResource(imageData, m_width, m_height, 4, true);
    //             stbi_image_free(imageData);*/
    //         }
    //         else
    //         {
    //             Log::Error("Image could not be loaded from a PNG file. Image:{} URI:{}", GetPath(model, index), image.uri);
    //         }
    //
    //     }
    //     else if (!image.image.empty())
    //     {
    //         m_width = image.width;
    //         m_height = image.height;
    //         m_channels = image.component;
    //         CreateTextureResource((BYTE*)image.image.data(), m_width, m_height, 4, true);
    //     }

    //}
    // else
    //{
    //    auto uri = model.GetPath();
    //    const auto lastSlashIdx = uri.rfind("/");
    //    uri = uri.substr(0, lastSlashIdx + 1);
    //    uri += image.uri;
    //   //  const string path = Engine.Resources().GetPath(uri);
    //    const auto buffer = Engine.FileIO().ReadBinaryFile(FileIO::Directory::Asset, uri);

    //    if (!buffer.empty())
    //    {
    //       /* BYTE* imageData = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.data()), (int)buffer.size(),
    //                                           &m_width, &m_height, &m_channels, 4);*/

    //          {
    //            std::lock_guard<std::mutex> lock(cacheMutex);
    //              imageCache[model.GetPath()] = buffer;  // Cache the buffer
    //        }

    //        // Load image asynchronously
    //        //    loadFuture = std::async(std::launch::async, &Image::LoadImage, this, buffer);
    //        loadFuture = std::async(std::launch::async,
    //                                [this, buffer]()
    //                                {
    //                                    return stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.data()),
    //                                                                 (int)buffer.size(), &m_width, &m_height, &m_channels, 4);
    //                                });

    //        if (imageData)
    //        {
    //        /*    CreateTextureResource(imageData, m_width, m_height, 4, true);
    //    stbi_image_free(imageData);*/
    //        }
    //        else
    //        {
    //            Log::Error("Image could not be loaded from a PNG file. Image:{} URI:{}", GetPath(model, index), image.uri);
    //        }
    //    }
    //    else
    //    {
    //        Log::Error("Image could not be loaded from a file. Image:{} URI:{}", GetPath(model, index), image.uri);
    //    }

    //

    //}

    // if (close)
    //{
    //     Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Close();

    //    ID3D12CommandList* ppCommandLists[] = {
    //        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get()};

    //    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->ExecuteCommandLists(
    //        _countof(ppCommandLists), ppCommandLists);
    //    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed = true;
    //    UINT back_index = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex();

    //    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]++;

    //    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->Signal(
    //        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_Fence[back_index].Get(),
    //        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_FenceValue[back_index]);
    //}

    // LabelGL(GL_TEXTURE, m_texture, m_path);
}
// Image::Image(const std::string& path) : Resource(ResourceType::Image)
//{
//     // const string fullpath = Engine.Resources().GetPath(FileIO::Directory::Asset, path);
//     const auto buffer = Engine.FileIO().ReadBinaryFile(FileIO::Directory::Asset, path);
//     imageData = nullptr;
//     if (!buffer.empty())
//     {
//         imageData = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.data()), (int)buffer.size(),
//         &m_width,
//                                            &m_height, &m_channels, 4);
//
//         if (!imageData)
//         {
//             Log::Error("Image could not be loaded from a PNG file. Image:{}", path);
//         }
//     }
//     else
//     {
//         Log::Error("Image could not be loaded from a file. Image:{}", path);
//     }
//
////    UploadGPU();
//
//}
void Image::WaitForLoad()
{
    if (loadFuture.valid())
    {
        imageData = loadFuture.get();
        if (imageData)
        {
            // UploadGPU(); // Call your GPU upload function here
        }
    }
}

Image::Image(const std::string& path) : Resource(ResourceType::Image), imageData(nullptr)
{
    const auto buffer = Engine.FileIO().ReadBinaryFile(FileIO::Directory::Asset, path);

    if (!buffer.empty())
    {
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            imageCache[path] = buffer;  // Cache the buffer
        }

        // Load image asynchronously
        //    loadFuture = std::async(std::launch::async, &Image::LoadImage, this, buffer);
        loadFuture = std::async(std::launch::async,
                                [this, buffer]()
                                {
                                    return stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.data()),
                                                                 (int)buffer.size(), &m_width, &m_height, &m_channels, 4);
                                });
    }
    else
    {
        Log::Error("Image could not be loaded from a file. Image:{}", path);
    }

    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->QueueImageLoading(this);
}
// Image::Image(const std::string& path) : Resource(ResourceType::Image)
//{
//     using namespace std::chrono;
//
//     auto start = high_resolution_clock::now();
//  //   std::cout << "start" << std::endl;
//     imageData = nullptr;
//     std::vector<char> buffer;
//     {
//         std::lock_guard<std::mutex> lock(cacheMutex);
//         auto it = imageCache.find(path);
//         if (it != imageCache.end())
//         {
//             buffer = it->second;
//         }
//         else
//         {
//             buffer = Engine.FileIO().ReadBinaryFile(FileIO::Directory::Asset, path);
//             if (!buffer.empty())
//             {
//                 imageCache[path] = buffer;
//             }
//         }
//     }
//
//     if (!buffer.empty())
//     {
//         auto read_time = high_resolution_clock::now();
//      //   std::cout << "read" << std::endl;
//      //   std::cout << "Time taken to read file: " << duration_cast<milliseconds>(read_time - start).count() << " ms"
//      //             << std::endl;
//
//         auto load_task = std::async(std::launch::async,
//                                     [&buffer, this]()
//                                     {
//                                         return stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buffer.data()),
//                                                                      (int)buffer.size(), &m_width, &m_height, &m_channels,
//                                                                      4);
//                                     });
//
//       imageData = load_task.get();
//
//         if (!imageData)
//         {
//             Log::Error("Image could not be loaded from a PNG file. Image:{}", path);
//         }
//     }
//     else
//     {
//         Log::Error("Image could not be loaded from a file. Image:{}", path);
//     }
// }

bool Image::UploadGPU()
{
    if (imageData)
    {
        CreateTextureResource(imageData, m_width, m_height, 4, true);

        stbi_image_free(imageData);
        /*       return true;*/
    }

    return false;
}

#include <iostream>
Image::~Image()
{
    //   if (m_texture) glDeleteTextures(1, &m_texture);
    //   std::cout << "ok";

    //  m_TextureBuffer->
}

string Image::GetPath(const Model& model, int index)
{
    const auto& image = model.GetDocument().images[index];
    return model.GetPath() + " | Texture-" + to_string(index) + ": " + image.name;
}

void Image::CreateTextureResource(const BYTE* data, int width, int height, int channels, bool genMipMaps)
{
   /*  bool execute =false;
      if (Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed)
     {
         Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->WaitForPreviousFrame();
         Engine.ECS()
             .GetSystem<RenderPipeline>()
             .GetDeviceManager()
             ->m_FenceValue[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]++;

         auto commandAllocator =
             Engine.ECS()
                 .GetSystem<RenderPipeline>()
                 .GetDeviceManager()
                 ->m_CommandAllocators[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()];

         commandAllocator->Reset();
         Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Reset(
             commandAllocator.Get(), Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_PipelineStateObject.Get());
         Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_command_list_closed = false;
         execute = true;
     }*/

    int imageBytesPerRow = width * 4;

    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    m_textureDesc = {};
    m_textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    m_textureDesc.Alignment = 0;
    m_textureDesc.Width = width;
    m_textureDesc.Height = height;
    m_textureDesc.DepthOrArraySize = 1;

    UINT maxDimension = std::max(width, height);

    m_mip_count = static_cast<UINT>(std::floor(std::log2(maxDimension))) + 1;
    // UINT mipLevels = 3;
    // m_textureDesc.MipLevels = 1;

    m_textureDesc.MipLevels = m_mip_count;
    m_textureDesc.Format = dxgiFormat;
    m_textureDesc.SampleDesc.Count = 1;
    m_textureDesc.SampleDesc.Quality = 0;
    // m_textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    m_textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    int imageSize = imageBytesPerRow * height;

    {
        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = data;
        textureData.RowPitch = imageBytesPerRow;
        textureData.SlicePitch = imageBytesPerRow * m_textureDesc.Height;

        if (imageSize <= 0)
        {
            throw std::exception("No image here");
        }
        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
            m_TextureBuffer, L"Texture Buffer Resource Heap", CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            CD3DX12_RESOURCE_DESC(m_textureDesc), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, nullptr);

        UINT64 textureUploadBufferSize;

        //  m_TextureBuffer->Release();
        /*    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->GetCopyableFootprints(
                &m_textureDesc, 0, 3, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);*/

        textureUploadBufferSize = GetRequiredIntermediateSize(m_TextureBuffer.Get(), 0, m_mip_count);

        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
            m_TextureBufferUploadHeap, L"Texture Buffer Upload  Resource Heap", CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr);

        UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(),
                           m_TextureBuffer.Get(), m_TextureBufferUploadHeap.Get(), 0, 0, 1, &textureData);
    }

    CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_TextureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(CB));

    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&constantBuffer));

    std::vector<ID3D12DescriptorHeap*> heaps = {
        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->m_mipMapHeap.Get()};

    for (UINT i = 1; i < m_mip_count; ++i)
    {
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Close();
        ID3D12CommandList* ppCommandLists[] = {
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get()};

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->ExecuteCommandLists(
            _countof(ppCommandLists), ppCommandLists);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->Signal(
            Engine.ECS()
                .GetSystem<RenderPipeline>()
                .GetDeviceManager()
                ->m_Fence[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]
                .Get(),
            Engine.ECS()
                .GetSystem<RenderPipeline>()
                .GetDeviceManager()
                ->m_FenceValue[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->WaitForPreviousFrame();




        Engine.ECS()
            .GetSystem<RenderPipeline>()
            .GetDeviceManager()
            ->m_FenceValue[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]++;

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Reset(
            Engine.ECS()
                .GetSystem<RenderPipeline>()
                .GetDeviceManager()
                ->m_CommandAllocators[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]
                .Get(),
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_MipMapPipelineStateObject.Get());

        //   // Bind mip level i-1 as SRV
        //   // Bind mip level i as UAV

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->SetComputeRootSignature(
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_MipMapRootSignature.Get());

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->SetPipelineState(
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_MipMapPipelineStateObject.Get());

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->SetDescriptorHeaps(
            static_cast<UINT>(heaps.size()), heaps.data());

        int mipWidth = max(1, width >> i);
        int mipHeight = max(1, height >> i);
        // float mipWidth = width;
        // float mipHeight = height;

        cbData.TexelSizeX = 1.0f / mipWidth;
        cbData.TexelSizeY = 1.0f / mipHeight;
       // cbData.mip = i;
        void* pData;
        CD3DX12_RANGE readRange(0, 0);
        constantBuffer->Map(0, &readRange, &pData);
        memcpy(pData, &cbData, sizeof(CB));
        constantBuffer->Unmap(0, nullptr);

        /*D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
       uavDesc.Format = m_TextureBuffer->GetDesc().Format;
       uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
       uavDesc.Texture2D.MipSlice = 1;

       Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->CreateUnorderedAccessView(
           m_TextureBuffer.Get(), nullptr, &uavDesc, descHandle);*/

        D3D12_CPU_DESCRIPTOR_HANDLE descHandle =
            Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->m_mipMapHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = m_TextureBuffer->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = i - 1;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->CreateShaderResourceView(
            m_TextureBuffer.Get(), &srvDesc, descHandle);

        descHandle.ptr +=
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = m_TextureBuffer->GetDesc().Format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = i;

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->CreateUnorderedAccessView(
            m_TextureBuffer.Get(), nullptr, &uavDesc, descHandle);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->SetComputeRoot32BitConstants(0, 3,
                                                                                                                    &cbData, 0);

        CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(
            Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->m_mipMapHeap->GetGPUDescriptorHandleForHeapStart(),
            0,
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->SetComputeRootDescriptorTable(1,
                                                                                                                     srvHandle);

        CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(
            Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->m_mipMapHeap->GetGPUDescriptorHandleForHeapStart(),
            1,
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->SetComputeRootDescriptorTable(2,
                                                                                                                     uavHandle);

        UINT dispatchX = max(1, (int)(mipWidth / 8.0f));
        UINT dispatchY = max(1, (int)(mipHeight / 8.0f));

        /*  D3D12_RESOURCE_BARRIER uavBarrier2 = {};
        uavBarrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
          uavBarrier2.UAV.pResource = m_TextureBuffer.Get();
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &uavBarrier2);*/

        if (dispatchX > 0 && dispatchY > 0)
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Dispatch(dispatchX, dispatchY, 1);

        // Transition the resource to the appropriate state if needed
    }

    //Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Close();
    //ID3D12CommandList* ppCommandLists[] = {Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get()};
    //Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists),
    //                                                                                                 ppCommandLists);

    //// Signal and wait for completion
    //auto deviceManager = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager();
    //auto fence = deviceManager->m_Fence[deviceManager->GetCurrentBufferIndex()].Get();
    //auto fenceValue = deviceManager->m_FenceValue[deviceManager->GetCurrentBufferIndex()];

    //deviceManager->m_CommandQueue->Signal(fence, fenceValue);
    //deviceManager->WaitForPreviousFrame();

    //deviceManager->m_FenceValue[deviceManager->GetCurrentBufferIndex()]++;

    /*  Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Close();

      Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists),
                                                                                                       ppCommandLists);

      Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->Signal(
          Engine.ECS()
              .GetSystem<RenderPipeline>()
              .GetDeviceManager()
              ->m_Fence[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]
              .Get(),
          Engine.ECS()
              .GetSystem<RenderPipeline>()
              .GetDeviceManager()
              ->m_FenceValue[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]);

      Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->WaitForPreviousFrame();

      Engine.ECS()
          .GetSystem<RenderPipeline>()
          .GetDeviceManager()
          ->m_FenceValue[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]++;

      Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Reset(
          Engine.ECS()
              .GetSystem<RenderPipeline>()
              .GetDeviceManager()
              ->m_CommandAllocators[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]
              .Get(),
          Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_MipMapPipelineStateObject.Get());*/

    /* Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Close();
     ID3D12CommandList* ppCommandLists[] =
     {Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get()};

     Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists),
                                                                                                      ppCommandLists);

     Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_CommandQueue->Signal(
         Engine.ECS()
             .GetSystem<RenderPipeline>()
             .GetDeviceManager()
             ->m_Fence[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]
             .Get(),
         Engine.ECS()
             .GetSystem<RenderPipeline>()
             .GetDeviceManager()
             ->m_FenceValue[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]);

     Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->WaitForPreviousFrame();

     Engine.ECS()
         .GetSystem<RenderPipeline>()
         .GetDeviceManager()
         ->m_FenceValue[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]++;

     Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->Reset(
         Engine.ECS()
             .GetSystem<RenderPipeline>()
             .GetDeviceManager()
             ->m_CommandAllocators[Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCurrentBufferIndex()]
             .Get(),
         Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_MipMapPipelineStateObject.Get());*/

    CD3DX12_RESOURCE_BARRIER transitionBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_TextureBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier2);

    m_texture = Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->UpdateTextureDescriptorHeap(this);

    /* if (execute)
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
     }*/
}