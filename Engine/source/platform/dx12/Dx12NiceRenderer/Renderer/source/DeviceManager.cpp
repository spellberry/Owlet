#include "platform/dx12/DeviceManager.hpp"

#include <dxgidebug.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx12.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/implot.h>

#include "core/fileio.hpp"
#include "core/input.hpp"
#include "platform/pc/core/device_pc.hpp"
// Resources used :
// https://www.braynzarsoft.net/viewtutorial/q16390-04-direct3d-12-drawing
// https://www.3dgep.com/learning-directx-12-1/#Input-Assembler_Stage

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include <vector>

#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "dxcapi.h"
bool DeviceManager::m_resize = false;
ComPtr<ID3D12RootSignature> DeviceManager::CreateRayGenSignature()
{
    nv_helpers_dx12::RootSignatureGenerator rsc;

    // nv_helpers_dx12::RootSignatureGenerator rsc;
    // rsc.AddHeapRangesParameter(
    //    {{0 /*u0*/, 1 /*1 descriptor */, 0 /*use the implicit register space 0*/,
    //      D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/, 0 /*heap slot where the UAV is defined*/},
    //     {0 /*t0*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/, 1},
    //     {0 /*b0*/, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Camera parameters*/, 2}});

    //  nv_helpers_dx12::RootSignatureGenerator rsc;
    //  rsc.AddHeapRangesParameter({{0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0}, {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    //  1}});

    //     D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0 /*heap slot where the UAV is defined*/
    //}
    //,
    //
    //    {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/, 2},
    //{
    //    0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV /*Camera parameters*/, 3
    //}
    //});

    rsc.AddHeapRangesParameter({{0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0},

                                {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7},
                                {0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 8}

    });

    //  rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 1);

    return rsc.Generate(m_Device.Get(), true);
}

ComPtr<ID3D12RootSignature> DeviceManager::CreateHitSignature()
{
    nv_helpers_dx12::RootSignatureGenerator rsc;

    rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0);

    rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1);

    rsc.AddHeapRangesParameter({// {2, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5},
                                {3, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 7},
                                {2, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8}});

    rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0);

    rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 1);

    return rsc.Generate(m_Device.Get(), true);
}

ComPtr<ID3D12RootSignature> DeviceManager::CreateMissSignature()
{
    nv_helpers_dx12::RootSignatureGenerator rsc;
    return rsc.Generate(m_Device.Get(), true);
}

IDxcBlob* DeviceManager::CompileShaderLibrary(LPCWSTR fileName, LPCWSTR targetProfile)
{
    static IDxcCompiler* pCompiler = nullptr;
    static IDxcLibrary* pLibrary = nullptr;
    static IDxcIncludeHandler* dxcIncludeHandler;

    HRESULT hr;

    if (!pCompiler)
    {
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)&pCompiler));
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary));
        ThrowIfFailed(pLibrary->CreateIncludeHandler(&dxcIncludeHandler));
    }

    std::ifstream shaderFile(fileName);
    if (!shaderFile.good())
    {
        throw std::logic_error("Cannot find shader file");
    }
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    std::string sShader = strStream.str();

    IDxcBlobEncoding* pTextBlob;
    ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)sShader.c_str(), (uint32_t)sShader.size(), 0, &pTextBlob));

    IDxcOperationResult* pResult;

    //-------------This for debug data I guess 
   /* ThrowIfFailed(
        pCompiler->Compile(pTextBlob, fileName, L"main", targetProfile, nullptr, 0, nullptr, 0, dxcIncludeHandler, &pResult));*/


   //  Compiler arguments
    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-O3");            
    arguments.push_back(L"-Ges");            
    arguments.push_back(L"-Qstrip_reflect"); 


    ThrowIfFailed(pCompiler->Compile(pTextBlob, fileName, L"main", targetProfile, arguments.data(), (uint32_t)arguments.size(),
                                     nullptr, 0, dxcIncludeHandler, &pResult));

    HRESULT resultCode;
    ThrowIfFailed(pResult->GetStatus(&resultCode));
    if (FAILED(resultCode))
    {
        IDxcBlobEncoding* pError;
        hr = pResult->GetErrorBuffer(&pError);
        if (FAILED(hr))
        {
            throw std::logic_error("Failed to get shader compiler error");
        }

        std::vector<char> infoLog(pError->GetBufferSize() + 1);
        memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
        infoLog[pError->GetBufferSize()] = 0;

        std::string errorMsg = "Shader Compiler Error:\n";
        errorMsg.append(infoLog.data());

        MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
        throw std::logic_error("Failed compile shader");
    }

    IDxcBlob* pBlob;
    ThrowIfFailed(pResult->GetResult(&pBlob));
    return pBlob;
}

