#pragma once
#include "cereal/cereal.hpp"
#include "cereal/types/unordered_map.hpp"

#include "unordered_map"
#include "string"
#include "vector"
#include "core/input.hpp"

namespace bee
{
enum class AnalogTypes
{
    none = 0,
    mouse = 1,
    mouseWheel = 2,
    mouseWheelCamera = 3,
    mouseWheelMax = 4,
    mouseWheelMin = 5,
};

struct DigitalData
{
    bool pressed = false;
    bool pressedOnce = false;
};

class InputWrapper
{
public:
    InputWrapper();
    void CreateDigitalAction(const std::string& action, int key = -1);
    void CreateAnalogAction(const std::string& action, AnalogTypes type = AnalogTypes::none);
#ifndef STEAM_API_WINDOWS
    void RemapeDigitalAction(const std::string& action, int key = -1);
    void RemapeAnalogAction(const std::string& action, AnalogTypes type = AnalogTypes::none);
    bee::Input::KeyboardKey  GetDigitalActionKey(const std::string& action);
#endif
    DigitalData GetDigitalData(const std::string& action);
    glm::vec2 GetAnalogData(const std::string& action);
    void RemoveAnalogAction(const std::string& action);
    void RemoveDigitalAction(const std::string& action);
    void Update();
    void LoadActionBinds(const std::string& loadPath);
    void SaveActionBinds(const std::string& savePath);

    const std::string pathToActionBinds = "keybinds";
private:
    void Initialize();
    void SetStaticAnalogAction(const std::string& action, AnalogTypes type = AnalogTypes::none);

    const bool m_loadActionBiding = true;

    std::unordered_map<std::string, DigitalData> m_digitalActionData;
    std::unordered_map<std::string, glm::vec2> m_analogActionData;

    
#ifdef STEAM_API_WINDOWS
    std::vector<std::string> m_digitalActionMapping;
    std::vector<std::string> m_analogActionMapping;
#else
    std::unordered_map<std::string, int> m_digitalActionMapping;
    std::unordered_map<std::string, AnalogTypes> m_analogActionMapping;

    template <class Archive>
    void serialize(Archive& archive)
    {
        CEREAL_NVP(m_digitalActionMapping), CEREAL_NVP(m_analogActionMapping),CEREAL_NVP(m_staticAnalogActionMapping);
    }
#endif
    std::unordered_map<std::string, AnalogTypes> m_staticAnalogActionMapping;
};
}  // namespace bee