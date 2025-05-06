#include "core/audio.hpp"

#include "core/engine.hpp"
#include "core/ecs.hpp"
#include "rendering/render_components.hpp"
#include "camera/camera_rts_system.hpp"
#include "core/transform.hpp"
#include "core/fileio.hpp"
#include "tools/log.hpp"
#include "math/math.hpp"
#include <fmod/fmod.h>
#include <fmod/fmod_studio.hpp>

#if defined(BEE_PLATFORM_PROSPERO)
#include <kernel.h>
#endif

using namespace bee;

Audio::Audio()
{
#if defined(BEE_PLATFORM_PROSPERO)
#ifdef DEBUG
    SceKernelModule core_mod = sceKernelLoadStartModule("/app0/sce_module/libfmodL.prx", 0, NULL, 0, NULL, NULL);
    assert(core_mod >= 0);
    SceKernelModule studio_mod = sceKernelLoadStartModule("/app0/sce_module/libfmodstudioL.prx", 0, NULL, 0, NULL, NULL);
    assert(studio_mod >= 0);
#else
    sceKernelLoadStartModule("/app0/sce_module/libfmod.prx", 0, NULL, 0, NULL, NULL);
    sceKernelLoadStartModule("/app0/sce_module/libfmodstudio.prx", 0, NULL, 0, NULL, NULL);
#endif
#endif
    Log::Info("Audio Engine: FMOD Studio by Firelight Technologies Pty Ltd.");

    // Create the Studio System object
    FMOD_RESULT result = FMOD::Studio::System::create(&m_system);
    if (result != FMOD_OK)
    {
        Log::Error("Failed to create the FMOD Studio System!");
        return;
    }

    // Initialize FMOD Studio, which will also initialize FMOD Core
    result = m_system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
    if (result != FMOD_OK)
    {
        Log::Error("Failed to initialize the FMOD Studio System!");
        return;
    }

    // Get the Core System pointer from the Studio System object
    result = m_system->getCoreSystem(&m_core_system);
    if (result != FMOD_OK)
    {
        Log::Error("Failed to get the FMOD Studio System after initialization!");
        return;
    }

    m_core_system->createSoundGroup("SFX", &m_soundGroupSFX);
    m_core_system->createSoundGroup("Music", &m_soundGroupMusic);
    m_core_system->set3DSettings(0.1f, 0.2f, 0.01f);
}

Audio::~Audio()
{
    m_soundGroupSFX->release();
    m_soundGroupMusic->release();
    
    // TODO
    //for (auto sound : sounds) sound.second->release();
    for (auto bank : m_banks) bank.second->unload();

    m_system->release();
}

void Audio::Update()
{
    m_system->update();

    // remove released events from the hashmap?
    std::vector<int> eventsToRemove;
    for (auto eventInstance : m_events)
    {
        FMOD_STUDIO_PLAYBACK_STATE state;
        eventInstance.second->getPlaybackState(&state);
        if (state == FMOD_STUDIO_PLAYBACK_STOPPED) eventsToRemove.push_back(eventInstance.first);
    }

    for (auto id : eventsToRemove) m_events.erase(id);
}