void DeviceManager::CreateRaytracingPipeline()
{
    nv_helpers_dx12::RayTracingPipelineGenerator pipeline(m_Device.Get());

    m_rayGenLibrary = CompileShaderLibrary(L"assets/shaders/HybridRayGen.hlsl", L"lib_6_3");
    //  m_missLibrary = CompileShaderLibrary(L"external/Dx12NiceRenderer/Renderer/Miss.hlsl");
    // m_hitLibrary = CompileShaderLibrary(L"external/Dx12NiceRenderer/Renderer/Hit.hlsl");
    m_aoLibrary = CompileShaderLibrary(L"assets/shaders/AOHit.hlsl",L"lib_6_3");
    m_shadowLibrary = CompileShaderLibrary(L"assets/shaders/Shadow.hlsl",L"lib_6_3");
   // m_reflectionLibrary = CompileShaderLibrary(L"assets/shaders/Reflection.hlsl",L"lib_6_3");

    pipeline.AddLibrary(m_rayGenLibrary.Get(), {L"RayGen"});
    //  pipeline.AddLibrary(m_missLibrary.Get(), {L"Miss"});

    pipeline.AddLibrary(m_shadowLibrary.Get(), {L"ShadowMiss"});
   // pipeline.AddLibrary(m_reflectionLibrary.Get(), {L"ReflectionClosestHit", L"ReflectionMiss"});
    pipeline.AddLibrary(m_aoLibrary.Get(), {L"AOClosestHit", L"AOMiss"});

    m_rayGenSignature = CreateRayGenSignature();
    m_missSignature = CreateMissSignature();
    m_aoSignature = CreateMissSignature();  // Calling miss signature generation on purpose because I was too lazy to write
                                            // another similar function

    //   m_shadowSignature = CreateHitSignature();
  //  m_reflectionSignature = CreateHitSignature();

    //   pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");

    //   pipeline.AddHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");

   // pipeline.AddHitGroup(L"ReflectionHitGroup", L"ReflectionClosestHit");
    pipeline.AddHitGroup(L"AOHitGroup", L"AOClosestHit");

    pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), {L"RayGen"});
    //  pipeline.AddRootSignatureAssociation(m_missSignature.Get(), {L"Miss"});
    // pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), {L"HitGroup"});

    pipeline.AddRootSignatureAssociation(m_missSignature.Get(), {L"ShadowMiss", L"AOMiss"});

    //  pipeline.AddRootSignatureAssociation(m_shadowSignature.Get(), {L"ShadowHitGroup"});

  //  pipeline.AddRootSignatureAssociation(m_reflectionSignature.Get(), {L"ReflectionHitGroup"});

    pipeline.AddRootSignatureAssociation(m_aoSignature.Get(), {L"AOHitGroup"});

    pipeline.SetMaxPayloadSize(4 * sizeof(float));

    pipeline.SetMaxAttributeSize(2 * sizeof(float));

    pipeline.SetMaxRecursionDepth(2);

    // m_rtStateObject =
    m_rtStateObject = pipeline.Generate();

    ThrowIfFailed(m_rtStateObject->QueryInterface(IID_PPV_ARGS(&m_rtStateObjectProps)));
}
void DeviceManager::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Resize(width, height);
    // std::cout << bee::Engine.Device().GetWidth() << " " << bee::Engine.Device().GetHeight() << std::endl;
    // Assuming you have access to your DX12 renderer instance here
    // renderer->Resize(width, height);
    bee::Engine.Device().SetWidth(width);
    bee::Engine.Device().SetHeight(height);
    m_resize = true;
}
void DeviceManager::CreateRootSignature(ComPtr<ID3D12RootSignature>& root_signature, ComPtr<ID3D12PipelineState>& pso,
                                        LPCWSTR vertexShader, LPCWSTR pixelShader, D3D12_ROOT_PARAMETER parms[],
                                        UINT numInputElems, UINT numRenderTargets, D3D12_CULL_MODE cull_mode)
{
    D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};

    // std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
    //
    // inputLayout.push_back({"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
    //                        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
    // inputLayout.push_back(
    //     {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
    //     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
    // inputLayout.push_back(
    //     {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
    //     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
    // inputLayout.push_back({"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
    //                        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
    // inputLayout.push_back({"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
    //                        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
    //
    //  inputLayout.push_back({"JOINT_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
    //                        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
    // inputLayout.push_back({"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
    //                        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});

//std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
//
//inputLayout.push_back({"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//                       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back(
//    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back(
//    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//                       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//                       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//
// inputLayout.push_back({"JOINT_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//                       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
//                       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});

