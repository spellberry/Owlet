#pragma once
#include <string>
#include <unordered_map>
#ifdef STEAM_API_WINDOWS
#include "steam/steam_api.h"

namespace bee
{
struct SteamDigitalInputWrapper
{
    InputDigitalActionData_t rawInput;
    bool pressedOnce = false;
};

class SteamInputSystem
{
public:
    SteamInputSystem();
    ~SteamInputSystem();
    void Update();
    void Initialize();
    bool IsActive() const;
    bool IsInitialized() const;
    bool ControllerSelection() const;

    InputAnalogActionData_t GetAnalogData(const std::string& actionName);
    SteamDigitalInputWrapper GetDigitalData(const std::string& actionName);
    void CreateAnalogAction(const std::string& actionName);
    void CreateDigitalAction(const std::string& actionName);
    void CreateActionSetLayer(const std::string& layerName);
    void ActivateActionSetLayer(const std::string& layerName);
    void DeactivateActionSetLayer(const std::string& layerName);
    void RemoveAnalogAction(const std::string& action);
    void RemoveDigitalAction(const std::string& action);
    void RemoveActionLayer(const std::string& layer);

private:
    std::unordered_map<std::string, SteamDigitalInputWrapper> m_digitalActionData{};
    std::unordered_map<std::string, InputAnalogActionData_t> m_analogActionData{};

    InputHandle_t m_controllerHandles[STEAM_CONTROLLER_MAX_COUNT]{};
    std::unordered_map<std::string, InputDigitalActionHandle_t> m_digitalActionHandles{};
    std::unordered_map<std::string, InputAnalogActionHandle_t> m_analogActionHandles{};
    std::unordered_map<std::string, InputActionSetHandle_t> m_actionSetsLayers{};

    int m_activeControllers = 0;
    bool m_initialized = true;
};
}  // namespace bee
#endif