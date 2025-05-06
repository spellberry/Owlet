#pragma once

#include <wrl.h>
using namespace Microsoft::WRL;

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

#include <algorithm>
#include <cassert>
#include <chrono>

#include "dx12/d3dx12.h"
#include "platform/dx12/DeviceManager.hpp"
#include "platform/dx12/ResourceManager.hpp"
//#include "Dx12NiceRenderer/Renderer/include/Helpers.hpp"

#include "core/ecs.hpp"
#include "core/resource.hpp"
namespace bee
{

class RenderPipeline : public System
{
public:

    
    RenderPipeline();

    ~RenderPipeline();
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif

    DeviceManager* GetDeviceManager() { return m_device_manager; };
    ResourceManager* GetResourceManager() { return m_resource_manager; };

private:
    

    void SaveSettings();
    void LoadSettings();
    void Render() override;

    DeviceManager* m_device_manager = nullptr;
    ResourceManager* m_resource_manager = nullptr;


   const FLOAT m_clear_color[4] = {0.357f, 0.678f, 0.969f,1.0f};
    bool do_init = true;

     int m_blas_count = 0;
  

     unsigned int m_draw_call_counter = 0;


   
};
}  // namespace bee