//std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
//
//inputLayout.push_back({"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"JOINT_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
//inputLayout.push_back({"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 72, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});


  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
     0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
     0},
    {"JOINT_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
     0},
    {"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
     0}};

   D3D12_INPUT_ELEMENT_DESC inputLayout2[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
    //  {"MATERIALINDEX", 0, DXGI_FORMAT_R32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}

      {"MATERIAL_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
      };


     //D3D12_INPUT_ELEMENT_DESC inputLayout2[] = {
     // {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
     // {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
     // {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
     // {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
     // {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
     // {"JOINT_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
     // {"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 72, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
  int LayoutSize;

    if (cull_mode != D3D12_CULL_MODE_NONE)
{
        LayoutSize = 7;
    if ( numRenderTargets == 1)
    {
        LayoutSize = 5;
    }

        staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;  // Trilinear filtering

    
        {
            staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    
        staticSamplerDesc.MipLODBias = 0.0f;
        staticSamplerDesc.MaxAnisotropy = 1;
        staticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSamplerDesc.MinLOD = 0.0f;
        staticSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplerDesc.ShaderRegister = 0;  // Register t0
        staticSamplerDesc.RegisterSpace = 0;
        staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }
    else
    {
        LayoutSize = 5;
       // if (vertexShader != L"assets/shaders/TerrainVertexShader.hlsl" && numRenderTargets!=1)
        {
            /*  inputLayout.push_back({"JOINT_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});
              inputLayout.push_back({"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0});*/
        }
        staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;  // Trilinear filtering
        staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplerDesc.MipLODBias = 0.0f;
        staticSamplerDesc.MaxAnisotropy = 1;
        staticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSamplerDesc.MinLOD = 0.0f;
        staticSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplerDesc.ShaderRegister = 0;  // Register t0
        staticSamplerDesc.RegisterSpace = 0;
        staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(numInputElems, parms, 1, &staticSamplerDesc,
                           D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                               D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                               D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                               D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

    ID3DBlob* errorBuff;
    ID3DBlob* signature;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff));

    ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                IID_PPV_ARGS(&root_signature)));

    ComPtr<IDxcBlob> vertexShaderBlob;

    vertexShaderBlob = CompileShaderLibrary(vertexShader, L"vs_6_0");

    D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
    vertexShaderBytecode.BytecodeLength = vertexShaderBlob->GetBufferSize();
    vertexShaderBytecode.pShaderBytecode = vertexShaderBlob->GetBufferPointer();

    ComPtr<IDxcBlob> pixelShaderBlob;

    pixelShaderBlob = CompileShaderLibrary(pixelShader, L"ps_6_0");

    D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
    pixelShaderBytecode.BytecodeLength = pixelShaderBlob->GetBufferSize();
    pixelShaderBytecode.pShaderBytecode = pixelShaderBlob->GetBufferPointer();

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};


    if (vertexShader != L"assets/shaders/TerrainVertexShader.hlsl")
    {
        inputLayoutDesc.NumElements = LayoutSize;  // sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputLayout;
    }
    else
    {
        inputLayoutDesc.NumElements =  sizeof(inputLayout2) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputLayout2;

    }

    //---------------------------- Pipeline State Object

    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;  // Solid filling
    rasterizerDesc.CullMode = cull_mode;              // Cull back-facing triangles
    rasterizerDesc.FrontCounterClockwise = TRUE;      // Define front-facing triangles as clockwise
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = TRUE;
    /* blendDesc.RenderTarget[0].BlendEnable = TRUE;
     blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
     blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
     blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
     blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
     blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
     blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
     blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;*/

    /*  blendDesc.RenderTarget[0].BlendEnable = TRUE;
      blendDesc.RenderTarget[0].SrcBlend = blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
      blendDesc.RenderTarget[0].DestBlend = blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
      blendDesc.RenderTarget[0].BlendOp = blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
      blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;*/

    if (numRenderTargets == 4)
    {
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        blendDesc.RenderTarget[1].BlendEnable = FALSE;
        blendDesc.RenderTarget[1].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[1].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[1].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[1].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[1].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[1].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        blendDesc.RenderTarget[2].BlendEnable = TRUE;
        blendDesc.RenderTarget[2].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[2].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;  // Use inverse source alpha
        blendDesc.RenderTarget[2].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[2].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[2].DestBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[2].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[2].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        blendDesc.RenderTarget[3].BlendEnable = FALSE;
        blendDesc.RenderTarget[3].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[3].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[3].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[3].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[3].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[3].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[3].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    else
    {
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = FALSE;
    depthStencilDesc.StencilEnable = FALSE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.pRootSignature = root_signature.Get();
    psoDesc.VS = vertexShaderBytecode;
    psoDesc.PS = pixelShaderBytecode;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.BlendState = blendDesc;
    psoDesc.SampleDesc = {1, 0};
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    {
        psoDesc.NumRenderTargets = numRenderTargets;
        if (numRenderTargets == 4)
        {
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
            psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
            psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.RTVFormats[3] = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        else
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}
DeviceManager::DeviceManager()
{
    glfwSetFramebufferSizeCallback(bee::Engine.Device().GetWindow(), &framebuffer_size_callback);

    EnableDebugLayer();

    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(m_UseWarp);

    m_Device = CreateDevice(dxgiAdapter4);

    m_CommandQueue = CreateCommandQueue(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain =
        CreateSwapChain(m_CommandQueue, bee::Engine.Device().GetWidth(), bee::Engine.Device().GetHeight(), m_NumFrames);

    m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

    m_RTVDescriptorHeap =
        CreateDescriptorHeap(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, m_NumFrames + 5);

    m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews(m_Device, m_SwapChain, m_RTVDescriptorHeap);

    CreateCommandAllocators();
    m_CommandList = CreateCommandList(m_Device, m_CommandAllocators[m_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_list_closed = false;
    for (int i = 0; i < m_NumFrames; i++)
    {
        ComPtr<ID3D12Fence> fence;
        ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        m_Fence[i] = fence;

        m_FenceValue[i] = 0;
    }

    HANDLE fenceEvent;
    fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent && "Failed to create fence event.");
    m_FenceEvent = fenceEvent;

    //--------------------Root Signature
    {
        D3D12_ROOT_DESCRIPTOR rootCBVDescriptor1;
        rootCBVDescriptor1.RegisterSpace = 0;
        rootCBVDescriptor1.ShaderRegister = 0;

        D3D12_ROOT_DESCRIPTOR rootSRVDescriptor2;
        rootSRVDescriptor2.RegisterSpace = 0;
        rootSRVDescriptor2.ShaderRegister = 1;

        D3D12_ROOT_DESCRIPTOR rootSRVDescriptor;
        rootSRVDescriptor.RegisterSpace = 0;
        rootSRVDescriptor.ShaderRegister = 0;

        D3D12_ROOT_DESCRIPTOR rootSRVDescriptor1;
        rootSRVDescriptor1.RegisterSpace = 0;
        rootSRVDescriptor1.ShaderRegister = 3;

        D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];
        descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptorTableRanges[0].NumDescriptors = 100;
        descriptorTableRanges[0].BaseShaderRegister = 4;
        descriptorTableRanges[0].RegisterSpace = 0;
        descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // Create a descriptor table
        D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
        descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
        descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

        D3D12_ROOT_PARAMETER rootParameters[5];

        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor = rootCBVDescriptor1;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[1].Descriptor = rootSRVDescriptor2;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[2].Descriptor = rootSRVDescriptor;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[3].Descriptor = rootSRVDescriptor1;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[4].DescriptorTable = descriptorTable;
        rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CreateRootSignature(m_RootSignature, m_PipelineStateObject, L"assets/shaders/VertexShader.hlsl",
                            L"assets/shaders/HybridPixelShader.hlsl", rootParameters, 5, 4, D3D12_CULL_MODE_BACK);

        CreateRootSignature(m_particleRootSignature, m_particlePipelineStateObject, L"assets/shaders/ParticleVertexShader.hlsl",
                            L"assets/shaders/ParticleHybridPixelShader.hlsl", rootParameters, 5, 1, D3D12_CULL_MODE_BACK);

        CreateRootSignature(m_terrainRootSignature, m_terrainPipelineStateObject, L"assets/shaders/TerrainVertexShader.hlsl",
                            L"assets/shaders/TerrainPixelShader.hlsl", rootParameters, 5, 4, D3D12_CULL_MODE_BACK);
    }
    {
        D3D12_ROOT_DESCRIPTOR rootCBVDescriptor1;
        rootCBVDescriptor1.RegisterSpace = 0;
        rootCBVDescriptor1.ShaderRegister = 0;

        D3D12_ROOT_DESCRIPTOR rootSRVDescriptor2;
        rootSRVDescriptor2.RegisterSpace = 0;
        rootSRVDescriptor2.ShaderRegister = 1;

        D3D12_ROOT_DESCRIPTOR rootSRVDescriptor;
        rootSRVDescriptor.RegisterSpace = 0;
        rootSRVDescriptor.ShaderRegister = 0;

        /*D3D12_ROOT_DESCRIPTOR rootSRVDescriptor1;
        rootSRVDescriptor1.RegisterSpace = 0;
        rootSRVDescriptor1.ShaderRegister = 3;*/

        D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];
        descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptorTableRanges[0].NumDescriptors = 100;
        descriptorTableRanges[0].BaseShaderRegister = 4;
        descriptorTableRanges[0].RegisterSpace = 0;
        descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // Create a descriptor table
        D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
        descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
        descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

        D3D12_ROOT_PARAMETER rootParameters[5];

        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor = rootCBVDescriptor1;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[1].Descriptor = rootSRVDescriptor2;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[2].Descriptor = rootSRVDescriptor;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        /*   rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
           rootParameters[3].Descriptor = rootSRVDescriptor1;
           rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;*/

        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[3].Constants.Num32BitValues = 1;
        rootParameters[3].Constants.RegisterSpace = 0;
        rootParameters[3].Constants.ShaderRegister = 1;

        rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[4].DescriptorTable = descriptorTable;
        rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CreateRootSignature(m_foliageRootSignature, m_foliagePipelineStateObject, L"assets/shaders/FoliageVertexShader.hlsl",
                            L"assets/shaders/FoliagePixelShader.hlsl", rootParameters, 5, 4, D3D12_CULL_MODE_NONE);
    }

    {
        D3D12_ROOT_DESCRIPTOR rootCBVDescriptor1;
        rootCBVDescriptor1.RegisterSpace = 0;
        rootCBVDescriptor1.ShaderRegister = 0;

        D3D12_ROOT_DESCRIPTOR rootSRVDescriptor2;
        rootSRVDescriptor2.RegisterSpace = 0;
        rootSRVDescriptor2.ShaderRegister = 1;

        D3D12_ROOT_DESCRIPTOR rootSRVDescriptor;
        rootSRVDescriptor.RegisterSpace = 0;
        rootSRVDescriptor.ShaderRegister = 0;

        D3D12_ROOT_DESCRIPTOR rootSRVDescriptor1;
        rootSRVDescriptor1.RegisterSpace = 0;
        rootSRVDescriptor1.ShaderRegister = 3;

        D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];
        descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descriptorTableRanges[0].NumDescriptors = 1;
        descriptorTableRanges[0].BaseShaderRegister = 4;
        descriptorTableRanges[0].RegisterSpace = 0;
        descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // Create a descriptor table
        D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
        descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
        descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

        D3D12_ROOT_PARAMETER rootParameters[5];

        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor = rootCBVDescriptor1;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[1].Descriptor = rootSRVDescriptor2;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[2].Descriptor = rootSRVDescriptor;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[3].Descriptor = rootSRVDescriptor1;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[4].DescriptorTable = descriptorTable;
        rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler,
                               D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                   D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                   D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                   D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

        ID3DBlob* errorBuff;
        ID3DBlob* signature;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff));

        ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                                    IID_PPV_ARGS(&m_volumetricRootSignature)));

        //-----------------------Shaders
        ComPtr<IDxcBlob> vertexShader;

        vertexShader = CompileShaderLibrary(L"assets/shaders/VertexShader.hlsl", L"vs_6_0");

        D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
        vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
        vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

        ComPtr<IDxcBlob> pixelShader;

        pixelShader = CompileShaderLibrary(L"assets/shaders/VolumetricPixelShader.hlsl", L"ps_6_0");

        D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
        pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
        pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"JOINT_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 72, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

        inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputLayout;

        //---------------------------- Pipeline State Object

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = inputLayoutDesc;
        psoDesc.pRootSignature = m_RootSignature.Get();
        psoDesc.VS = vertexShaderBytecode;
        psoDesc.PS = pixelShaderBytecode;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc = {1, 0};
        psoDesc.SampleMask = 0xffffffff;
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.NumRenderTargets = 1;
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_volumetricPipelineStateObject)));
    }

    //-------Compute Shader root signatures

    CD3DX12_DESCRIPTOR_RANGE1 myTextureUAV(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
                                           D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    /*CD3DX12_DESCRIPTOR_RANGE1 myTextureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
                                           D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);*/

    //  CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {myTextureUAV, myTextureSRV};

    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {myTextureUAV};
        CD3DX12_ROOT_PARAMETER1 rootParameters[3];

        rootParameters[0].InitAsConstantBufferView(0, 0);
        rootParameters[1].InitAsShaderResourceView(0, 0);
        rootParameters[2].InitAsDescriptorTable(_countof(ranges), ranges);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(3, rootParameters, 0, nullptr);

        // Serialize the root signature.
        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSignatureBlob,
                                              &errorBlob);

        m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_DIRootSignature));

        Microsoft::WRL::ComPtr<IDxcBlob> computeShader;

        /*::D3DReadFileToBlob(
                L"Dx12NiceRenderer\\Renderer\\ComputeShader.hlsl",
                &computeShader
        );*/
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob1;

        computeShader = CompileShaderLibrary(L"assets/shaders/DirectIlluminationComputeShader.hlsl", L"cs_6_0");

        /* ThrowIfFailed(D3DCompileFromFile(L"Dx12NiceRenderer\\Renderer\\DirectIlluminationComputeShader.hlsl", nullptr,
           nullptr, "main", "cs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &computeShader, &errorBlob1));*/

        D3D12_SHADER_BYTECODE computeShaderBytecode = {};
        computeShaderBytecode.BytecodeLength = computeShader->GetBufferSize();
        computeShaderBytecode.pShaderBytecode = computeShader->GetBufferPointer();

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_CS CS;
        } pipelineStateStream;

        // Setting the root signature and the compute shader to the PSO
        pipelineStateStream.pRootSignature = m_DIRootSignature.Get();
        pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBytecode);

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(PipelineStateStream), &pipelineStateStream};

        ThrowIfFailed(m_Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_DIPipelineStateObject)));
    }

    {
        CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
        CD3DX12_ROOT_PARAMETER rootParameters[3];
        srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
        srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
        rootParameters[0].InitAsConstants(2, 0);
        rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
        rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

        /*   CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
          CD3DX12_ROOT_PARAMETER rootParameters[2];
           srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
          srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
          rootParameters[0].InitAsDescriptorTable(1, &srvCbvRanges[0]);
          rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[1]);*/

        D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.MaxAnisotropy = 0;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        samplerDesc.ShaderRegister = 0;
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        Microsoft::WRL::ComPtr<ID3DBlob> signature;

        Microsoft::WRL::ComPtr<ID3DBlob> error;
        /* CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
         rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc,
                                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);*/

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 1, &samplerDesc);

        D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);

        m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                      IID_PPV_ARGS(&m_MipMapRootSignature));

        Microsoft::WRL::ComPtr<IDxcBlob> computeShader;

        computeShader = CompileShaderLibrary(L"assets/shaders/MipMapComputeShader.hlsl", L"cs_6_0");

        D3D12_SHADER_BYTECODE computeShaderBytecode = {};
        computeShaderBytecode.BytecodeLength = computeShader->GetBufferSize();
        computeShaderBytecode.pShaderBytecode = computeShader->GetBufferPointer();

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_CS CS;
        } pipelineStateStream;

        // Setting the root signature and the compute shader to the PSO
        pipelineStateStream.pRootSignature = m_MipMapRootSignature.Get();
        pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBytecode);

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(PipelineStateStream), &pipelineStateStream};

        ThrowIfFailed(m_Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_MipMapPipelineStateObject)));
    }

    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {myTextureUAV};
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];

        rootParameters[0].InitAsConstantBufferView(0, 0);
        rootParameters[1].InitAsDescriptorTable(_countof(ranges), ranges);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(2, rootParameters, 0, nullptr);

        // Serialize the root signature.
        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSignatureBlob,
                                              &errorBlob);

        m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_BloomHRootSignature));

        m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_BloomVRootSignature));

        Microsoft::WRL::ComPtr<IDxcBlob> computeShader;

        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob1;

        computeShader = CompileShaderLibrary(L"assets/shaders/BloomHorizontalComputeShader.hlsl", L"cs_6_0");

        D3D12_SHADER_BYTECODE computeShaderBytecode = {};
        computeShaderBytecode.BytecodeLength = computeShader->GetBufferSize();
        computeShaderBytecode.pShaderBytecode = computeShader->GetBufferPointer();

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_CS CS;
        } pipelineStateStream;

        // Setting the root signature and the compute shader to the PSO
        pipelineStateStream.pRootSignature = m_BloomHRootSignature.Get();
        pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBytecode);

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(PipelineStateStream), &pipelineStateStream};

        ThrowIfFailed(m_Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_BloomHPipelineStateObject)));
    }

    {
        Microsoft::WRL::ComPtr<IDxcBlob> computeShader;

        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob1;

        computeShader = CompileShaderLibrary(L"assets/shaders/BloomVerticalComputeShader.hlsl", L"cs_6_0");

        D3D12_SHADER_BYTECODE computeShaderBytecode = {};
        computeShaderBytecode.BytecodeLength = computeShader->GetBufferSize();
        computeShaderBytecode.pShaderBytecode = computeShader->GetBufferPointer();

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_CS CS;
        } pipelineStateStream;

        // Setting the root signature and the compute shader to the PSO
        pipelineStateStream.pRootSignature = m_BloomVRootSignature.Get();
        pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBytecode);

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(PipelineStateStream), &pipelineStateStream};

        ThrowIfFailed(m_Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_BloomVPipelineStateObject)));
    }

    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {myTextureUAV};
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];

        rootParameters[0].InitAsConstantBufferView(0, 0);
        rootParameters[1].InitAsDescriptorTable(_countof(ranges), ranges);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(2, rootParameters, 0, nullptr);

        // Serialize the root signature.
        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSignatureBlob,
                                              &errorBlob);

        m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_BloomCopyRootSignature));

        Microsoft::WRL::ComPtr<IDxcBlob> computeShader;

        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob1;

        computeShader = CompileShaderLibrary(L"assets/shaders/BloomCopyComputeShader.hlsl", L"cs_6_0");

        D3D12_SHADER_BYTECODE computeShaderBytecode = {};
        computeShaderBytecode.BytecodeLength = computeShader->GetBufferSize();
        computeShaderBytecode.pShaderBytecode = computeShader->GetBufferPointer();

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_CS CS;
        } pipelineStateStream;

        // Setting the root signature and the compute shader to the PSO
        pipelineStateStream.pRootSignature = m_BloomCopyRootSignature.Get();
        pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBytecode);

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(PipelineStateStream), &pipelineStateStream};

        ThrowIfFailed(m_Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_BloomCopyPipelineStateObject)));
    }

    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {myTextureUAV};
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];

        // rootParameters[0].InitAsConstantBufferView(0, 0);
        //  rootParameters[1].InitAsShaderResourceView(0, 0);
        rootParameters[0].InitAsConstantBufferView(0, 0);
        rootParameters[1].InitAsDescriptorTable(_countof(ranges), ranges);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(2, rootParameters, 0, nullptr);

        // Serialize the root signature.
        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSignatureBlob,
                                              &errorBlob);

        m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_ImageEffectsRootSignature));
        Microsoft::WRL::ComPtr<IDxcBlob> computeShader;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob1;

        computeShader = CompileShaderLibrary(L"assets/shaders/ImageEffectsShader.hlsl", L"cs_6_0");

        D3D12_SHADER_BYTECODE computeShaderBytecode = {};
        computeShaderBytecode.BytecodeLength = computeShader->GetBufferSize();
        computeShaderBytecode.pShaderBytecode = computeShader->GetBufferPointer();

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_CS CS;
        } pipelineStateStream;

        // Setting the root signature and the compute shader to the PSO
        pipelineStateStream.pRootSignature = m_ImageEffectsRootSignature.Get();
        pipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(computeShaderBytecode);

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(PipelineStateStream), &pipelineStateStream};

        ThrowIfFailed(
            m_Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_ImageEffectsPipelineStateObject)));
    }

    //---------------------------------------------Depth/Stencil
    m_DepthStencilDescriptorHeap =
        CreateDescriptorHeap(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, bee::Engine.Device().GetWidth(), bee::Engine.Device().GetHeight(), 1, 0, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

        m_Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                          &depthOptimizedClearValue, IID_PPV_ARGS(&m_DepthStencilBuffer));
    }

    m_DepthStencilDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

    m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &depthStencilDesc,
                                     m_DepthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    CheckRaytracingSupport();
    if (b_rayTracingEnabled)
    {
         
        //   CreateRaytracinuavTexturesBuffer();
        CreateRaytracingPipeline();
    }

    //    // Create our buffer
    //    m_globalConstantBuffer =
    //        nv_helpers_dx12::CreateBuffer(m_Device.Get(), sizeof(bufferData), D3D12_RESOURCE_FLAG_NONE,
    //                                      D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

    //    // Copy CPU memory to GPU
    //    uint8_t* pData;
    //    ThrowIfFailed(m_globalConstantBuffer->Map(0, nullptr, (void**)&pData));
    //    memcpy(pData, bufferData, sizeof(bufferData));
    //    m_globalConstantBuffer->Unmap(0, nullptr);

    //}

    CD3DX12_RESOURCE_BARRIER barrierrr9 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    {
        D3D12_RESOURCE_BARRIER barriers[1] = {barrierrr9};

        m_CommandList->ResourceBarrier(1, barriers);
    }
