#pragma once

#include <wrl.h>
using namespace Microsoft::WRL;

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "dx12/d3dx12.h"

#include <DirectXMath.h>
#include <dxcapi.h>

#include <algorithm>
#include <cassert>
#include <chrono>


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> 

#include <algorithm>
#include <cassert>
#include <chrono>

#include "platform/dx12/Helpers.hpp"
#include "dx12/DXRHelpers/nv_helpers_dx12/TopLevelASGenerator.h"
#include "dx12/DXRHelpers/DXRHelper.h"
#include "dx12/DXRHelpers/nv_helpers_dx12/BottomLevelASGenerator.h"
#include "dx12/DXRHelpers/nv_helpers_dx12/RaytracingPipelineGenerator.h"   
#include "dx12/DXRHelpers/nv_helpers_dx12/RootSignatureGenerator.h"

#include "dx12/DXRHelpers/nv_helpers_dx12/ShaderBindingTableGenerator.h"

 struct GLFWwindow;
struct GLFWmonitor;

 struct AccelerationStructureBuffers
{
    ComPtr<ID3D12Resource> pScratch;       // Scratch memory for AS builder
    ComPtr<ID3D12Resource> pResult;        // Where the AS is
    ComPtr<ID3D12Resource> pInstanceDesc;  // Hold the matrices of the instances
};


 #include "core/ecs.hpp"

 
namespace bee
{
class RenderPipeline;
}


class DeviceManager
{
public:
    friend class bee::RenderPipeline;
    friend class ResourceManager;

    ComPtr<ID3D12Device5> GetDevice();
    ComPtr<ID3D12GraphicsCommandList4> GetCommandList();
    ComPtr<ID3D12DescriptorHeap> m_ImGui_DescriptorHeap;
    void EndFrame();
  
 static bool m_resize ;

   void WaitForPreviousFrame();

 void Flush();


 IDxcBlob* CompileShaderLibrary(LPCWSTR fileName, LPCWSTR targetProfile);

   ComPtr<ID3D12CommandQueue> m_CommandQueue;

   static const uint8_t m_NumFrames = 3;

   ComPtr<ID3D12Fence> m_Fence[m_NumFrames];

   UINT64 m_FenceValue[m_NumFrames];

   HANDLE m_FenceEvent;

   UINT GetCurrentBufferIndex();
   ComPtr<ID3D12CommandAllocator> m_CommandAllocators[m_NumFrames];
   ComPtr<ID3D12PipelineState> m_PipelineStateObject;
   ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;

   
    ComPtr<ID3D12DescriptorHeap> m_DepthStencilDescriptorHeap;

   bool m_fullscreen=false;

    bool m_command_list_closed = false;
   ComPtr<ID3D12Resource> m_outputResource;

   ComPtr<ID3D12Resource> m_materialResource;

     ComPtr<ID3D12Resource> m_BackBuffers[m_NumFrames];


   ComPtr<ID3D12RootSignature> m_MipMapRootSignature;
   ComPtr<ID3D12PipelineState> m_MipMapPipelineStateObject;
    void SetFullscreen(bool fullscreen);

   private:  
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
   

    DeviceManager();
    ~DeviceManager();

    void FinishInit();

    void BeginFrame();

    void FlushBuffers();

    ComPtr<ID3D12Device5> CreateDevice(ComPtr<IDXGIAdapter4> adapter);

