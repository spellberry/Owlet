#include "platform/pc/core/device_pc.hpp"

#include <cassert>
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "core/device.hpp"
#include "platform/dx12/DeviceManager.hpp"
#include "platform/opengl/open_gl.hpp"
#include "tools/log.hpp"

using namespace bee;

static void ErrorCallback(int error, const char* description) { fputs(description, stderr); }

void LogOpenGLVersionInfo()
{
    //  const auto vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    //   const auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    //  const auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    //  const auto shaderVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Log::Info("OpenGL Vendor {}", vendor);
    //  Log::Info("OpenGL Renderer {}", renderer);
    //  Log::Info("OpenGL Version {}", version);
    //  Log::Info("OpenGL Shader Version {}", shaderVersion);
}

Device::Device()
{
    if (!glfwInit())
    {
        Log::Critical("GLFW init failed");
        assert(false);
        exit(EXIT_FAILURE);
    }

    Log::Info("GLFW version {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(DEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif

#if defined(BEE_INSPECTOR)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#endif

    m_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(m_monitor);

    auto maxScreenWidth = mode->width;
    auto maxScreenHeight = mode->height;

    //    m_width = 1920;
    //    m_height = 1080;

    m_width = maxScreenWidth;
    m_height = maxScreenHeight;

    if (m_fullscreen)
    {
        m_width = maxScreenWidth;
        m_height = maxScreenHeight;
        m_window = glfwCreateWindow(m_width, m_height, "BEE", m_monitor, nullptr);
    }
    else
    {
        m_window = glfwCreateWindow(m_width, m_height, "BEE", nullptr, nullptr);
    }

    /* if (!m_window)
     {
         Log::Critical("GLFW window could not be created");
         glfwTerminate();
         assert(false);
         exit(EXIT_FAILURE);
     }*/

    glfwMakeContextCurrent(m_window);

    m_vsync = true;
    if (!m_vsync) glfwSwapInterval(0);

    int major = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MAJOR);
    int minor = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MINOR);
    int revision = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_REVISION);
    Log::Info("GLFW OpenGL context version {}.{}.{}", major, minor, revision);

    // if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    // {
    //    Log::Critical("GLAD failed to initialize OpenGL context");
    //    assert(false);
    //   exit(EXIT_FAILURE);
    // }

    LogOpenGLVersionInfo();
    InitDebugMessages();
}

float bee::Device::GetMonitorUIScale() const
{
    float xscale, yscale;
    glfwGetMonitorContentScale(m_monitor, &xscale, &yscale);
    return xscale;
}

void bee::Device::SetWindowName(const std::string& name) { glfwSetWindowTitle(m_window, name.c_str()); }

bee::Device::~Device() { glfwTerminate(); }

void bee::Device::Update()
{
    glfwPollEvents();
    // glfwSwapBuffers(m_window);
}

bool bee::Device::ShouldClose() { return glfwWindowShouldClose(m_window); }
void bee::Device::SetShouldClose() { glfwSetWindowShouldClose(m_window, true); }

GLFWwindow* bee::Device::GetWindow() { return m_window; }

HWND bee::Device::GetHWND() { return glfwGetWin32Window(m_window); }

void bee::Device::BeginFrame() {}
void bee::Device::EndFrame() {}