#ifdef BEE_INSPECTOR
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 7;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_ImGui_DescriptorHeap));
        ;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE Handle(m_ImGui_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ImGui_ImplDX12_Init(m_Device.Get(), 3, DXGI_FORMAT_R8G8B8A8_UNORM, m_ImGui_DescriptorHeap.Get(),
                        m_ImGui_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                        m_ImGui_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    auto rtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    Handle.Offset(rtvDescriptorSize);

    m_Device->CreateShaderResourceView(m_outputResource.Get(), &srvDesc, Handle);
    Handle.Offset(rtvDescriptorSize);
#endif

    /* m_Device->CreateShaderResourceView(m_materialResource.Get(), &srvDesc, Handle);
     Handle.Offset(rtvDescriptorSize);*/

    /* m_Device->CreateShaderResourceView(m_posBuffer[1].Get(), &srvDesc, Handle);
     Handle.Offset(rtvDescriptorSize);
     m_Device->CreateShaderResourceView(m_posBuffer[2].Get(), &srvDesc, Handle);*/

    // CD3DX12_RESOURCE_BARRIER barrierrr10 = CD3DX12_RESOURCE_BARRIER::Transition(
    //     m_outputResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    ///* CD3DX12_RESOURCE_BARRIER barrierrr11 = CD3DX12_RESOURCE_BARRIER::Transition(
    //     m_posBuffer[1].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    // CD3DX12_RESOURCE_BARRIER barrierrr12 = CD3DX12_RESOURCE_BARRIER::Transition(
    //     m_posBuffer[2].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);*/

    // {
    //     D3D12_RESOURCE_BARRIER barriers[1] = {barrierrr10};

    //     m_CommandList->ResourceBarrier(1, barriers);
    // }
}

void DeviceManager::CreateRaytracinuavTexturesBuffer()
{
    /*{
        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.DepthOrArraySize = 1;
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        resDesc.Width = bee::Engine.Device().GetWidth();
        resDesc.Height = bee::Engine.Device().GetHeight();
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_Device->CreateCommittedResource(
            &nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
            IID_PPV_ARGS(&m_outputResource)));
    }*/
}

void DeviceManager::FlushBuffers()
{
    // Flush();
    for (int i = 0; i < m_NumFrames; ++i)
    {
        m_CurrentBackBufferIndex = i;
        //  if (i == m_CurrentBackBufferIndex)
        m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), m_FenceValue[m_CurrentBackBufferIndex]);
    }

    for (int i = 0; i < m_NumFrames; ++i)
    {
        //  if (cur == i) continue;

        m_CurrentBackBufferIndex = i;
        WaitForPreviousFrame();
        m_FenceValue[m_CurrentBackBufferIndex]++;

        //  m_CommandQueue->Signal(m_Fence[i].Get(), m_FenceValue[i]);
        //   m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), m_FenceValue[m_CurrentBackBufferIndex]);
    }

    ::CloseHandle(m_FenceEvent);
}

