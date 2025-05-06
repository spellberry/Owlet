#include "Dx12NiceRenderer/include/Dx12NiceRenderer.hpp"


#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx12.h>
#include<imgui/imgui_impl_glfw.h>

#include "Dx12NiceRenderer/Renderer/include/Window.hpp"
#include"Dx12NiceRenderer/Renderer/include/DeviceManager.hpp"
#include"Dx12NiceRenderer/Renderer/include/RenderPipeline.hpp"
#include"Dx12NiceRenderer/Renderer/include/ResourceManager.hpp"

#include<iostream>
#include <DirectXMath.h>

#include"Dx12NiceRenderer/Renderer/include/Helpers.hpp"

namespace Dx12NiceRenderer
{

    Window* glfw_window = nullptr;
    DeviceManager* device_manager = nullptr;
   // RenderPipeline* render_pipeline = nullptr;
    ResourceManager* resource_manager = nullptr;

    DirectX::XMFLOAT4X4 view_matrix;
    DirectX::XMFLOAT4X4 projection_matrix;

    bool created_window = false;
    bool updated_window = true;
    bool finish_resource_init = true;
    bool renderer_is_initialized = false;
    bool added_model = false;
    bool frame_started = false;
    bool rendered = false;
    bool updated_models = false;


    namespace imgui
    {
        bool InitImGui();

        void BeginImGuiFrame();

        void RenderImGui();

        void DestroyImGui();
    } 
}

namespace debugging
{
    //------------------------Check box values for visual debugging
    //--These are passed to the shaders in a constant buffer
bool base_color = true;
bool normal_map = true;
bool metal_map = true;
bool roughness_map = true;
bool ambient_occlusion_map=true;
bool emissive = true;
bool normals;
bool tangents;
bool bitangents;
bool only_normal_map;
bool only_metal;
bool only_roughness;
bool showSpecular;
bool showDiffuse;
bool showFresnel;


}


 bool Dx12NiceRenderer::window::InitWindow()
{ 
	glfw_window = new Window;
    created_window = true;
    return glfw_window->InitWindow();
}

void Dx12NiceRenderer::window::UpdateWindow() 
{ 
	glfw_window->Update();
    updated_window = true;
}

void Dx12NiceRenderer::window::TerminateWindow() 
{ 
	delete glfw_window;
    glfw_window = nullptr;
}

bool Dx12NiceRenderer::window::ShouldCloseWindow() 
{
	return glfw_window->ShouldCloseWindow(); 
}

uint32_t Dx12NiceRenderer::window::GetWidth() { return glfw_window->GetWidth(); }

uint32_t Dx12NiceRenderer::window::GetHeight() { return glfw_window->GetHeight(); }

GLFWwindow* Dx12NiceRenderer::window::GetWindow() { return glfw_window->GetWindowHandle(); }


bool Dx12NiceRenderer::renderer::InitRenderer()
{ 
   
    try
    {
        if (!created_window)
        {
            throw std::runtime_error("You have no window!");
        }
    }
    catch (const std::runtime_error& e)
    {
        Dx12NiceRenderer::window::InitWindow(); 
        std::cerr << "You forgot something: " << e.what() << std::endl;
    }
      
    //device_manager = new DeviceManager(glfw_window);

     
    Dx12NiceRenderer::imgui::InitImGui();

    resource_manager = new ResourceManager(device_manager);
      
   // render_pipeline = new RenderPipeline(device_manager, resource_manager);

    finish_resource_init = true;

    renderer_is_initialized = true;


      return true;
   
}
using namespace debugging;
void Dx12NiceRenderer::renderer::BeginFrame() 
{ 
  
   /* if (!added_model)
    {  
        throw std::runtime_error("You have no model!"); 
    }*/
 
    if (finish_resource_init)
    {
        resource_manager->FinishResourceInit();
        device_manager->FinishInit();
        finish_resource_init = false;
    }
    
     try
    {
        if (frame_started)
        {
            throw std::runtime_error("You did not end the frame!");
        }
    }
    catch (const std::runtime_error& e)
    {
        EndFrame();
        std::cerr << "You forgot something: " << e.what() << std::endl;
    }

     
    resource_manager->SetDebugBuffer(0, base_color);
    resource_manager->SetDebugBuffer(1, normal_map);
    resource_manager->SetDebugBuffer(2, metal_map);
    resource_manager->SetDebugBuffer(3, roughness_map);
    resource_manager->SetDebugBuffer(4, ambient_occlusion_map);
    resource_manager->SetDebugBuffer(5, emissive);
    resource_manager->SetDebugBuffer(6, normals);
    resource_manager->SetDebugBuffer(7, showSpecular);
    resource_manager->SetDebugBuffer(8, showDiffuse);
    resource_manager->SetDebugBuffer(9, only_normal_map);
    resource_manager->SetDebugBuffer(10, only_metal);
    resource_manager->SetDebugBuffer(11, only_roughness);
    resource_manager->SetDebugBuffer(12, showFresnel);
    resource_manager->SetDebugBuffer(13, tangents);
    resource_manager->SetDebugBuffer(14, bitangents);


    Dx12NiceRenderer::imgui::BeginImGuiFrame();

    resource_manager->BeginFrame(view_matrix, projection_matrix);
    device_manager->BeginFrame();

    frame_started = true;

}

