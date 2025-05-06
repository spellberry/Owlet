#pragma once
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include "core/resource.hpp"
//#include "Dx12NiceRenderer/Renderer/include/DeviceManager.hpp"
#include "platform/dx12/Renderpipeline.hpp"
namespace bee
{

class Model;

class Mesh : public Resource
{
    friend class Resources;

public:
    Mesh();
    Mesh(const Model& model, int index);
    ~Mesh() override;

     enum class Attribute
    {
        Position,
        Normal,
        Tangent,
        Color,
        Texture,
        Texture1,
    };

    template <typename DataT>
    void SetAttribute(Attribute attribute, std::vector<DataT>& data);

    void SetAttribute(std::vector<Vertex>& data);

    void SetIndices(std::vector<DWORD>& data);
   
    void SetIndices(std::vector<uint16_t>& data);
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView; 
    size_t m_num_verts=0;
    size_t m_num_indices=0;

   // ConstantBufferPerObject constant_data;

    unsigned int m_mesh_index;
    unsigned int m_cb_index;
 
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_indexBuffer;
 
    bool b_resetBLAS = false;
    

    void ResetBLAS();


    AccelerationStructureBuffers m_BLASes;

   // ComPtr<ID3D12Resource> m_BLASBuffer;

protected:
     void SetAttribute(Attribute attribute, size_t count, void* data);
      static std::string GetPath(const Model& model, int index);

     //    uint32_t m_count = 0;
    //  uint32_t m_indexType = 0;

  private:

       void CreateVertexBuffer(const std::vector<Vertex>& vertex_list);
      void CreateIndexBuffer(const std::vector<DWORD>& index_list);

   

    ComPtr<ID3D12Resource> vBufferUploadHeap;
    ComPtr<ID3D12Resource> iBufferUploadHeap;

   

   

};

template <typename DataT>
inline void Mesh::SetAttribute(Attribute attribute, std::vector<DataT>& data)
{
    SetAttribute(attribute, data.size() * sizeof(data[0]), &data[0]);
}

}  // namespace bee