void Audio::LoadBank(const std::string& filename)
{
    // check if the bank already exists
    int hash = static_cast<int>(std::hash<std::string>{}(filename));
    if (m_banks.find(hash) != m_banks.end()) return;

    // try to load the bank
    FMOD::Studio::Bank* bank;
    const std::string& real_filename = Engine.FileIO().GetPath(FileIO::Directory::Asset, filename);
    auto result = m_system->loadBankFile(real_filename.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
    if (result != FMOD_OK)
    {
        Log::Error("FMOD bank with filename {} could not be loaded!", filename);
        return;
    }

    // load all of the bank's sample data immediately
    bank->loadSampleData();
    m_system->flushSampleLoading();  // enable this to wait for loading to finish

    // store the bank by its ID
    m_banks[hash] = bank;
}

void Audio::UnloadBank(const std::string& filename)
{
    int hash = static_cast<int>(std::hash<std::string>{}(filename));
    if (m_banks.find(hash) == m_banks.end())
    {
        Log::Error("FMOD bank with filename {} could not be unloaded!", filename);
        return;
    }

    m_banks[hash]->unload();
    m_banks.erase(hash);
}

int Audio::StartEvent(const std::string& name)
{
    // get the event description
    FMOD::Studio::EventDescription* evd;
    auto result = m_system->getEvent(("event:/" + name).c_str(), &evd);
    if (result != FMOD_OK)
    {
        Log::Error("FMOD event with name {} does not exist!", name);
        return -1;
    }

    // create an event instance
    FMOD::Studio::EventInstance* evi;
    result = evd->createInstance(&evi);
    if (result != FMOD_OK)
    {
        Log::Error("FMOD event instance with name {} could not be created!", name);
        return -1;
    }

    int eventID = m_nextEventID;
    m_events[eventID] = evi;
    ++m_nextEventID;

    // trigger the event
    result = evi->start();

    // mark it for release immediately
    result = evi->release();

    return eventID;
}

void Audio::SetParameter(const std::string& name, float value, int eventInstanceID)
{
    if (eventInstanceID < 0)
    {
        m_system->setParameterByName(name.c_str(), (float)value);
    }
    else
    {
        auto it = m_events.find(eventInstanceID);
        if (it == m_events.end())
        {
            Log::Error("FMOD event with ID {} does not exist!", eventInstanceID);
            return;
        }

        it->second->setParameterByName(name.c_str(), (float)value);
    }
}

void Audio::SetParameter(const std::string& name, const std::string& value, int eventInstanceID)
{
    if (eventInstanceID < 0)
    {
        m_system->setParameterByNameWithLabel(name.c_str(), value.c_str());
    }
    else
    {
        auto it = m_events.find(eventInstanceID);
        if (it == m_events.end())
        {
            Log::Error("FMOD event with ID {} does not exist!", eventInstanceID);
            return;
        }

        it->second->setParameterByNameWithLabel(name.c_str(), value.c_str());
    }
}

void Audio::LoadSound(const std::string& filename, bool isMusic)
{
    // check if the sound already exists
    int hash = static_cast<int>(std::hash<std::string>{}(filename));
    if (m_sounds.find(hash) != m_sounds.end()) return;

    // try to load the sound file
    FMOD_MODE mode = isMusic ? (FMOD_CREATESTREAM | FMOD_LOOP_NORMAL) : FMOD_3D;
    FMOD::Sound* sound;
    const std::string& real_filename = Engine.FileIO().GetPath(FileIO::Directory::Asset, filename);
    FMOD_RESULT result = m_core_system->createSound(real_filename.c_str(), mode, nullptr, &sound);
    if (result != FMOD_OK)
    {
        Log::Error("Sound with filename {} could not be loaded!", filename);
        return;
    }

    // attach the sound to the right group, and store it by its ID
    sound->setSoundGroup(isMusic ? m_soundGroupMusic : m_soundGroupSFX);
    m_sounds[hash] = sound;
}

int Audio::PlaySoundW(const std::string& filename, const float volume, bool scaleWithFOV)
{
    // Check if the sound exists
    int hash = static_cast<int>(std::hash<std::string>{}(filename));
    auto sound = m_sounds.find(hash);
    if (sound == m_sounds.end())
    {
        Log::Error("Sound with filename {} has not been loaded!", filename);
        return -1;
    }

    // Play the sound
    FMOD::Channel* channel;
    m_core_system->playSound(sound->second, nullptr, true, &channel);

    if (!channel)
    {
        Log::Error("Failed to play sound {}!", filename);
        return -1;
    }

    // Calculate distance between camera and point
    if (scaleWithFOV)
    {
        auto& cameraSystem = bee::Engine.ECS().GetSystem<bee::CameraSystemRTS>();
        glm::vec3 cameraPos = glm::vec3(0.0f);
        auto view = bee::Engine.ECS().Registry.view<bee::Camera, bee::Transform>();
        for (auto [entity, camera, transform] : view.each())
        {
            cameraPos = transform.Translation;
        }
        float fov = cameraSystem.GetRTSFOV();
        float scaledVolume = bee::Remap(5.0f, 25.0f, 0.0f, 0.9f, fov);
        bee::Log::Info("Scaled Volume {}", scaledVolume);
        scaledVolume = 1.0f - scaledVolume;
        bee::Log::Info("Final Volume {}", scaledVolume * volume);
        // Set 3D attributes
        channel->setVolume(scaledVolume * volume);
    }
    else
    {
        channel->setVolume(volume);
    }

    // Unpause the channel to start playback
    channel->setPaused(false);

    // Return the index of the channel on which it plays
    int channel_index;
    channel->getIndex(&channel_index);
    return channel_index;
}

int Audio::PlaySoundW(const std::string& filename, const float volume)
{
    // check if the sound exists
    int hash = static_cast<int>(std::hash<std::string>{}(filename));
    auto sound = m_sounds.find(hash);
    if (sound == m_sounds.end())
    {
        Log::Error("Sound with filename {} has not been loaded!", filename);
        return -1;
    }

    // play it
    FMOD::Channel* channel;
    m_core_system->playSound(sound->second, nullptr, false, &channel);
    channel->setVolume(volume);

    // return the index of the channel on which it plays
    int channel_index;
    channel->getIndex(&channel_index);
    return channel_index;
}

void Audio::SetChannelPaused(int channelID, bool paused)
{
    FMOD::Channel* channel;
    FMOD_RESULT result = m_core_system->getChannel(channelID, &channel);
    if (result != FMOD_OK)
    {
        Log::Error("Sound channel with ID {} does not exist!", channelID);
        return;
    }
    channel->setPaused(paused);
};

void Audio::UpdateListenerPosition(glm::vec3 listenerPos, glm::vec3 listenerVelocity, glm::vec3 listenerForward,
                                   glm::vec3 listenerUp) const
{
    const FMOD_VECTOR position = {listenerPos.x, listenerPos.y, listenerPos.z};
    const FMOD_VECTOR velocity = {listenerVelocity.x, listenerVelocity.y, listenerVelocity.z};
    const FMOD_VECTOR forward = {0, listenerForward.y, 0};
    const FMOD_VECTOR up = {0, 0, listenerUp.z * (listenerForward.y >= 0.0f ? 1.0f : -1.0f)};
    m_core_system->set3DListenerAttributes(0, &position, &velocity, &forward, &up);
};