DeviceManager::~DeviceManager()
{
    // m_window = nullptr;
    /* ImGui_ImplDX12_Shutdown();
  ImGui_ImplGlfw_Shutdown();ComPtr
  ImGui::DestroyContext();*/
}

void DeviceManager::FinishInit()
{
    m_CommandList->Close();  //---------------------------------CommanList Close

    ID3D12CommandList* ppCommandLists[] = {m_CommandList.Get()};

    m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    m_command_list_closed = true;
    m_FenceValue[m_CurrentBackBufferIndex]++;

    m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), m_FenceValue[m_CurrentBackBufferIndex]);

    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = bee::Engine.Device().GetWidth();
    m_Viewport.Height = bee::Engine.Device().GetHeight();
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    //  Scissor rect
    m_ScissorRect.left = 0;
    m_ScissorRect.top = 0;
    m_ScissorRect.right = bee::Engine.Device().GetWidth();
    m_ScissorRect.bottom = bee::Engine.Device().GetHeight();

    width = bee::Engine.Device().GetWidth();
    height = bee::Engine.Device().GetHeight();
}
void DeviceManager::BeginFrame()
{
    if (bee::Engine.Input().GetKeyboardKeyOnce(bee::Input::KeyboardKey::F11))
    {
        m_fullscreen = !m_fullscreen;
        SetFullscreen(m_fullscreen);
        // m_resize = true;
    }

    if (m_resize)
    {
        Resize(bee::Engine.Device().GetWidth(), bee::Engine.Device().GetHeight());
        m_resize = false;
    }
    // else
    //{
    m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
    WaitForPreviousFrame();
    m_FenceValue[m_CurrentBackBufferIndex]++;
    //  }
}