    ComPtr<IDXGISwapChain4> CreateSwapChain(ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height,
                                            uint32_t bufferCount);

    ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE type);

    ComPtr<ID3D12GraphicsCommandList4> CreateCommandList(ComPtr<ID3D12Device5> device,
                                                         ComPtr<ID3D12CommandAllocator> commandAllocator,
                                                         D3D12_COMMAND_LIST_TYPE type);

    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE type);

    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device5> device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                      D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t numDescriptors);

    void CreateCommandAllocators();

    ComPtr<ID3D12Resource> CreateRTVBuffer(ComPtr<ID3D12Device5> device, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
                                           DXGI_FORMAT format);

    void UpdateRenderTargetViews(ComPtr<ID3D12Device5> device, ComPtr<IDXGISwapChain4> swapChain,
                                 ComPtr<ID3D12DescriptorHeap> descriptorHeap);

    bool CheckTearingSupport();

    void EnableDebugLayer();

    void CreateRootSignature(ComPtr<ID3D12RootSignature>& root_signature, ComPtr<ID3D12PipelineState>& pso,
                             LPCWSTR vertexShader, LPCWSTR pixelShader, D3D12_ROOT_PARAMETER parms[], UINT numInputElems,
                             UINT numRenderTargets,
                             D3D12_CULL_MODE cull_mode);

    void Resize(uint32_t width, uint32_t height);

    void CheckRaytracingSupport();

    bool m_UseWarp = false;
    bool m_vsync = false;
    bool m_TearingSupported = false;

    ComPtr<ID3D12Device5> m_Device;

    ComPtr<IDXGISwapChain4> m_SwapChain;

    D3D12_VIEWPORT m_Viewport;

    D3D12_RECT m_ScissorRect;

    ComPtr<ID3D12GraphicsCommandList4> m_CommandList;

    UINT m_RTVDescriptorSize;

    UINT m_CurrentBackBufferIndex;

  
    ComPtr<ID3D12Resource> m_posBuffer;

    ComPtr<ID3D12Resource> m_normalBuffer;

    ComPtr<ID3D12Resource> m_colorBuffer;

    ComPtr<ID3D12Resource> m_materialBuffer;

    ComPtr<ID3D12Resource> m_shadowBuffer;

    ComPtr<ID3D12Resource> m_bloomBuffer;

    ComPtr<ID3D12RootSignature> m_RootSignature;

    ComPtr<ID3D12RootSignature> m_particleRootSignature;
    ComPtr<ID3D12PipelineState> m_particlePipelineStateObject;

    ComPtr<ID3D12RootSignature> m_volumetricRootSignature;
    ComPtr<ID3D12PipelineState> m_volumetricPipelineStateObject;


    ComPtr<ID3D12RootSignature> m_foliageRootSignature;
    ComPtr<ID3D12PipelineState> m_foliagePipelineStateObject;

     ComPtr<ID3D12RootSignature> m_terrainRootSignature;
    ComPtr<ID3D12PipelineState>  m_terrainPipelineStateObject;


    ComPtr<ID3D12RootSignature> m_DIRootSignature;
    ComPtr<ID3D12PipelineState> m_DIPipelineStateObject;

       ComPtr<ID3D12RootSignature> m_BloomCopyRootSignature;
    ComPtr<ID3D12PipelineState> m_BloomCopyPipelineStateObject;

     ComPtr<ID3D12RootSignature> m_BloomHRootSignature;
    ComPtr<ID3D12PipelineState> m_BloomHPipelineStateObject;

    ComPtr<ID3D12RootSignature> m_BloomVRootSignature;
    ComPtr<ID3D12PipelineState> m_BloomVPipelineStateObject;

    ComPtr<ID3D12RootSignature> m_ImageEffectsRootSignature;
    ComPtr<ID3D12PipelineState> m_ImageEffectsPipelineStateObject;


    ComPtr<ID3D12Resource> m_DepthStencilBuffer;

    ComPtr<ID3D12Resource> m_bottomLevelAS;

    nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
    nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator2;

    AccelerationStructureBuffers m_topLevelASBuffers1;
    AccelerationStructureBuffers m_topLevelASBuffers2;


    std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> m_instances;

    ComPtr<ID3D12RootSignature> CreateRayGenSignature();
    ComPtr<ID3D12RootSignature> CreateMissSignature();
    ComPtr<ID3D12RootSignature> CreateHitSignature();

    void CreateRaytracingPipeline();

    void CreateRaytracinuavTexturesBuffer();

    ComPtr<IDxcBlob> m_rayGenLibrary;

    ComPtr<IDxcBlob> m_aoLibrary;
    ComPtr<IDxcBlob> m_missLibrary;
    ComPtr<IDxcBlob> m_shadowLibrary;
    ComPtr<IDxcBlob> m_reflectionLibrary;

    ComPtr<ID3D12RootSignature> m_rayGenSignature;

    ComPtr<ID3D12RootSignature> m_missSignature;

    ComPtr<ID3D12RootSignature> m_reflectionSignature;
    ComPtr<ID3D12RootSignature> m_aoSignature;

    ComPtr<ID3D12StateObject> m_rtStateObject;

    ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;

    nv_helpers_dx12::ShaderBindingTableGenerator m_sbtHelper;
    ComPtr<ID3D12Resource> m_sbtStorage;

    ComPtr<ID3D12Resource> m_CameraBuffer;
    ComPtr<ID3D12DescriptorHeap> m_ConstHeap;
    uint32_t m_CameraBufferSize = 0;


    ComPtr<ID3D12Resource> m_vertexBuffer;

    ComPtr<ID3D12Resource> m_indexBuffer;
   
    ComPtr<ID3D12Resource> m_globalConstantBuffer;

    bool b_RTVsInitialized = true;

#ifdef STEAM_API_WINDOWS
    bool b_rayTracingEnabled = false;
#else
    bool b_rayTracingEnabled = true;
#endif


    int width, height;

    RECT m_WindowRect;
  
};
