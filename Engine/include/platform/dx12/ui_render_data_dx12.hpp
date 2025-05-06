#pragma once
#include "platform/dx12/DeviceManager.hpp"

namespace bee
{
namespace ui
{
struct MatrixConstantBuffer
{
    DirectX::XMFLOAT4X4 wpMat;
};
struct RendererData
{
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;

    ComPtr<ID3D12Resource> vBufferUploadHeap;
    ComPtr<ID3D12Resource> iBufferUploadHeap;

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
};
struct UITexture
{
    ComPtr<ID3D12Resource> m_TextureBuffer;
    ComPtr<ID3D12Resource> m_TextureBufferUploadHeap;
    D3D12_RESOURCE_DESC m_textureDesc;
};
struct ElementRenderData
{
    MatrixConstantBuffer m_constant_data;
};
}  // namespace ui
}  // namespace bee