void DeviceManager::EndFrame()
{
    auto backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        m_CommandList->ResourceBarrier(1, &barrier);

        ThrowIfFailed(m_CommandList->Close());

        ID3D12CommandList* const commandLists[] = {m_CommandList.Get()};

        m_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
        m_command_list_closed = true;
        m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), m_FenceValue[m_CurrentBackBufferIndex]);
    }
    m_vsync = false;
    UINT syncInterval = m_vsync ? 1 : 0;
    UINT presentFlags = m_TearingSupported && !m_vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));
}

ComPtr<ID3D12Device5> DeviceManager::GetDevice() { return m_Device; }

ComPtr<ID3D12GraphicsCommandList4> DeviceManager::GetCommandList() { return m_CommandList; }

UINT DeviceManager::GetCurrentBufferIndex() { return m_CurrentBackBufferIndex; }

ComPtr<ID3D12Device5> DeviceManager::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
    ComPtr<ID3D12Device5> d3d12Device2;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&d3d12Device2)));
    HRESULT WINAPI D3D12CreateDevice(_In_opt_ IUnknown * pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, _In_ REFIID riid,
                                     _Out_opt_ void** ppDevice);

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};

        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
}

ComPtr<IDXGIAdapter4> DeviceManager::GetAdapter(bool useWarp)
{
    ComPtr<IDXGIFactory4> dxgiFactory;

    UINT createFactoryFlags = 0;

#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;

    if (useWarp)
    {
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    }
    else
    {
        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
            }
        }
    }

    return dxgiAdapter4;
}

