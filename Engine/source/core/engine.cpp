#include "core/engine.hpp"

#include <chrono>
#include <iostream>

#include "core/audio.hpp"
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/fileio.hpp"
#include "core/game_base.hpp"
#include "core/input.hpp"
#include "core/resources.hpp"
#include "core/transform.hpp"
#include "rendering/debug_render.hpp"
#include "tools/inspector.hpp"
#include "tools/log.hpp"
#include "tools/profiler.hpp"
#include "tools/serialization.hpp"
#include "tools/thread_pool.hpp"
#ifdef STEAM_API_WINDOWS
#include "tools/steam_input_system.hpp"
#endif
#include "steam/steam_api.h"


using namespace bee;

// Make the engine a global variable on free store memory.
bee::EngineClass bee::Engine;

void EngineClass::Initialize()
{
    Log::Initialize();
    m_fileIO = new bee::FileIO();
    m_resources = new bee::Resources();

    if (!m_headless)
    {
        m_device = new bee::Device();
    }

    m_audio = new bee::Audio();

    if (!m_headless)
    {
        m_input = new bee::Input();
        m_debugRenderer = new bee::DebugRenderer();
        m_inspector = new bee::Inspector();
        m_serializer = new bee::Serializer();
        m_profiler = new bee::Profiler();
    }

    m_ECS = new EntityComponentSystem();
    #ifdef STEAM_API_WINDOWS
        m_steamInputSystem = new bee::SteamInputSystem();
    #endif
        m_inputWrapper = new bee::InputWrapper();
    Transform::SubscribeToEvents();
}

void EngineClass::InitializeHeadless()
{
    m_headless = true;
    Initialize();
}

#include <dxgidebug.h>

#include "platform/dx12/DeviceManager.hpp"

void EngineClass::Shutdown()
{
    Transform::UnsubscribeToEvents();
    if (m_game != nullptr)
    {
        m_game->ShutDown();
    }
    delete m_ECS;

    if (!m_headless)
    {
        delete m_profiler;
        delete m_serializer;
        delete m_inspector;
        delete m_debugRenderer;
        delete m_input;
        delete m_inputWrapper;
    }

    delete m_audio;

    if (!m_headless)
    {
        delete m_device;
    }

    delete m_resources;
    delete m_fileIO;

    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
        }
    }
#ifdef STEAM_API_WINDOWS
    delete m_steamInputSystem;
#endif
}

void EngineClass::Run()
{
    auto time = std::chrono::high_resolution_clock::now();

    

    while (!m_device->ShouldClose())
    {
        auto ctime = std::chrono::high_resolution_clock::now();
        auto elapsed = ctime - time;
        float dt = (float)((double)std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000000.0);

        if (m_game != nullptr)
        {
            m_game->Update(dt);
            m_game->Render();
        }

        #ifdef STEAM_API_WINDOWS
        m_steamInputSystem->Update();
        #else
        m_input->Update();
        #endif
        m_inputWrapper->Update();
        m_audio->Update();
        m_ECS->UpdateSystems(dt);
        m_device->BeginFrame();
        m_ECS->RenderSystems();
        m_debugRenderer->Render();
        m_ECS->RemovedDeleted();
        m_inspector->Inspect(dt);
        m_device->EndFrame();
        m_device->Update();

        time = ctime;
    }
}

ThreadPool& bee::EngineClass::ThreadPool()
{
    if (!m_pool) m_pool = new bee::ThreadPool(4);
    return *m_pool;
}
