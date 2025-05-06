#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include <string>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

struct GLFWwindow;
struct GLFWmonitor;

namespace bee
{

class Device
{
public:
    bool ShouldClose();
    void SetShouldClose();
    GLFWwindow* GetWindow();
    GLFWmonitor* GetMonitor() { return m_monitor; };
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    void BeginFrame();
    void EndFrame();
    void SetFullScreen(bool toggle) { m_fullscreen = toggle; }

    void SetWidth(int width) { m_width = width; }
    void SetHeight(int height) { m_height = height; }

    float GetMonitorUIScale() const;
    void SetWindowName(const std::string& name);
    HWND GetHWND();

private:
    friend class EngineClass;
    Device();
    ~Device();
    //  void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    void Update();

    GLFWwindow* m_window = nullptr;
    GLFWmonitor* m_monitor = nullptr;
    bool m_vsync = true;
    bool m_fullscreen = false;
    int m_width = -1;
    int m_height = -1;
};

}  // namespace bee
