#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> 

#include <algorithm>
#include <cassert>
#include <chrono>
#include "Dx12NiceRenderer/include/Dx12NiceRenderer.hpp"

 struct GLFWwindow;
 struct GLFWmonitor;




class Window
{

public:
    friend class DeviceManager;
    friend class RenderPipeline;
    friend class ResourceManager;

    friend bool Dx12NiceRenderer::window::InitWindow();
    friend void Dx12NiceRenderer::window::UpdateWindow();
    friend void Dx12NiceRenderer::window::TerminateWindow();
    friend bool Dx12NiceRenderer::window::ShouldCloseWindow();
    friend uint32_t Dx12NiceRenderer::window::GetWidth();
    friend uint32_t Dx12NiceRenderer::window::GetHeight();
    
     
    GLFWwindow* GetWindowHandle()
    { 
        return m_window;
    }

private:

  
    Window();
    ~Window();
    void Update();
    bool InitWindow();  
    bool ShouldCloseWindow();


  

    HWND GetHWND();
    uint32_t GetWidth();
    uint32_t GetHeight();


    uint32_t m_ClientWidth= 1920;
        
    uint32_t m_ClientHeight= 1080 ;
      
    GLFWwindow* m_window = nullptr;
        
    GLFWmonitor* m_monitor = nullptr;
 
    bool m_vsync = true;
       
    bool m_fullscreen = true;
     

       
};