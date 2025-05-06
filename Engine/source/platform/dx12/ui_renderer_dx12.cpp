#include "platform/dx12/ui_renderer_dx12.hpp"

#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "rendering/ui_render_data.hpp"
#include "tools/inspector.hpp"
#include "user_interface/font_handler.hpp"
#include "user_interface/user_interface.hpp"
// #include "Dx12NiceRenderer/Renderer/include/DeviceManager.hpp"
// #include "Dx12NiceRenderer/Renderer/include/RenderPipeline.hpp"
using namespace bee;
using namespace ui;
using namespace internal;

void UIRenderer::StartFrame(bool& execute)
{
    auto deviceManager = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager();
    if (deviceManager->m_command_list_closed)
    {
        deviceManager->WaitForPreviousFrame();
        deviceManager->m_FenceValue[deviceManager->GetCurrentBufferIndex()]++;

        auto commandAllocator = Engine.ECS()
                                    .GetSystem<RenderPipeline>()
                                    .GetDeviceManager()
                                    ->m_CommandAllocators[deviceManager->GetCurrentBufferIndex()];

        commandAllocator->Reset();
        deviceManager->GetCommandList()->Reset(commandAllocator.Get(), deviceManager->m_PipelineStateObject.Get());
        deviceManager->m_command_list_closed = false;
        execute = true;
    }
}
void UIRenderer::EndFrame(bool execute)
{
    if (execute)
    {
        auto deviceManager = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager();
        deviceManager->GetCommandList()->Close();

        ID3D12CommandList* ppCommandLists[] = {deviceManager->GetCommandList().Get()};

        deviceManager->m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
        deviceManager->m_command_list_closed = true;
        UINT back_index = deviceManager->GetCurrentBufferIndex();

        deviceManager->m_FenceValue[back_index]++;

        deviceManager->m_CommandQueue->Signal(deviceManager->m_Fence[back_index].Get(),
                                              deviceManager->m_FenceValue[back_index]);
    }
}

void UIRenderer::GenFont(Font& font, std::vector<float>& fim)
{
    int imageBytesPerRow = font.width * 12;

    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;

    font.tex.m_textureDesc = {};
    font.tex.m_textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    font.tex.m_textureDesc.Alignment = 0;
    font.tex.m_textureDesc.Width = font.width;
    font.tex.m_textureDesc.Height = font.height;
    font.tex.m_textureDesc.DepthOrArraySize = 1;
    font.tex.m_textureDesc.MipLevels = 1;
    font.tex.m_textureDesc.Format = dxgiFormat;
    font.tex.m_textureDesc.SampleDesc.Count = 1;
    font.tex.m_textureDesc.SampleDesc.Quality = 0;
    font.tex.m_textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    font.tex.m_textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    int imageSize = imageBytesPerRow * font.height;

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = fim.data();
    textureData.RowPitch = imageBytesPerRow;
    textureData.SlicePitch = imageBytesPerRow * font.tex.m_textureDesc.Height;

    if (imageSize <= 0)
    {
        throw std::exception("No image here");
    }
    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
        font.tex.m_TextureBuffer, L"Texture Buffer Resource Heap", CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        CD3DX12_RESOURCE_DESC(font.tex.m_textureDesc), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, nullptr);

    UINT64 textureUploadBufferSize;

    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->GetCopyableFootprints(
        &font.tex.m_textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
        font.tex.m_TextureBufferUploadHeap, L"Texture Buffer Upload  Resource Heap",
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
        D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(),
                       font.tex.m_TextureBuffer.Get(), font.tex.m_TextureBufferUploadHeap.Get(), 0, 0, 1, &textureData);

    CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        font.tex.m_TextureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);

    // m_texture =
    UpdateTextureDescriptorHeap(font);

    font_count++;

    // Engine.ECS().GetSystem<UIRenderer>().
}