bool DeviceManager::CheckTearingSupport()
{
    BOOL allowTearing = FALSE;

    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}

void DeviceManager::EnableDebugLayer()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}

ComPtr<IDXGISwapChain4> DeviceManager::CreateSwapChain(ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height,
                                                       uint32_t bufferCount)
{
    HWND hWnd = bee::Engine.Device().GetHWND();

    //  bee::Engine.Device().GetWindow();
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;

    ComPtr<IDXGIFactory4> dxgiFactory4;

    UINT createFactoryFlags = 0;

#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = {1, 0};
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(
        dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

    return dxgiSwapChain4;
}

ComPtr<ID3D12CommandAllocator> DeviceManager::CreateCommandAllocator(ComPtr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;

    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

void DeviceManager::CreateCommandAllocators()
{
    for (int i = 0; i < m_NumFrames; ++i)
    {
        m_CommandAllocators[i] = CreateCommandAllocator(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
}

ComPtr<ID3D12GraphicsCommandList4> DeviceManager::CreateCommandList(ComPtr<ID3D12Device5> device,
                                                                    ComPtr<ID3D12CommandAllocator> commandAllocator,
                                                                    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList4> commandList;

    ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    return commandList;
}

ComPtr<ID3D12CommandQueue> DeviceManager::CreateCommandQueue(ComPtr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

    return d3d12CommandQueue;
}

ComPtr<ID3D12DescriptorHeap> DeviceManager::CreateDescriptorHeap(ComPtr<ID3D12Device5> device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                                 D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    desc.Flags = flags;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

ComPtr<ID3D12Resource> DeviceManager::CreateRTVBuffer(ComPtr<ID3D12Device5> device, CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
                                                      DXGI_FORMAT format)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    ComPtr<ID3D12Resource> gBuffer;
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.DepthOrArraySize = 1;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Format = format;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    resDesc.Width = bee::Engine.Device().GetWidth();
    resDesc.Height = bee::Engine.Device().GetHeight();
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;

    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc,
                                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&gBuffer)));

    gBuffer->SetName(L"Screen Buffer");
    device->CreateRenderTargetView(gBuffer.Get(), nullptr, rtvHandle);

    rtvHandle.Offset(rtvDescriptorSize);

    return gBuffer;
}

void DeviceManager::UpdateRenderTargetViews(ComPtr<ID3D12Device5> device, ComPtr<IDXGISwapChain4> swapChain,
                                            ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < m_NumFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        m_BackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }

    if (b_RTVsInitialized)
    {
        m_posBuffer = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R32G32B32A32_FLOAT);

        /* if (booboo)
        {
            m_posBuffer[0] = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R8G8B8A8_UNORM);
            m_posBuffer[1] = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R8G8B8A8_UNORM);
            m_posBuffer[2] = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R8G8B8A8_UNORM);
            booboo = false;
        }*/

        m_normalBuffer = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R32G32B32A32_FLOAT);

        m_colorBuffer = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R8G8B8A8_UNORM);

        m_materialBuffer = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R8G8B8A8_UNORM);

        m_outputResource = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R8G8B8A8_UNORM);

        // m_shadowBuffer = CreateRTVBuffer(device, rtvHandle, DXGI_FORMAT_R8G8B8A8_UNORM);

        /*  D3D12_RESOURCE_DESC resDesc1 = {};
         resDesc1.DepthOrArraySize = 1;
         resDesc1.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
         resDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
         resDesc1.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
         resDesc1.Width = bee::Engine.Device().GetWidth();
         resDesc1.Height = bee::Engine.Device().GetHeight();
         resDesc1.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
         resDesc1.MipLevels = 1;
         resDesc1.SampleDesc.Count = 1;


         D3D12_HEAP_PROPERTIES heapProperties1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
         ThrowIfFailed(device->CreateCommittedResource(&heapProperties1, D3D12_HEAP_FLAG_NONE, &resDesc1,
                                                       D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                                       IID_PPV_ARGS(&m_outputResource)));

         m_outputResource->SetName(L"Screen Buffer");*/

        {
            D3D12_RESOURCE_DESC resDesc = {};
            resDesc.DepthOrArraySize = 1;
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            resDesc.Width = bee::Engine.Device().GetWidth();
            resDesc.Height = bee::Engine.Device().GetHeight();
            resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resDesc.MipLevels = 1;
            resDesc.SampleDesc.Count = 1;

            D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc,
                                                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                                          IID_PPV_ARGS(&m_shadowBuffer)));

            m_shadowBuffer->SetName(L"Screen Buffer");
        }

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.DepthOrArraySize = 1;
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        resDesc.Width = bee::Engine.Device().GetWidth() / 2;
        resDesc.Height = bee::Engine.Device().GetHeight() / 2;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;

        D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                                      IID_PPV_ARGS(&m_bloomBuffer)));

        m_bloomBuffer->SetName(L"Screen Buffer");

        /*  D3D12_RESOURCE_DESC resDesc1 = {};
          resDesc1.DepthOrArraySize = 1;
          resDesc1.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
          resDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
          resDesc1.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
          resDesc1.Width = bee::Engine.Device().GetWidth();
          resDesc1.Height = bee::Engine.Device().GetHeight();
          resDesc1.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
          resDesc1.MipLevels = 1;
          resDesc1.SampleDesc.Count = 1;

          D3D12_HEAP_PROPERTIES heapProperties2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
          ThrowIfFailed(device->CreateCommittedResource(&heapProperties2, D3D12_HEAP_FLAG_NONE, &resDesc1,
                                                        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                                        IID_PPV_ARGS(&m_materialResource)));

          m_materialResource->SetName(L"Screen Buffer");*/

        b_RTVsInitialized = false;
    }
}

