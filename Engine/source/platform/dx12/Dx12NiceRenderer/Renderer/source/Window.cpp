#include "Dx12NiceRenderer/Renderer/include/Window.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>

Window::Window() 
{
    m_ClientWidth = 1920;
    m_ClientHeight = 1080;

    m_fullscreen = false;
    m_vsync = true;

}

Window::~Window() 
{ 
	glfwTerminate(); 
}

void Window::Update() 
{ 
	glfwPollEvents();
}

bool Window::InitWindow() 
{
    if (!glfwInit())
    {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(DEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif


    m_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(m_monitor);

    auto maxScreenWidth = mode->width;
    auto maxScreenHeight = mode->height;
    
  
    if (m_fullscreen)
    {
        m_ClientWidth = maxScreenWidth;
        m_ClientHeight = maxScreenHeight;
        m_window = glfwCreateWindow(m_ClientWidth, m_ClientHeight, "BEE", m_monitor, nullptr);
    }
    else
    {
        m_window = glfwCreateWindow(m_ClientWidth, m_ClientHeight, "BEE", nullptr, nullptr);
    }

    if (!m_window)
    {
       
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

  
    if (!m_vsync) glfwSwapInterval(0);
   

    return true;

}

bool Window::ShouldCloseWindow() 
{

    return glfwWindowShouldClose(m_window);
  
}

uint32_t Window::GetWidth() 
{ 
    return m_ClientWidth;
}

uint32_t Window::GetHeight() 
{ 
    return m_ClientHeight;

}
HWND Window::GetHWND() 
{ 
    return glfwGetWin32Window(m_window);
}