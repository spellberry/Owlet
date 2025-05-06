#pragma once
#include "core/ecs.hpp"
#include "rendering/render_components.hpp"


namespace bee
{

struct Material;
class MaterialSystem : public System
{
public:
    
    MaterialSystem();
    ~MaterialSystem() = default;
    
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif
    
    void Render() override;
    
    void SaveMaterial(const Material& material);
    void LoadMaterial(const std::string& path);
    void LoadMaterial(Material& material, const std::string& path);

    unsigned int RegisterDefaultMaterial();

private:
    void RenderTexturePreview(const unsigned int textureID);
    
    std::shared_ptr<Material> m_currentMaterial = nullptr;
    std::string m_currentMaterialPath = "";

//    Camera m_camera;
 //   std::shared_ptr < Model> m_previewModel;

    unsigned int m_baseTextureID = 0;
    unsigned int m_normalTextureID = 0;
        unsigned int m_metallicRoughnessTextureID = 0;
    unsigned int m_emissiveTextureID = 0;


    int m_materialCount = 0;


    
};
}  // namespace bee