void DeviceManager::WaitForPreviousFrame()
{
    if (m_Fence[m_CurrentBackBufferIndex]->GetCompletedValue() < m_FenceValue[m_CurrentBackBufferIndex])
    {
        m_Fence[m_CurrentBackBufferIndex]->SetEventOnCompletion(m_FenceValue[m_CurrentBackBufferIndex], m_FenceEvent);
        ::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
    }
}

void DeviceManager::Flush()
{
    // Signal the GPU to finish processing commands up to the current point.
    const UINT64 flushFenceValue = m_FenceValue[m_CurrentBackBufferIndex];
    m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), flushFenceValue);

    // Wait for the GPU to complete commands up to this point.
    WaitForPreviousFrame();
    // UINT cur = m_CurrentBackBufferIndex;
    // for (int i = 0; i < m_NumFrames; ++i)
    //{
    //     m_CurrentBackBufferIndex = i;
    //   //  if (i == m_CurrentBackBufferIndex)
    //     m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), m_FenceValue[m_CurrentBackBufferIndex]);
    // }

    // for (int i = 0; i < m_NumFrames; ++i)
    //{
    //   //  if (cur == i) continue;

    //    m_CurrentBackBufferIndex = i;

    //    WaitForPreviousFrame();
    //    m_FenceValue[m_CurrentBackBufferIndex]++;

    //  //  m_CommandQueue->Signal(m_Fence[i].Get(), m_FenceValue[i]);
    // //   m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), m_FenceValue[m_CurrentBackBufferIndex]);
    //}

    //  m_CurrentBackBufferIndex = cur;

    /* for (int i = 0; i < m_NumFrames; ++i)
     {
         m_CurrentBackBufferIndex = i;
         m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), m_FenceValue[m_CurrentBackBufferIndex]);
     }*/

    //   m_FenceValue[m_CurrentBackBufferIndex]++;

    //   m_CommandQueue->Signal(m_Fence[m_CurrentBackBufferIndex].Get(), m_FenceValue[m_CurrentBackBufferIndex]);
    /*  m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
     WaitForPreviousFrame();
     m_FenceValue[m_CurrentBackBufferIndex]++;*/
}

void DeviceManager::CheckRaytracingSupport()
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    ThrowIfFailed(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
    if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0) b_rayTracingEnabled = false;
      //  throw std::runtime_error("Raytracing not supported on device");
}

void DeviceManager::Resize(uint32_t width, uint32_t height)
{
    // if (bee::Engine.Device().GetWidth() != width || bee::Engine.Device().GetHeight() != height)
    // {
    //   g_ClientWidth = std::max(1u, width);
    //    g_ClientHeight = std::max(1u, height);

    /* m_Viewport.Width = width;
   m_Viewport.Height = height;

      m_ScissorRect.right = width;
   m_ScissorRect.bottom = height;*/

    Flush();
    /**/
    for (int i = 0; i < m_NumFrames; ++i)
    {
        m_BackBuffers[i].Reset();
        //   g_FenceValue[i] = g_FenceValue[g_CurrentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));

    ThrowIfFailed(m_SwapChain->ResizeBuffers(m_NumFrames, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

    // g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

    UpdateRenderTargetViews(m_Device, m_SwapChain, m_RTVDescriptorHeap);

    //  }
}
//

void setWindowed(GLFWwindow* window, int width, int height, int posX, int posY)
{
    // Switch to windowed mode
    glfwSetWindowMonitor(window, NULL, posX, posY, width, height, 0);  // Last parameter is refresh rate, 0 for no change
}
void DeviceManager::SetFullscreen(bool fullscreen)
{
    if (fullscreen)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(bee::Engine.Device().GetMonitor());

        glfwSetWindowMonitor(bee::Engine.Device().GetWindow(), bee::Engine.Device().GetMonitor(), 0, 0, mode->width,
                             mode->height, mode->refreshRate);
    }
    else
    {
        const GLFWvidmode* mode = glfwGetVideoMode(bee::Engine.Device().GetMonitor());

        int windowWidth = mode->width;
        int windowHeight = mode->height;

        glfwSetWindowMonitor(bee::Engine.Device().GetWindow(), nullptr, 0, 30, windowWidth, windowHeight, 0);
    }
}