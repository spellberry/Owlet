#include "material_system/material_system.hpp"
#include "rendering/render_components.hpp"

#include <imgui/imgui.h>
#include "imgui/imgui_stdlib.h"

#include <fstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <platform/dx12/RenderPipeline.hpp>

#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "core/resources.hpp"
#include "tools/asset_explorer_system.hpp"
#include "tools/tools.hpp"

#include "platform/dx12/image_dx12.h"
using namespace bee;

MaterialSystem::MaterialSystem()
{
    Title = "Material System";
   // m_previewModel = Engine.Resources().Load<Model>("models/fleet/Sphere.gltf");
}

#ifdef BEE_INSPECTOR
void MaterialSystem::Inspect()
{
    System::Inspect();
    ImGui::Begin("Material editor");
    if(ImGui::Button("New Material"))
    {
        m_currentMaterial = std::make_shared<Material>();
        m_currentMaterial->Name = "New material";
        
    }

    ImGui::InputText("Current material", &m_currentMaterialPath);
    std::filesystem::path path;
    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".pepimat"}))
    {
        m_currentMaterialPath = path.string();
        RemoveSubstring(m_currentMaterialPath, "assets/");
        LoadMaterial(m_currentMaterialPath);
                        
    }
    
    ImGui::Separator();
    ImGui::Text("--- Material Editor ---");
    ImGui::Separator();
        if(m_currentMaterial)
        {

                // Create a text input field
                ImGui::InputText("Material Name", &m_currentMaterial->Name);



                ImGui::ColorEdit4("Base Color Factor", &m_currentMaterial->BaseColorFactor[0]);
                ImGui::ColorEdit3("Emissive Factor", &m_currentMaterial->EmissiveFactor[0]);
                ImGui::SliderFloat("Metallic Factor", &m_currentMaterial->MetallicFactor, 0.0f, 1.0f);
                ImGui::SliderFloat("Roughness Factor", &m_currentMaterial->RoughnessFactor, 0.0f, 1.0f);
                
            
                ImGui::Checkbox("Use Base Texture", &m_currentMaterial->UseBaseTexture);
                if(m_currentMaterial->UseBaseTexture)
                {
                    ImGui::InputText("Base texture", &m_currentMaterial->BaseColorTexturePath);
                    std::filesystem::path path;

                    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".png"}))
                    {
                        m_currentMaterial->BaseColorTexturePath = path.string();
                        RemoveSubstring(m_currentMaterial->BaseColorTexturePath, "assets/");
                        auto image = Engine.Resources().Load<Image>(m_currentMaterial->BaseColorTexturePath);
                        auto sampler = std::make_shared<Sampler>();
                        m_currentMaterial->BaseColorTexture = std::make_shared<Texture>(image, sampler);
                        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->UpdateImGuiDescriptorHeap(m_currentMaterial->BaseColorTexture->Image.get(),1);
                    }

                    RenderTexturePreview(1);
                   
                }    
                ImGui::Checkbox("Use Emissive Texture", &m_currentMaterial->UseEmissiveTexture);
                if(m_currentMaterial->UseEmissiveTexture)
                {

                    ImGui::InputText("Emissive texture", &m_currentMaterial->EmissiveTexturePath);
                    std::filesystem::path path;
                    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".png"}))
                    {
                        m_currentMaterial->EmissiveTexturePath = path.string();
                        RemoveSubstring(m_currentMaterial->EmissiveTexturePath, "assets/");
                        auto image = Engine.Resources().Load<Image>(m_currentMaterial->EmissiveTexturePath);
                        auto sampler = std::make_shared<Sampler>();
                        m_currentMaterial->EmissiveTexture = std::make_shared<Texture>(image, sampler);
                        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->UpdateImGuiDescriptorHeap(m_currentMaterial->EmissiveTexture->Image.get(),2);

                    }
                    RenderTexturePreview(2);

                }
                ImGui::Checkbox("Use Normal Texture", &m_currentMaterial->UseNormalTexture);
                if(m_currentMaterial->UseNormalTexture)
                {
                    ImGui::InputText("Normal texture", &m_currentMaterial->NormalTexturePath);
                    std::filesystem::path path;
                    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".png"}))
                    {
                        m_currentMaterial->NormalTexturePath = path.string();
                        RemoveSubstring(m_currentMaterial->NormalTexturePath, "assets/");
                        auto image = Engine.Resources().Load<Image>(m_currentMaterial->NormalTexturePath);
                        auto sampler = std::make_shared<Sampler>();
                        m_currentMaterial->NormalTexture = std::make_shared<Texture>(image, sampler);
                        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->UpdateImGuiDescriptorHeap(m_currentMaterial->NormalTexture->Image.get(),3);

                    }
                    RenderTexturePreview(3);
                    ImGui::SliderFloat("Normal Texture Scale", &m_currentMaterial->NormalTextureScale, 0.0f, 1.0f);
                }

                ImGui::Checkbox("Use Occlusion Texture", &m_currentMaterial->UseOcclusionTexture);
                if(m_currentMaterial->UseOcclusionTexture)
                {
                    ImGui::InputText("Occlusion texture", &m_currentMaterial->OcclusionTexturePath);
                    std::filesystem::path path;
                    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".png"}))
                    {
                        m_currentMaterial->OcclusionTexturePath = path.string();
                        RemoveSubstring(m_currentMaterial->OcclusionTexturePath, "assets/");
                        auto image = Engine.Resources().Load<Image>(m_currentMaterial->OcclusionTexturePath);
                        auto sampler = std::make_shared<Sampler>();
                        m_currentMaterial->OcclusionTexture = std::make_shared<Texture>(image, sampler);
                        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->UpdateImGuiDescriptorHeap(m_currentMaterial->OcclusionTexture->Image.get(),4);

                    }
                    RenderTexturePreview(4);
                    ImGui::SliderFloat("Occlusion Texture Strength", &m_currentMaterial->OcclusionTextureStrength, 0.0f, 1.0f);
                }

              
                ImGui::Checkbox("Use Metallic Roughness Texture", &m_currentMaterial->UseMetallicRoughnessTexture);
                if(m_currentMaterial->UseMetallicRoughnessTexture)
                {
                    ImGui::InputText("Metallic texture", &m_currentMaterial->MetallicRoughnessTexturePath);
                    std::filesystem::path path;
                    if (Engine.ECS().GetSystem<AssetExplorer>().SetDragDropTarget(path, {".png"}))
                    {
                        m_currentMaterial->MetallicRoughnessTexturePath = path.string();
                        RemoveSubstring(m_currentMaterial->MetallicRoughnessTexturePath, "assets/");
                        auto image = Engine.Resources().Load<Image>(m_currentMaterial->MetallicRoughnessTexturePath);
                        auto sampler = std::make_shared<Sampler>();
                        m_currentMaterial->MetallicRoughnessTexture = std::make_shared<Texture>(image, sampler);
                        Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager()->UpdateImGuiDescriptorHeap(m_currentMaterial->MetallicRoughnessTexture->Image.get(),5);

                    }
                    RenderTexturePreview(5);

                }
                

            ImGui::Checkbox("Cast Shadows", &m_currentMaterial->ReceiveShadows);

                if(ImGui::Button("Save"))
                {
                SaveMaterial(*m_currentMaterial);
                }
        }
    
    ImGui::End();
    
}
#endif

