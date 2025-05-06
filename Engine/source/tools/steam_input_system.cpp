#include "tools/steam_input_system.hpp"

#include <iostream>

#include "tools/log.hpp"

using namespace bee;
#ifdef STEAM_API_WINDOWS
bee::SteamInputSystem::SteamInputSystem()
{
    Initialize();
    Update();
}
SteamInputSystem::~SteamInputSystem()
{
    if (m_initialized)
    {
        SteamInput()->Shutdown();
        SteamAPI_Shutdown();
    }
}

void SteamInputSystem::Update()
{
    if (m_initialized)
    {
        SteamAPI_RunCallbacks();
        m_activeControllers = SteamInput()->GetConnectedControllers(m_controllerHandles);
        if (m_activeControllers > 0)
        {
            bool previouseData;
            for (auto& actionData : m_digitalActionData)
            {
                previouseData = actionData.second.rawInput.bState;
                actionData.second.rawInput =
                    SteamInput()->GetDigitalActionData(m_controllerHandles[0], m_digitalActionHandles[actionData.first]);
                actionData.second.pressedOnce = previouseData==false && previouseData!=actionData.second.rawInput.bState ;
                if (!actionData.second.rawInput.bActive)
                {
                    actionData.second.rawInput.bState = false;
                    actionData.second.pressedOnce = false;
                }
            }

            for (auto& actionData : m_analogActionData)
            {
                actionData.second =
                    SteamInput()->GetAnalogActionData(m_controllerHandles[0], m_analogActionHandles[actionData.first]);
                if (!actionData.second.bActive)
                {
                    actionData.second.x = 0.0f;
                    actionData.second.y = 0.0f;
                }
            }
        }
    }
}

void SteamInputSystem::Initialize()
{
    if (!SteamAPI_Init())
    {
        bee::Log::Error("Fatal Error - Steam must be running to play this game (SteamAPI_Init() failed).\n");
        m_initialized = false;
        return;
    }
    if (!SteamInput()->Init(false))
    {
        bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
        m_initialized = false;
        return;
    }

    //Already initialized action sets layer

    CreateActionSetLayer("InGameControls");
    std::cout << m_actionSetsLayers["InGameControls"] << "\n";
    CreateActionSetLayer("MainMenuControls");
    // Already initialized action sets

    /*CreateDigitalAction("Selection");
    CreateDigitalAction("Order");
    CreateDigitalAction("All");
    CreateDigitalAction("Up");
    CreateDigitalAction("Down");
    CreateDigitalAction("Change_Mode");
    CreateDigitalAction("Click");
    CreateDigitalAction("Press");
    CreateDigitalAction("Press_All");

    CreateAnalogAction("Camera");
    CreateAnalogAction("Menu");
    CreateAnalogAction("Ui_Movement");*/

    SteamAPI_RunCallbacks();
    m_activeControllers = SteamInput()->GetConnectedControllers(m_controllerHandles);
}

bool SteamInputSystem::IsActive() const { return m_activeControllers > 0 && m_initialized; }

bool SteamInputSystem::IsInitialized() const { return m_initialized; }

bool bee::SteamInputSystem::ControllerSelection() const
{
    if (m_initialized)
    {
        const ESteamInputType inputType = SteamInput()->GetInputTypeForHandle(m_controllerHandles[0]);
        return inputType == k_ESteamInputType_XBoxOneController || inputType == k_ESteamInputType_XBox360Controller;
    }
    bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
    return false;
}

InputAnalogActionData_t bee::SteamInputSystem::GetAnalogData(const std::string& actionName)
{
    const auto action = m_analogActionData.find(actionName);
    if (action == m_analogActionData.end())
    {
        if (!m_initialized)
            bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
        else
            bee::Log::Warn("No Digital Action with the name " + actionName + ". Try another name.");
        return InputAnalogActionData_t{};
    }
    return action->second;
}

SteamDigitalInputWrapper bee::SteamInputSystem::GetDigitalData(const std::string& actionName)
{
    const auto action = m_digitalActionData.find(actionName);
    if (action == m_digitalActionData.end())
    {
        if (!m_initialized)
            bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
        else
            bee::Log::Warn("No Digital Action with the name " + actionName + ". Try another name.");
        return SteamDigitalInputWrapper{};
    }
    return action->second;
}

