#pragma once
#include <memory>

#include "tools/input_wrapper.hpp"

namespace bee
{
class GameBase;

class EntityComponentSystem;
class FileIO;
class Resources;
class Device;
class Input;
class Audio;
class DebugRenderer;
class Inspector;
class Serializer;
class Profiler;
class ThreadPool;
class SteamInputSystem;
class EngineClass
{
public:
    void InitializeHeadless();
    void Initialize();
    void Shutdown();
    void Run();

    FileIO& FileIO() { return *m_fileIO; }
    Resources& Resources() { return *m_resources; }
    Device& Device() { return *m_device; }
    Input& Input() { return *m_input; }
    Audio& Audio() { return *m_audio; }
#ifdef STEAM_API_WINDOWS
    SteamInputSystem& SteamInputSystem() { return *m_steamInputSystem; }
#endif
    InputWrapper& InputWrapper() { return *m_inputWrapper; }
    DebugRenderer& DebugRenderer() { return *m_debugRenderer; }
    Inspector& Inspector() { return *m_inspector; }
    Serializer& Serializer() { return *m_serializer; }
    Profiler& Profiler() { return *m_profiler; }
    EntityComponentSystem& ECS() { return *m_ECS; }
    ThreadPool& ThreadPool();  // Thread pool does lazy initialization
    GameBase& Game() { return *m_game; }

    template <typename T>
    void SetGame()
    {
        static_assert(std::is_base_of_v<GameBase, T>, "T must inherit from GameBase");
        if (m_game.get() != nullptr)
        {
            m_game.get()->ShutDown();
            m_game.reset();
        }
        m_game = std::make_unique<T>();
        if (m_game != nullptr)
        {
            m_game.get()->Init();
        }
    }

    bool IsHeadless() const { return m_headless; }
    bool& WasPreInitialized() { return m_preInitialized; }

private:
    bool m_headless = false;
    std::unique_ptr<bee::GameBase> m_game{};
    bee::FileIO* m_fileIO = nullptr;
    bee::Resources* m_resources = nullptr;
    bee::Device* m_device = nullptr;
    bee::DebugRenderer* m_debugRenderer = nullptr;
    bee::Input* m_input = nullptr;
    bee::Audio* m_audio = nullptr;
    bee::Inspector* m_inspector = nullptr;
    bee::Serializer* m_serializer = nullptr;
    bee::Profiler* m_profiler = nullptr;
    bee::ThreadPool* m_pool = nullptr;
#ifdef STEAM_API_WINDOWS
    bee::SteamInputSystem* m_steamInputSystem = nullptr;
#endif
    bee::InputWrapper* m_inputWrapper = nullptr;
    EntityComponentSystem* m_ECS = nullptr;
    bool m_preInitialized = false;
};

extern EngineClass Engine;

}  // namespace bee