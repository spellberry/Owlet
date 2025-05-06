#pragma once

#include "user_interface/user_interface_structs.hpp"
// #include "platform/dx12/ui_render_data_dx12.hpp"
#include "platform/dx12/RenderPipeline.hpp"

namespace bee
{
namespace ui
{

class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    void Render();
    void ClearBuffers(internal::UIElement& element);

    void GenElement(internal::UIElement& element);
    void GenTexture(internal::UIImage& img, int c, unsigned char* imgData, std::string& name);
    void ReplaceImg(internal::UIElement& element, internal::UIComponent& compo, std::vector<float>& data);
    void genImageComp(ImgType type, internal::UIImageComponent& imgComp, std::string& name);
    void genTextComp(ImgType type, internal::UITextComponent& imgComp, std::string& name);
    void DelTexture(int image);
    std::vector<internal::UIImage> images;
    void ReplaceBuffers(RendererData& rData, size_t verticesSize, size_t indicesSize, float* VBOstart, unsigned int* EBOstart,
                        unsigned int vertex_size);
    void GenFont(Font& font, std::vector<float>& fim);

    void StartFrame(bool& execute);
    void EndFrame(bool execute);

private:
    friend class UserInterface;
    friend class UIEditor;
    enum ShaderType
    {
        vertex = 1,
        fragment = 2
    };
    // shader loading
    std::string readFile(const std::string& filename, std::string& error);
    int LoadShader(std::string name, ShaderType shaderType, std::string& error);

    void UpdateTextureDescriptorHeap(Font& img);
    ComPtr<ID3D12DescriptorHeap> m_fontDescriptorHeap;
    int font_count = 0;

    void CreateRootSignature(ComPtr<ID3D12RootSignature>& root_signature, ComPtr<ID3D12PipelineState>& pso,
                             LPCWSTR vertexShader, LPCWSTR pixelShader, UINT numInputElems, UINT numRootConstants);

    void CreateVertexBuffer(RendererData& element, const void* vertexList, size_t verticesSize, size_t indicesSize,
                            unsigned int vertex_size);
    void CreateIndexBuffer(RendererData& element, const void* indexList, size_t verticesSize, size_t indicesSize);

    void CreateTextureResource(const BYTE* data, internal::UIImage& img, bool genMipMaps);

    void UpdateTextureDescriptorHeap(internal::UIImage& img);

    // unsigned int font_count = 0;

    ComPtr<ID3D12RootSignature> m_FontRootSignature;
    ComPtr<ID3D12RootSignature> m_ImageRootSignature;
    ComPtr<ID3D12RootSignature> m_ProgressRootSignature;

    ComPtr<ID3D12PipelineState> m_FontPipelineStateObject;
    ComPtr<ID3D12PipelineState> m_ImagePipelineStateObject;
    ComPtr<ID3D12PipelineState> m_ProgressPipelineStateObject;

    ComPtr<ID3D12DescriptorHeap> m_imageDescriptorHeap;
    // ComPtr<ID3D12DescriptorHeap> m_fontDescriptorHeap;

    ComPtr<ID3D12Resource> m_ConstantBufferUploadHeaps[DeviceManager::m_NumFrames];

    UINT8* m_CbvGPUAddress[DeviceManager::m_NumFrames];
    int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;

    DirectX::XMFLOAT4X4 m_projectionMatrix;
    glm::mat4 m_projMatrix;
    //// shaders
    // int m_textShaderProgram = 0;
    // int m_imgShaderProgram = 0;
    // int m_progressBarProgram = 0;

    //// locations for uniforms
    // int imgprojection = 0;
    // int imgtrans = 0;
    // int pbprojection = 0;
    // int pbtrans = 0;
    // int textprojection = 0;
    // int texttrans = 0;

    int drawCalls = 0;
};
}  // namespace ui
}  // namespace bee