void MaterialSystem::Render()
{
    System::Render();
    
}

void MaterialSystem::SaveMaterial(const Material& material)
{
    
    std::ofstream os(Engine.FileIO().GetPath(FileIO::Directory::Asset, "/materials/" + material.Name + ".pepimat"));
    m_currentMaterialPath = "/materials/" + material.Name + ".pepimat";
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(material));
}

void MaterialSystem::LoadMaterial(const std::string& path)
{
    if(Engine.FileIO().Exists(FileIO::Directory::Asset, path))
    {
        
    
        m_currentMaterial = Engine.Resources().Load<Material>(path);
        auto sampler = std::make_shared<Sampler>();
        const auto& resourceManager = Engine.ECS().GetSystem<RenderPipeline>().GetResourceManager();
        if(Engine.FileIO().Exists(FileIO::Directory::Asset, m_currentMaterial->BaseColorTexturePath))
        {
            m_currentMaterial->BaseColorTexture = std::make_shared<Texture>(Engine.Resources().Load<Image>(m_currentMaterial->BaseColorTexturePath), sampler);
            resourceManager->UpdateImGuiDescriptorHeap(m_currentMaterial->BaseColorTexture->Image.get(),1);
        }

        if(Engine.FileIO().Exists(FileIO::Directory::Asset, m_currentMaterial->EmissiveTexturePath))
        {
           
            m_currentMaterial->EmissiveTexture = std::make_shared<Texture>(Engine.Resources().Load<Image>(m_currentMaterial->EmissiveTexturePath), sampler);
            resourceManager->UpdateImGuiDescriptorHeap(m_currentMaterial->EmissiveTexture->Image.get(),2);

        }

        if(Engine.FileIO().Exists(FileIO::Directory::Asset, m_currentMaterial->NormalTexturePath))
        {
            m_currentMaterial->NormalTexture = std::make_shared<Texture>(Engine.Resources().Load<Image>(m_currentMaterial->NormalTexturePath), sampler);
            resourceManager->UpdateImGuiDescriptorHeap(m_currentMaterial->NormalTexture->Image.get(),3);
        }


        if(Engine.FileIO().Exists(FileIO::Directory::Asset, m_currentMaterial->OcclusionTexturePath))
        {
            m_currentMaterial->OcclusionTexture = std::make_shared<Texture>(Engine.Resources().Load<Image>(m_currentMaterial->OcclusionTexturePath), sampler);
            resourceManager->UpdateImGuiDescriptorHeap(m_currentMaterial->OcclusionTexture->Image.get(),4);
        }
        
        if(Engine.FileIO().Exists(FileIO::Directory::Asset, m_currentMaterial->MetallicRoughnessTexturePath))
        {
            m_currentMaterial->MetallicRoughnessTexture = std::make_shared<Texture>(Engine.Resources().Load<Image>(m_currentMaterial->MetallicRoughnessTexturePath), sampler);
            resourceManager->UpdateImGuiDescriptorHeap(m_currentMaterial->MetallicRoughnessTexture->Image.get(),5);
        }
    }
}



unsigned int bee::MaterialSystem::RegisterDefaultMaterial()
{
    m_materialCount++;
    return m_materialCount - 1;
}

void MaterialSystem::RenderTexturePreview(const unsigned textureID)
{
    
    
    
    if(textureID!=0)
    {
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuStartHandle(
                   Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->m_ImGui_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

        auto rtvDescriptorSize =
            Engine.ECS().GetSystem<RenderPipeline>().GetDeviceManager()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        gpuStartHandle.Offset(textureID + 1,rtvDescriptorSize);

                        
        ImGui::Image((ImTextureID)gpuStartHandle.ptr, ImVec2(256, 256));
    }
}