void bee::SteamInputSystem::CreateAnalogAction(const std::string& actionName)
{
    if (m_initialized)
    {
        if (m_analogActionHandles.find(actionName) != m_analogActionHandles.end())
        {
            bee::Log::Warn("Action Name " + actionName + " is already used. Try another name.");
            return;
        }
        m_analogActionHandles.insert(std::pair<std::string, InputAnalogActionHandle_t>(
            actionName, SteamInput()->GetAnalogActionHandle(actionName.c_str())));
        m_analogActionData.insert(std::pair<std::string, InputAnalogActionData_t>(actionName, InputAnalogActionData_t{}));
    }
    else
    {
        bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
    }
}

void bee::SteamInputSystem::CreateDigitalAction(const std::string& actionName)
{
    if (m_initialized)
    {
        if (m_digitalActionHandles.find(actionName) != m_digitalActionHandles.end())
        {
            bee::Log::Warn("Action Name " + actionName + " is already used. Try another name.");
            return;
        }
        m_digitalActionHandles.insert(std::pair<std::string, InputDigitalActionHandle_t>(
            actionName, SteamInput()->GetDigitalActionHandle(actionName.c_str())));
        m_digitalActionData.insert(std::pair<std::string, SteamDigitalInputWrapper>(actionName, SteamDigitalInputWrapper{}));
    }
    else
    {
        bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
    }
}

void bee::SteamInputSystem::CreateActionSetLayer(const std::string& layerName)
{
    if (m_initialized)
    {
        if (m_actionSetsLayers.find(layerName) != m_actionSetsLayers.end())
        {
            bee::Log::Warn("Layer Name " + layerName + " is already used. Try another name.");
            return;
        }
        m_actionSetsLayers.insert(
            std::pair<std::string, InputActionSetHandle_t>(layerName, SteamInput()->GetActionSetHandle(layerName.c_str())));
    }
    else
    {
        bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
    }
}

void bee::SteamInputSystem::ActivateActionSetLayer(const std::string& layerName)
{
    if (m_initialized)
    {
        const auto handle = m_actionSetsLayers.find(layerName);
        if (handle == m_actionSetsLayers.end())
        {
            bee::Log::Warn("Layer Name " + layerName + " does not exist. Try another name.");
            return;
        }
        SteamInput()->ActivateActionSet(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, handle->second);
    }
    else
    {
        bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
    }
}

void bee::SteamInputSystem::DeactivateActionSetLayer(const std::string& layerName)
{
    if (m_initialized)
    {
        const auto handle = m_actionSetsLayers.find(layerName);
        if (handle == m_actionSetsLayers.end())
        {
            bee::Log::Warn("Layer Name " + layerName + " does not exist. Try another name.");
            return;
        }
        SteamInput()->ActivateActionSet(STEAM_INPUT_HANDLE_ALL_CONTROLLERS, 0);
    }
    else
    {
        bee::Log::Error("Fatal Error - Steam Input failed to initialize.\n");
    }
}

void bee::SteamInputSystem::RemoveAnalogAction(const std::string& action)
{
    const auto handle = m_analogActionHandles.find(action);
    if (handle == m_analogActionHandles.end())
    {
        bee::Log::Warn("Action Name " + action + " does not exist. Try another name.");
        return;
    }
    m_analogActionHandles.erase(handle);
    const auto data = m_analogActionData.find(action);
    if (data != m_analogActionData.end())
    m_analogActionData.erase(data);
}

void bee::SteamInputSystem::RemoveDigitalAction(const std::string& action)
{
    const auto handle = m_digitalActionHandles.find(action);
    if (handle == m_digitalActionHandles.end())
    {
        bee::Log::Warn("Action Name " + action + " does not exist. Try another name.");
        return;
    }
    m_digitalActionHandles.erase(handle);
    const auto data = m_digitalActionData.find(action);
    if (data != m_digitalActionData.end()) m_digitalActionData.erase(data);
}

void bee::SteamInputSystem::RemoveActionLayer(const std::string& layer)
{
    const auto handle = m_actionSetsLayers.find(layer);
    if (handle == m_actionSetsLayers.end())
    {
        bee::Log::Warn("Layer Name " + layer + " does not exist. Try another name.");
        return;
    }
    m_actionSetsLayers.erase(handle);
}
#endif
