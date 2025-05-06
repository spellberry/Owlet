#pragma once
#include <string>

#include "core/resource.hpp"
#include "platform/dx12/DeviceManager.hpp"

#include <future>
#include <unordered_map>
#include <mutex>
    namespace bee
{
class Model;

struct CB
{
    float TexelSizeX;
    float TexelSizeY;
  //  int mip;
};

class Image : public Resource
{
    friend class Resources;

public:
    Image(const Model& model, int index);
    Image(const std::string& path);
    ~Image();

 /// Creates a texture from RGBA provided data
  //  void CreateGLTextureWithData(unsigned char* data, int width, int height, int channels, bool genMipMaps);
     int GetTextureId() const { return m_texture; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
 
    ComPtr<ID3D12Resource> m_TextureBuffer;
    D3D12_RESOURCE_DESC m_textureDesc;

    int m_mip_count = 0;

    bool UploadGPU();
    void WaitForLoad();

    private:
    void CreateTextureResource(const BYTE* data, int width, int height, int channels, bool genMipMaps);

    static std::string GetPath(const Model& model, int index);
    static std::string GetPath(const std::string& path) { return Resource::GetPath(path); }

    CB cbData;
  //  std::vector < ComPtr<ID3D12Resource>> constantBuffer;
    ComPtr < ID3D12Resource > constantBuffer;
    ComPtr<ID3D12Resource> m_TextureBufferUploadHeap;
   
   

    int m_texture = -1;
    int m_width = -1;
    int m_height = -1;
    int m_channels = -1;
   
    BYTE* imageData;

        static std::unordered_map<std::string, std::vector<char>> imageCache;
    static std::mutex cacheMutex;
        std::future<BYTE*> loadFuture;
    

    //ComPtr<ID3D12DescriptorHeap> m_mipMapHeap;
};

}  // namespace bee