void UIRenderer::UpdateTextureDescriptorHeap(Font& font)
{
    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();
    // Engine.ECS().GetSystem<FontHandler>().m_fonts
    // Engine.ECS().GetSystem<UIRenderer>().

    if (m_fontDescriptorHeap.Get() != nullptr)
    {
        // D3D12_DESCRIPTOR_HEAP_DESC newHeapDesc = {};
        // newHeapDesc.NumDescriptors = m_fontDescriptorHeap->GetDesc().NumDescriptors + 1;
        // newHeapDesc.Flags = true ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        // newHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        // ComPtr<ID3D12DescriptorHeap> newFontDescriptorHeap;

        // ThrowIfFailed(
        //     Renderer.GetDeviceManager()->GetDevice()->CreateDescriptorHeap(&newHeapDesc,
        //     IID_PPV_ARGS(&newFontDescriptorHeap)));

        // CD3DX12_CPU_DESCRIPTOR_HANDLE oldCpuStartHandle(m_fontDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        // CD3DX12_CPU_DESCRIPTOR_HANDLE newCpuStartHandle(newFontDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // Renderer.GetDeviceManager()->GetDevice()->CopyDescriptorsSimple(m_fontDescriptorHeap->GetDesc().NumDescriptors,
        //                                                                 newCpuStartHandle, oldCpuStartHandle,
        //                                                                 D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // m_fontDescriptorHeap = newFontDescriptorHeap;
    }

    else
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 10;  // m_texture_descriptor_count + m_tex_descriptor_increment;
        heapDesc.Flags = true ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        ThrowIfFailed(
            Renderer.GetDeviceManager()->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_fontDescriptorHeap)));
        m_fontDescriptorHeap->GetDesc().NumDescriptors;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuStartHandle(m_fontDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    UINT descriptorSize =
        Renderer.GetDeviceManager()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    int offset = font_count;

    cpuStartHandle.Offset(offset, descriptorSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = font.tex.m_textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    Renderer.GetDeviceManager()->GetDevice()->CreateShaderResourceView(font.tex.m_TextureBuffer.Get(), &srvDesc,
                                                                       cpuStartHandle);

    // m_texture_descriptor_count++;
    //  return m_texture_descriptor_count - 1;
}
UIRenderer::UIRenderer()
{
    /*  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
          {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
          {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};*/

    CreateRootSignature(m_FontRootSignature, m_FontPipelineStateObject, L"assets/shaders/FontVertexShader.hlsl",
                        L"assets/shaders/FontPixelShader.hlsl", 3, 1);

    CreateRootSignature(m_ImageRootSignature, m_ImagePipelineStateObject, L"assets/shaders/ImageVertexShader.hlsl",
                        L"assets/shaders/ImagePixelShader.hlsl", 2, 1);

    CreateRootSignature(m_ProgressRootSignature, m_ProgressPipelineStateObject, L"assets/shaders/PBVertexShader.hlsl",
                        L"assets/shaders/PBPixelShader.hlsl", 5, 1);

    int height = bee::Engine.Device().GetHeight();

    int width = bee::Engine.Device().GetWidth();

    // DirectX::XMMATRIX projectionDX = DirectX::XMMatrixOrthographicOffCenterLH(-1, 1.0f, -1.0f, 1, -1, 1.0f);

    /*  DirectX::XMMATRIX projectionDX =
        DirectX::XMMatrixOrthographicOffCenterLH(-1, (float)width / (float)height, -1.0f, 1, -1, 1.0f);
*/

    // DirectX::XMMATRIX projectionDX =
    //     DirectX::XMMatrixOrthographicOffCenterLH(0.0f, (float)width / (float)height, 1.0f, 0.0f, 10.0f, -10.0f);

    m_projMatrix = glm::ortho(0.0f, (float)width / (float)height, 1.0f, 0.0f, 100.0f, -100.0f);
    DirectX::XMMATRIX projectionDX = ConvertGLMToDXMatrix(m_projMatrix);

    /* DirectX::XMMATRIX projectionDX =
         DirectX::XMMatrixOrthographicOffCenterLH( (float)width / (float)height,0.0f, 1.0f,0.0f, -0.1f, 10.0f);*/
    /* DirectX::XMMATRIX projectionDX =
         DirectX::XMMatrixOrthographicLH((float)width,  (float)height, -10.0f, 10.0f);*/

    // Transpose for HLSL (if needed, depending on your setup)
    DirectX::XMMATRIX projectionDXTransposed = DirectX::XMMatrixTranspose(projectionDX);

    // Prepare matrix for constant buffer (assuming you need it in row-major form)

    DirectX::XMStoreFloat4x4(&m_projectionMatrix, projectionDXTransposed);

    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();

    for (int i = 0; i < Renderer.GetDeviceManager()->m_NumFrames; ++i)
    {
        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
            m_ConstantBufferUploadHeaps[i], L"Constant Buffer Upload Resource Heap",
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), D3D12_HEAP_FLAG_NONE,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

        CD3DX12_RANGE readRange(0, 0);

        ThrowIfFailed(m_ConstantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_CbvGPUAddress[i])));
    }
}
void UIRenderer::CreateRootSignature(ComPtr<ID3D12RootSignature>& root_signature, ComPtr<ID3D12PipelineState>& pso,
                                     LPCWSTR vertexShader, LPCWSTR pixelShader, UINT numInputElems, UINT numRootConstants)
{
    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();

    ComPtr<ID3D12Device5> device = Renderer.GetDeviceManager()->GetDevice();

    D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
    rootCBVDescriptor.RegisterSpace = 0;
    rootCBVDescriptor.ShaderRegister = 1;

    D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];

    descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorTableRanges[0].NumDescriptors = 1;
    descriptorTableRanges[0].BaseShaderRegister = 0;
    descriptorTableRanges[0].RegisterSpace = 0;
    descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Create a descriptor table
    D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
    descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
    descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

    D3D12_ROOT_PARAMETER rootParameters[3];
    /* rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
     rootParameters[0].Descriptor = rootCBVDescriptor;
     rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;*/
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.Num32BitValues = 1;  // Number of 32-bit values. Adjust as necessary.
    rootParameters[0].Constants.ShaderRegister = 0;  // The register to bind to.
    rootParameters[0].Constants.RegisterSpace = 0;   // The register space. Typically 0.
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor = rootCBVDescriptor;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable = descriptorTable;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler = {};

    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

    // if (numRootConstants == 6)//means it's the font pso
    //{
    //     sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //     sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    //     sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    // }
    // else
    {
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    }
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

    ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                              IID_PPV_ARGS(&root_signature)));

    ComPtr<IDxcBlob> vertexShaderBlob;

    /* ThrowIfFailed(D3DCompileFromFile(vertexShader, nullptr, nullptr, "main", "vs_5_0",
                                      D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShaderBlob, &errorBuff));*/

    vertexShaderBlob = Renderer.GetDeviceManager()->CompileShaderLibrary(vertexShader, L"vs_6_0");

    D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
    vertexShaderBytecode.BytecodeLength = vertexShaderBlob->GetBufferSize();
    vertexShaderBytecode.pShaderBytecode = vertexShaderBlob->GetBufferPointer();

    ComPtr<IDxcBlob> pixelShaderBlob;
    // ThrowIfFailed(D3DCompileFromFile(pixelShader, nullptr, nullptr, "main", "ps_5_0",
    //                                   D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShaderBlob, &errorBuff));

    pixelShaderBlob = Renderer.GetDeviceManager()->CompileShaderLibrary(pixelShader, L"ps_6_0");

    D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
    pixelShaderBytecode.BytecodeLength = pixelShaderBlob->GetBufferSize();
    pixelShaderBytecode.pShaderBytecode = pixelShaderBlob->GetBufferPointer();

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_INPUT_ELEMENT_DESC inputLayout2[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}

    };

    D3D12_INPUT_ELEMENT_DESC inputLayout3[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                               {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                                               {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                                                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    if (numInputElems == 2)
    {
        inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputLayout;
    }
    else if (numInputElems == 5)
    {
        inputLayoutDesc.NumElements = sizeof(inputLayout2) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputLayout2;
    }
    else if (numInputElems == 3)
    {
        inputLayoutDesc.NumElements = sizeof(inputLayout3) / sizeof(D3D12_INPUT_ELEMENT_DESC);
        inputLayoutDesc.pInputElementDescs = inputLayout3;
    }
    //  std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
    /* if (numInputElems==2)
     {
         inputLayout.resize(numInputElems);
         inputLayout[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
         inputLayout[1] = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
     }
     else
     {
         inputLayout.resize(numInputElems);
         inputLayout[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
         inputLayout[1] = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
         inputLayout[2] = {"BAR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
         inputLayout[3] = {"BCOLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
         inputLayout[4] = {"FCOLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

     }*/

    // D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

    // inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
    // inputLayoutDesc.pInputElementDescs = inputLayout;

    //---------------------------- Pipeline State Object

    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;  // Solid filling
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;   // Cull back-facing triangles
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
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

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

    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    psoDesc.SampleDesc = {1, 0};
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = blendDesc;
    psoDesc.NumRenderTargets = 1;

    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

UIRenderer::~UIRenderer() {}
#include <iostream>

bool sortUIElements(const std::tuple<bee::Entity, UIElement, Transform>& element1,
                    const std::tuple<bee::Entity, UIElement, Transform>& element2)
{
    return std::get<2>(element1).Translation.z > std::get<2>(element2).Translation.z;
}

void UIRenderer::Render()
{
    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();
    auto deviceManager = Renderer.GetDeviceManager();
    auto commandList = deviceManager->GetCommandList();

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
        deviceManager->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 7,
        deviceManager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv1(
        deviceManager->m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), deviceManager->GetCurrentBufferIndex(),
        deviceManager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(deviceManager->m_DepthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    auto view = Engine.ECS().Registry.view<UIElement, Transform>();

    auto& UI = Engine.ECS().GetSystem<UserInterface>();

    std::vector<std::tuple<bee::Entity, UIElement, Transform>> drawables;
    for (const auto& [e, renderer, transform] : Engine.ECS().Registry.view<UIElement, Transform>().each())
    {
        drawables.push_back({e, renderer, transform});
    }
    std::sort(drawables.begin(), drawables.end(), sortUIElements);
    int counter = 0;
    for (auto& [ent, element, trans] : drawables)
    {
        if (m_imageDescriptorHeap)
        {
            ID3D12DescriptorHeap* descriptorHeaps[] = {m_imageDescriptorHeap.Get()};
            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        }
        if (element.drawing)
        {
            /* auto transmat = trans.World();
             glUseProgram(m_imgShaderProgram);
             glUniformMatrix4fv(imgtrans, 1, GL_FALSE, glm::value_ptr(transmat));
             glUniform1f(imgop, element.opacity);*/

            glm::mat4 transmat = glm::mat4(0.0f);
            if (ent == UI.m_selectedElementOverlay && UI.GetSelectedInteractable() != -1)
            {
                transmat = Engine.ECS().Registry.get<Transform>(UI.GetSelectedElement()).WorldMatrix;
            }
            else
            {
                transmat = trans.WorldMatrix;
            }

            DirectX::XMMATRIX worldMat = ConvertGLMToDXMatrix(transmat);
            DirectX::XMMATRIX worldDXTransposed = DirectX::XMMatrixTranspose(worldMat);

            DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4(&m_projectionMatrix);

            DirectX::XMMATRIX wpMat = worldDXTransposed * projMat;

            DirectX::XMMATRIX transposed = DirectX::XMMatrixTranspose(wpMat);

            DirectX::XMStoreFloat4x4(&element.elRData.m_constant_data.wpMat, wpMat);

            //  element.textRenderData.m_constant_data.wpMat = worldMat*projMat;

            memcpy(m_CbvGPUAddress[deviceManager->GetCurrentBufferIndex()] + counter * ConstantBufferPerObjectAlignedSize,
                   &element.elRData.m_constant_data, sizeof(element.elRData.m_constant_data));

            commandList->SetPipelineState(m_ImagePipelineStateObject.Get());
            commandList->SetGraphicsRootSignature(m_ImageRootSignature.Get());

            commandList->SetGraphicsRootConstantBufferView(
                1, m_ConstantBufferUploadHeaps[deviceManager->GetCurrentBufferIndex()]->GetGPUVirtualAddress() +
                       counter * ConstantBufferPerObjectAlignedSize);
            counter++;

            for (auto& imgcomp : element.imageComponents)
            {
                //  imgcomp.second.renderData.m_constant_data;

                commandList->IASetVertexBuffers(0, 1, &imgcomp.second.renderData.m_vertexBufferView);

                commandList->IASetIndexBuffer(&imgcomp.second.renderData.m_indexBufferView);

                CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStartHandle(m_imageDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
                UINT descriptorSize =
                    deviceManager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                int texture_index = 0;
                texture_index = imgcomp.first;

                gpuStartHandle.Offset(texture_index, descriptorSize);

                FLOAT opacity_value = element.opacity;
                commandList->SetGraphicsRoot32BitConstants(0, 1, &opacity_value, 0);

                commandList->SetGraphicsRootDescriptorTable(2, gpuStartHandle);

                commandList->DrawIndexedInstanced(imgcomp.second.indices.size(), 1, 0, 0, 0);
                drawCalls++;
            }

            commandList->SetPipelineState(m_ProgressPipelineStateObject.Get());
            commandList->SetGraphicsRootSignature(m_ProgressRootSignature.Get());

            for (auto& barComp : element.progressBarimageComponents)
            {
                commandList->IASetVertexBuffers(0, 1, &barComp.second.renderData.m_vertexBufferView);

                commandList->IASetIndexBuffer(&barComp.second.renderData.m_indexBufferView);

                CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStartHandle(m_imageDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
                UINT descriptorSize =
                    deviceManager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                int texture_index = 0;
                texture_index = barComp.first;

                gpuStartHandle.Offset(texture_index, descriptorSize);

                FLOAT opacity_value = element.opacity;
                commandList->SetGraphicsRoot32BitConstants(0, 1, &opacity_value, 0);

                commandList->SetGraphicsRootDescriptorTable(2, gpuStartHandle);
                // barComp.second.indices.size()
                commandList->DrawIndexedInstanced(barComp.second.indices.size(), 1, 0, 0, 0);
                drawCalls++;
            }

            commandList->SetPipelineState(m_FontPipelineStateObject.Get());
            commandList->SetGraphicsRootSignature(m_FontRootSignature.Get());

            ID3D12DescriptorHeap* descriptorHeaps[] = {m_fontDescriptorHeap.Get()};

            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            for (auto& textComp : element.textComponents)
            {
                commandList->IASetVertexBuffers(0, 1, &textComp.second.renderData.m_vertexBufferView);

                commandList->IASetIndexBuffer(&textComp.second.renderData.m_indexBufferView);

                CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStartHandle(m_fontDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
                UINT descriptorSize =
                    deviceManager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                int font_index = textComp.first;

                gpuStartHandle.Offset(font_index, descriptorSize);

                FLOAT opacity_value = element.opacity;
                commandList->SetGraphicsRoot32BitConstants(0, 1, &opacity_value, 0);

                commandList->SetGraphicsRootDescriptorTable(2, gpuStartHandle);
                // barComp.second.indices.size()

                commandList->DrawIndexedInstanced(textComp.second.indices.size(), 1, 0, 0, 0);
                drawCalls++;
            }
        }
    }
#ifdef BEE_INSPECTOR
    CD3DX12_RESOURCE_BARRIER barrierrr9 = CD3DX12_RESOURCE_BARRIER::Transition(
        deviceManager->m_outputResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    {
        D3D12_RESOURCE_BARRIER barriers[1] = {barrierrr9};

        commandList->ResourceBarrier(1, barriers);
    }
    commandList->OMSetRenderTargets(1, &rtv1, FALSE, &dsvHandle);

#else
    commandList->OMSetRenderTargets(1, &rtv1, FALSE, &dsvHandle);

    bool b_rayTracingEnabled = true;

    CD3DX12_RESOURCE_BARRIER barrierrr9 = CD3DX12_RESOURCE_BARRIER::Transition(
        deviceManager->m_outputResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

    CD3DX12_RESOURCE_BARRIER barrierrr10 =
        CD3DX12_RESOURCE_BARRIER::Transition(deviceManager->m_BackBuffers[deviceManager->GetCurrentBufferIndex()].Get(),
                                             D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);

    {
        D3D12_RESOURCE_BARRIER barriers[2] = {barrierrr9, barrierrr10};

        deviceManager->GetCommandList()->ResourceBarrier(2, barriers);
    }

    deviceManager->GetCommandList()->CopyResource(deviceManager->m_BackBuffers[deviceManager->GetCurrentBufferIndex()].Get(),
                                                  deviceManager->m_outputResource.Get());

    barrierrr9 = CD3DX12_RESOURCE_BARRIER::Transition(deviceManager->m_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    barrierrr10 =
        CD3DX12_RESOURCE_BARRIER::Transition(deviceManager->m_BackBuffers[deviceManager->GetCurrentBufferIndex()].Get(),
                                             D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_RESOURCE_BARRIER barriers[2] = {barrierrr9, barrierrr10};
    deviceManager->GetCommandList()->ResourceBarrier(2, barriers);

    // ID3D12DescriptorHeap* imguiDescriptorHeaps[] = {device_manager->m_ImGui_DescriptorHeap.Get()};
    // device_manager->GetCommandList()->SetDescriptorHeaps(_countof(imguiDescriptorHeaps), imguiDescriptorHeaps);

    deviceManager->EndFrame();

#endif
}

void UIRenderer::ClearBuffers(UIElement& element) {}
void UIRenderer::GenElement(UIElement& element)
{
    /* glGenVertexArrays(1, &element.textRenderData.VAO);
    glGenBuffers(1, &element.textRenderData.VBO);
    glGenBuffers(1, &element.textRenderData.EBO);

    glBindVertexArray(element.textRenderData.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, element.textRenderData.VBO);
    glBufferData(GL_ARRAY_BUFFER, BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element.textRenderData.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    LabelGL(GL_VERTEX_ARRAY, element.textRenderData.VAO, "VAO | text | element: " + std::to_string((int)element.ID));
    LabelGL(GL_BUFFER, element.textRenderData.VBO, "VBO | text | element: " + std::to_string((int)element.ID));
    LabelGL(GL_BUFFER, element.textRenderData.EBO, "EBO | text | element: " + std::to_string((int)element.ID));
*/
    std::vector<float> vList;
    std::vector<UINT> iList;

    /*float v;
    UINT i;
    vList.push_back(v);
    vList.push_back(v);
    vList.push_back(v);
    vList.push_back(v);
    vList.push_back(v);

    iList.push_back(i);*/
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texCoord;
        DirectX::XMFLOAT4 Color;
    };

    Vertex screenQuadVertices[] = {
        {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},   // Top Left
        {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},    // Top Right
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},  // Bottom Left
        {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}    // Bottom Right
    };

    UINT screenQuadIndices[] = {
        0, 1, 2,  // First triangle (Top Left, Top Right, Bottom Left)
        1, 3, 2   // Second triangle (Top Right, Bottom Right, Bottom Left)
    };

    bool execute = false;
    // StartFrame(execute);
    // CreateVertexBuffer(element.textRenderData, screenQuadVertices, 20, 6, 9);
    // CreateIndexBuffer(element.textRenderData, screenQuadIndices, 20, 6);
    // EndFrame(execute);
}
void UIRenderer::GenTexture(UIImage& img, int c, unsigned char* imgData, std::string& name)
{
    bool execute = false;
    StartFrame(execute);
    /* glGenTextures(1, &img.tex.texture);
     glBindTexture(GL_TEXTURE_2D, img.tex.texture);

     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
     if (c == 3)
     {
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, imgData);
     }
     else if (c == 4)
     {
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
     }
     glGenerateTextureMipmap(img.tex.texture);
     LabelGL(GL_TEXTURE, img.tex.texture, "TEX | " + name);*/
    CreateTextureResource(imgData, img, false);
    EndFrame(execute);
}
void UIRenderer::ReplaceImg(UIElement& element, UIComponent& compo, std::vector<float>& data)
{
    bool execute = false;
    StartFrame(execute);
    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();

    if (element.imageComponents.find(compo.image)->second.renderData.m_vertexBufferView.SizeInBytes !=
        sizeof(float) * data.size()
        /*  ||
           element.imageComponents.find(compo.image)->second.renderData.m_indexBufferView.SizeInBytes !=
               sizeof(UINT) * indicesSize*/
    )
    {
        Renderer.GetResourceManager()->m_deletion_queue.push(
            element.imageComponents.find(compo.image)->second.renderData.m_vertexBuffer);
        Renderer.GetResourceManager()->m_deletion_queue.push(
            element.imageComponents.find(compo.image)->second.renderData.vBufferUploadHeap);
        Renderer.GetResourceManager()->m_deletion_queue.push(
            element.imageComponents.find(compo.image)->second.renderData.m_indexBuffer);
        Renderer.GetResourceManager()->m_deletion_queue.push(
            element.imageComponents.find(compo.image)->second.renderData.iBufferUploadHeap);

        element.imageComponents.find(compo.image)->second.renderData.m_vertexBuffer = nullptr;
        element.imageComponents.find(compo.image)->second.renderData.vBufferUploadHeap = nullptr;
        element.imageComponents.find(compo.image)->second.renderData.m_indexBuffer = nullptr;
        element.imageComponents.find(compo.image)->second.renderData.iBufferUploadHeap = nullptr;

        CreateVertexBuffer(element.imageComponents.find(compo.image)->second.renderData, data.data(), data.size(), 6, 5);
        //  CreateIndexBuffer(element.imageComponents.find(compo.image)->second.renderData, EBOstart, verticesSize,
        //  indicesSize);
        EndFrame(execute);

        return;
    }

    DWORD count = element.imageComponents.find(compo.image)->second.vertices.size();
    int vBufferSize = sizeof(float) * count;

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =

            CD3DX12_RESOURCE_BARRIER::Transition(
                element.imageComponents.find(compo.image)->second.renderData.m_vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<const BYTE*>(element.imageComponents.find(compo.image)->second.vertices.data());
    vertexData.RowPitch = vBufferSize;
    vertexData.SlicePitch = vBufferSize;

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(),
                       element.imageComponents.find(compo.image)->second.renderData.m_vertexBuffer.Get(),
                       element.imageComponents.find(compo.image)->second.renderData.vBufferUploadHeap.Get(), 0, 0, 1,
                       &vertexData);

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =

            CD3DX12_RESOURCE_BARRIER::Transition(
                element.imageComponents.find(compo.image)->second.renderData.m_vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    element.imageComponents.find(compo.image)->second.renderData.m_vertexBufferView.BufferLocation =
        element.imageComponents.find(compo.image)->second.renderData.m_vertexBuffer->GetGPUVirtualAddress();
    element.imageComponents.find(compo.image)->second.renderData.m_vertexBufferView.StrideInBytes = sizeof(float) * 5;
    element.imageComponents.find(compo.image)->second.renderData.m_vertexBufferView.SizeInBytes = vBufferSize;
    EndFrame(execute);
}

void UIRenderer::genImageComp(ImgType type, UIImageComponent& imgComp, std::string& name)
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texCoord;
    };

    // Vertex screenQuadVertices[] = {
    //     {{-1.0f, 0.5f, 0.0f}, {0.0f, 0.0f}},   // Top Left
    //     {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f}},    // Top Right
    //     {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // Bottom Left
    //     {{0.5f, -1.0f, 0.0f}, {1.0f, 1.0f}}    // Bottom Right
    // };

    Vertex screenQuadVertices[10] = {
        // Bottom Right
    };

    unsigned int squareIndices[] = {
        0, 1, 2,  // First Triangle
        1, 3, 2   // Second Triangle
    };
    // after square and in gencomp

    bool execute = false;

    StartFrame(execute);

    ////iList.push_back(i);
    CreateVertexBuffer(imgComp.renderData, screenQuadVertices, 10, 6, 5);
    CreateIndexBuffer(imgComp.renderData, squareIndices, 10, 6);

    EndFrame(execute);
}
void UIRenderer::DelTexture(int image) {}
void bee::ui::UIRenderer::genTextComp(ImgType type, internal::UITextComponent& imgComp, std::string& name)
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT2 texCoord;
        DirectX::XMFLOAT4 Color;
    };

    // Vertex screenQuadVertices[] = {
    //     {{-1.0f, 0.5f, 0.0f}, {0.0f, 0.0f}},   // Top Left
    //     {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f}},    // Top Right
    //     {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},  // Bottom Left
    //     {{0.5f, -1.0f, 0.0f}, {1.0f, 1.0f}}    // Bottom Right
    // };

    Vertex screenQuadVertices[10] = {};

    unsigned int squareIndices[] = {
        0, 1, 2,  // First Triangle
        1, 3, 2   // Second Triangle
    };
    // after square and in gencomp

    bool execute = false;

    StartFrame(execute);

    CreateVertexBuffer(imgComp.renderData, screenQuadVertices, 10, 6, 9);
    CreateIndexBuffer(imgComp.renderData, squareIndices, 10, 6);

    EndFrame(execute);
}
void UIRenderer::ReplaceBuffers(RendererData& rData, size_t verticesSize, size_t indicesSize, float* VBOstart,
                                unsigned int* EBOstart, unsigned int vertex_size)
{
    bool execute = false;
    auto deviceManager = Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager();

    StartFrame(execute);
    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();

    if (rData.m_vertexBufferView.SizeInBytes != sizeof(float) * verticesSize ||
        rData.m_indexBufferView.SizeInBytes != sizeof(UINT) * indicesSize)
    {
        Renderer.GetResourceManager()->m_deletion_queue.push(rData.m_vertexBuffer);
        Renderer.GetResourceManager()->m_deletion_queue.push(rData.vBufferUploadHeap);
        Renderer.GetResourceManager()->m_deletion_queue.push(rData.m_indexBuffer);
        Renderer.GetResourceManager()->m_deletion_queue.push(rData.iBufferUploadHeap);

        rData.m_vertexBuffer = nullptr;
        rData.vBufferUploadHeap = nullptr;
        rData.m_indexBuffer = nullptr;
        rData.iBufferUploadHeap = nullptr;

        CreateVertexBuffer(rData, VBOstart, verticesSize, indicesSize, vertex_size);
        CreateIndexBuffer(rData, EBOstart, verticesSize, indicesSize);

        EndFrame(execute);

        return;
    }

    DWORD vcount = verticesSize;
    int vBufferSize = sizeof(float) * vcount;

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =

            CD3DX12_RESOURCE_BARRIER::Transition(rData.m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                                 D3D12_RESOURCE_STATE_COPY_DEST);

        deviceManager->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<const BYTE*>(VBOstart);
    vertexData.RowPitch = vBufferSize;
    vertexData.SlicePitch = vBufferSize;

    UpdateSubresources(deviceManager->GetCommandList().Get(), rData.m_vertexBuffer.Get(), rData.vBufferUploadHeap.Get(), 0, 0,
                       1, &vertexData);

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =

            CD3DX12_RESOURCE_BARRIER::Transition(rData.m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        deviceManager->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    rData.m_vertexBufferView.BufferLocation = rData.m_vertexBuffer->GetGPUVirtualAddress();
    rData.m_vertexBufferView.StrideInBytes = sizeof(float) * vertex_size;
    rData.m_vertexBufferView.SizeInBytes = vBufferSize;

    EndFrame(execute);
}

void UIRenderer::CreateVertexBuffer(RendererData& rd, const void* vertexList, size_t verticesSize, size_t indicesSize,
                                    unsigned int vertex_size)
{
    int vBufferSize = sizeof(float) * verticesSize;

    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
        rd.m_vertexBuffer, L"Vertex Buffer Resource Heap", CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, nullptr);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&rd.vBufferUploadHeap));
    rd.vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = reinterpret_cast<const BYTE*>(vertexList);
    vertexData.RowPitch = vBufferSize;
    vertexData.SlicePitch = vBufferSize;

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(),
                       rd.m_vertexBuffer.Get(), rd.vBufferUploadHeap.Get(), 0, 0, 1, &vertexData);

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =
            /*CD3DX12_RESOURCE_BARRIER::Transition(m_meshes[index].m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);*/
            CD3DX12_RESOURCE_BARRIER::Transition(rd.m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    rd.m_vertexBufferView.BufferLocation = rd.m_vertexBuffer->GetGPUVirtualAddress();
    rd.m_vertexBufferView.StrideInBytes = sizeof(float) * vertex_size;
    rd.m_vertexBufferView.SizeInBytes = vBufferSize;
}

void UIRenderer::CreateIndexBuffer(RendererData& rd, const void* indexList, size_t verticesSize, size_t indicesSize)
{
    int iBufferSize = sizeof(UINT) * indicesSize;

    // m_num_indices = index_list.size();

    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
        rd.m_indexBuffer, L"Index Buffer Resource Heap", CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, nullptr);

    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * verticesSize);
        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&rd.iBufferUploadHeap));

        rd.iBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");
    }

    D3D12_SUBRESOURCE_DATA indexData = {};
    indexData.pData = reinterpret_cast<const BYTE*>(indexList);
    indexData.RowPitch = iBufferSize;
    indexData.SlicePitch = iBufferSize;

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(),
                       rd.m_indexBuffer.Get(), rd.iBufferUploadHeap.Get(), 0, 0, 1, &indexData);

    {
        CD3DX12_RESOURCE_BARRIER transitionBarrier =
            /*  CD3DX12_RESOURCE_BARRIER::Transition(m_meshes[index].m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                   D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);*/
            CD3DX12_RESOURCE_BARRIER::Transition(rd.m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                 D3D12_RESOURCE_STATE_INDEX_BUFFER);

        Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);
    }

    rd.m_indexBufferView.BufferLocation = rd.m_indexBuffer->GetGPUVirtualAddress();
    rd.m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    rd.m_indexBufferView.SizeInBytes = iBufferSize;
}
void UIRenderer::CreateTextureResource(const BYTE* data, UIImage& img, bool genMipMaps)
{
    //   const BYTE* imageData = &gltfImage.image[0];
    int imageBytesPerRow = img.width * 4;

    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    // DXGI_FORMAT_R8G8B8_UNORM

    /*   if (gltfImage.mimeType == "image/jpeg")
      {
          dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
      }
      else if (gltfImage.mimeType == "image/png")
      {
          dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
      }*/

    img.tex.m_textureDesc = {};
    img.tex.m_textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    img.tex.m_textureDesc.Alignment = 0;
    img.tex.m_textureDesc.Width = img.width;
    img.tex.m_textureDesc.Height = img.height;
    img.tex.m_textureDesc.DepthOrArraySize = 1;
    img.tex.m_textureDesc.MipLevels = 1;
    img.tex.m_textureDesc.Format = dxgiFormat;
    img.tex.m_textureDesc.SampleDesc.Count = 1;
    img.tex.m_textureDesc.SampleDesc.Quality = 0;
    img.tex.m_textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    img.tex.m_textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    int imageSize = imageBytesPerRow * img.height;

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = data;
    textureData.RowPitch = imageBytesPerRow;
    textureData.SlicePitch = imageBytesPerRow * img.tex.m_textureDesc.Height;

    if (imageSize <= 0)
    {
        throw std::exception("No image here");
    }
    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
        img.tex.m_TextureBuffer, L"Texture Buffer Resource Heap", CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        CD3DX12_RESOURCE_DESC(img.tex.m_textureDesc), D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, nullptr);

    UINT64 textureUploadBufferSize;

    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->GetCopyableFootprints(
        &img.tex.m_textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

    Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->CreateCommittedResource(
        img.tex.m_TextureBufferUploadHeap, L"Texture Buffer Upload  Resource Heap",
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize),
        D3D12_HEAP_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

    UpdateSubresources(Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList().Get(),
                       img.tex.m_TextureBuffer.Get(), img.tex.m_TextureBufferUploadHeap.Get(), 0, 0, 1, &textureData);

    CD3DX12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        img.tex.m_TextureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

    Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetCommandList()->ResourceBarrier(1, &transitionBarrier);

    // m_texture =
    UpdateTextureDescriptorHeap(img);
}
void UIRenderer::UpdateTextureDescriptorHeap(UIImage& img)
{
    auto& Renderer = Engine.ECS().GetSystem<RenderPipeline>();

    if (m_imageDescriptorHeap)
    {
        // D3D12_DESCRIPTOR_HEAP_DESC newHeapDesc = {};
        // newHeapDesc.NumDescriptors = m_imageDescriptorHeap->GetDesc().NumDescriptors + 1;
        // newHeapDesc.Flags = true ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        // newHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        // ComPtr<ID3D12DescriptorHeap> newFontDescriptorHeap;

        // ThrowIfFailed(
        //     Renderer.GetDeviceManager()->GetDevice()->CreateDescriptorHeap(&newHeapDesc,
        //     IID_PPV_ARGS(&newFontDescriptorHeap)));

        // CD3DX12_CPU_DESCRIPTOR_HANDLE oldCpuStartHandle(m_imageDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        // CD3DX12_CPU_DESCRIPTOR_HANDLE newCpuStartHandle(newFontDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // Renderer.GetDeviceManager()->GetDevice()->CopyDescriptorsSimple(m_imageDescriptorHeap->GetDesc().NumDescriptors,
        //                                                                 newCpuStartHandle, oldCpuStartHandle,
        //                                                                 D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        // m_imageDescriptorHeap = newFontDescriptorHeap;
    }
    else
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 100;  // m_texture_descriptor_count + m_tex_descriptor_increment;
        heapDesc.Flags = true ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        ThrowIfFailed(
            Renderer.GetDeviceManager()->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_imageDescriptorHeap)));
        m_imageDescriptorHeap->GetDesc().NumDescriptors;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuStartHandle(m_imageDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    UINT descriptorSize =
        Renderer.GetDeviceManager()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    cpuStartHandle.Offset(images.size(), descriptorSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = img.tex.m_textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    Renderer.GetDeviceManager()->GetDevice()->CreateShaderResourceView(img.tex.m_TextureBuffer.Get(), &srvDesc, cpuStartHandle);

    // m_texture_descriptor_count++;
    //  return m_texture_descriptor_count - 1;
}