void Dx12NiceRenderer::renderer::Render(glm::mat4 view, glm::mat4 proj) 
{
   
    try
    {
        if (!frame_started)
        {
            throw std::runtime_error("You did not begin the frame!");
        }
    }
    catch (const std::runtime_error& e)
    {
        BeginFrame();
        std::cerr << "You forgot something: " << e.what() << std::endl;
    }

    try
    {
        if (!updated_models)
        {
            throw std::runtime_error("You forgot to update your models!");
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "You forgot something: " << e.what() << std::endl;
    }

    if (created_window)
    {
        try
        {
            if (!updated_window)
            {
                throw std::runtime_error("You forgot to update your window!");
            }
        }
        catch (const std::runtime_error& e)
        {
            Dx12NiceRenderer::window::UpdateWindow();
            std::cerr << "You forgot something: " << e.what() << std::endl;
        }

        updated_window = false;
    }
  


    DirectX::XMStoreFloat4x4(&view_matrix, ConvertGLMToDXMatrix(view));
    DirectX::XMStoreFloat4x4(&projection_matrix, ConvertGLMToDXMatrix(proj));

 //   render_pipeline->Render();
    rendered = true;
    updated_models = false;
}

void Dx12NiceRenderer::renderer::EndFrame() 
{ 
   try
   {
       if (!rendered)
        {
            throw std::runtime_error("You forgot to render!");
        }
   }
    
   catch (const std::runtime_error& e)
   {
        Render(glm::mat4(0), glm::mat4(0)); 
        std::cerr << "You forgot something: " << e.what() << std::endl;
   }

    Dx12NiceRenderer::imgui::RenderImGui();
    device_manager->EndFrame();
    rendered = false;
    frame_started = false;
}

#include <dxgidebug.h>

void Dx12NiceRenderer::renderer::DestroyRenderer() 
{ 
    device_manager->FlushBuffers();
    
   // delete render_pipeline;
  
    delete resource_manager;
   
    delete device_manager; 

    Dx12NiceRenderer::imgui::DestroyImGui(); 

    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
        }
    }

  
}

void Dx12NiceRenderer::renderer::EnableRayTracing(bool render) 
{ 
 //   render_pipeline->EnableRayTracing(render);
}


bool Dx12NiceRenderer::imgui::InitImGui()
{
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 1; 
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device_manager->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&device_manager->m_ImGui_DescriptorHeap));
    }

    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOther(Dx12NiceRenderer::glfw_window->GetWindowHandle(), true);
    ImGui_ImplDX12_Init(device_manager->GetDevice().Get(), 3, DXGI_FORMAT_R8G8B8A8_UNORM,
                        device_manager->m_ImGui_DescriptorHeap.Get(), 
                        device_manager->m_ImGui_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                        device_manager->m_ImGui_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());


      ImGuiIO& io = ImGui::GetIO();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

      return true;

}


void Dx12NiceRenderer::imgui::BeginImGuiFrame() 
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

 
}

void Dx12NiceRenderer::imgui::RenderImGui() 
{

 

    ImGui::Begin("PBR Debugging"); 
   
     ImGui::Checkbox("Use Base Color", &base_color);
   
     ImGui::Checkbox("Use Normal Map", &normal_map);
   
     ImGui::Checkbox("Use Metal Map", &metal_map);
    
      ImGui::Checkbox("Use Roughness Map", &roughness_map);

     ImGui::Checkbox("Use Ambient Occlusion", &ambient_occlusion_map);
    
     ImGui::Checkbox("Use Emissive", &emissive);
   

      if (ImGui::Checkbox("Only Normals", &normals))
      {
          if (normals)
          {
              only_normal_map = only_metal = only_roughness = tangents = bitangents = false;
          }
      }
      if (ImGui::Checkbox("Only Tangents", &tangents))
      {
          if (tangents)
          {
              only_normal_map = only_metal = only_roughness = normals = bitangents = false;
          }
      }
      if (ImGui::Checkbox("Only Bitangents", &bitangents))
      {
          if (bitangents)
          {
              only_normal_map = only_metal = only_roughness = tangents = normals = false;
          }
      }
     if (ImGui::Checkbox("Only Normal Map", &only_normal_map))
     {
         if (only_normal_map)
         {
             normals = only_metal = only_roughness = false;
         }
      
     }

     if (ImGui::Checkbox("Only Metallic Map", &only_metal))  
     {
         if (only_metal)
         {
             normals = only_normal_map = false;
         }
         
     }
     if (ImGui::Checkbox("Only Roughness Map", &only_roughness))
     {
         if (only_roughness)
         {
             normals = only_normal_map = false;
         }
     }

  ImGui::End(); 


 /*    if (ImGui::Checkbox("Show Fresnel", &showFresnel))
     {
         if (showFresnel)
         {
             normals = only_normal_map = false;
         } 
     }*/



  


   

    
    ID3D12DescriptorHeap* imguiDescriptorHeaps[] = {device_manager->m_ImGui_DescriptorHeap.Get()};
    device_manager->GetCommandList()->SetDescriptorHeaps(_countof(imguiDescriptorHeaps), imguiDescriptorHeaps);

    ImGui::Render();

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), device_manager->GetCommandList().Get());


 

}

void Dx12NiceRenderer::imgui::DestroyImGui() 
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(); 
}


int Dx12NiceRenderer::resourceManager::CreateModel(const std::string& file_path)
{
    try
    {
        if (!renderer_is_initialized)
        {
            throw std::runtime_error("You did not initialize the renderer!");
        }
    }
    catch (const std::runtime_error& e)
    {
        Dx12NiceRenderer::renderer::InitRenderer(); 
        std::cerr << "You forgot something: " << e.what() << std::endl;
    }
   
    added_model = true;

    return resource_manager->AddModel(file_path);
}


void Dx12NiceRenderer::resourceManager::UpdateModel(const int handle,glm::mat4 transform)
{ 
    DirectX::XMFLOAT4X4 temp_mat;
    DirectX::XMStoreFloat4x4(&temp_mat, ConvertGLMToDXMatrix(transform));
  

    resource_manager->UpdateObject(handle,temp_mat);
    updated_models = true;
}
