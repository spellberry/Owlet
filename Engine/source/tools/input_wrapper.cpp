#include "tools/input_wrapper.hpp"
#include "core/engine.hpp"
#include <fstream>
#ifdef STEAM_API_WINDOWS
#include "tools/steam_input_system.hpp"
#endif
#include <cereal/archives/json.hpp>

#include "tools/log.hpp"
#include "core/fileio.hpp"

#include "imgui/crude_json.h"
#include "actors/units/unit_order_type.hpp"
#include "magic_enum/magic_enum.hpp"


using namespace bee;
// credit to stack overflow user:
// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bee::InputWrapper::InputWrapper()
{
    if (m_loadActionBiding)
        LoadActionBinds(pathToActionBinds);
    else
        Initialize();
    Update();
}
void InputWrapper::CreateDigitalAction(const std::string& action, int key)
{
#ifdef STEAM_API_WINDOWS
    bee::Engine.SteamInputSystem().CreateDigitalAction(action);
    m_digitalActionMapping.push_back(action);
    m_digitalActionData.insert(std::pair<std::string, DigitalData>(action, DigitalData{}));
#else
    if (key < static_cast<int>(magic_enum::enum_value<Input::KeyboardKey>(0)))
    {
        bee::Log::Warn("There is no key to bind to the action " + action);
        return;
    }
    m_digitalActionMapping.insert(std::pair<std::string, int>(action, key));
    m_digitalActionData.insert(std::pair<std::string, DigitalData>(action, DigitalData{}));
#endif
}

void bee::InputWrapper::CreateAnalogAction(const std::string& action, AnalogTypes type)
{
#ifdef STEAM_API_WINDOWS
    bee::Engine.SteamInputSystem().CreateAnalogAction(action);
    m_analogActionMapping.push_back(action);
    m_analogActionData.insert(std::pair<std::string, glm::vec2>(action, glm::vec2{}));
#else
    if (type == AnalogTypes::none)
    {
        bee::Log::Warn("There is no input to bind to the action " + action);
        return;
    }
    m_analogActionMapping.insert(std::pair<std::string, AnalogTypes>(action, type));
    m_analogActionData.insert(std::pair<std::string, glm::vec2>(action, glm::vec2{0.0f,0.0f}));
#endif
}
#ifndef STEAM_API_WINDOWS
void bee::InputWrapper::RemapeDigitalAction(const std::string& action, int key)
{
    if (m_digitalActionMapping.find(action) == m_digitalActionMapping.end())
    {
        bee::Log::Warn("There is no action with the name " + action);
        return;
    }
    if (key < static_cast<int>(magic_enum::enum_value<Input::KeyboardKey>(0)))
    {
        bee::Log::Warn("There is no key to bind to the action " + action);
        return;
    }
    m_digitalActionMapping[action]= key;
}

void bee::InputWrapper::RemapeAnalogAction(const std::string& action, AnalogTypes type)
{
    if (m_analogActionMapping.find(action) == m_analogActionMapping.end())
    {
        bee::Log::Warn("There is no action with the name " + action);
        return;
    }
    if (type == AnalogTypes::none)
    {
        bee::Log::Warn("There is no input to bind to the action " + action);
        return;
    }
    m_analogActionMapping[action] = type;
}

bee::Input::KeyboardKey bee::InputWrapper::GetDigitalActionKey(const std::string& action)
{
     if (m_digitalActionMapping.find(action) == m_digitalActionMapping.end())
     {
         bee::Log::Warn("There is no action with the name " + action);
         return bee::Input::KeyboardKey::None;
     }
     return static_cast<bee::Input::KeyboardKey>(m_digitalActionMapping[action]);
}
#endif
DigitalData InputWrapper::GetDigitalData(const std::string& action)
{
    if (m_digitalActionData.find(action) == m_digitalActionData.end())
    {
        bee::Log::Warn("No Digital Action with the name " + action + ". Try another name.");
        return DigitalData{};
    }
    return m_digitalActionData[action];
}

glm::vec2 bee::InputWrapper::GetAnalogData(const std::string& action)
{
    if (m_analogActionData.find(action) == m_analogActionData.end())
    {
        bee::Log::Warn("No Digital Action with the name " + action + ". Try another name.");
        return glm::vec2{0.0f,0.0f};
    }
    return m_analogActionData[action];
}

void bee::InputWrapper::RemoveAnalogAction(const std::string& action)
{
#ifdef STEAM_API_WINDOWS
    m_analogActionMapping.erase(std::remove(m_analogActionMapping.begin(), m_analogActionMapping.end(), action),
                                m_analogActionMapping.end());
    const auto data = m_analogActionData.find(action);
    if (data != m_analogActionData.end()) m_analogActionData.erase(data);
    bee::Engine.SteamInputSystem().RemoveAnalogAction(action);
#else
    const auto handle = m_analogActionMapping.find(action);
    if (handle == m_analogActionMapping.end())
    {
        bee::Log::Warn("Action Name " + action + " does not exist. Try another name.");
        return;
    }
    m_analogActionMapping.erase(handle);
    const auto data = m_analogActionData.find(action);
    if (data != m_analogActionData.end()) m_analogActionData.erase(data);
#endif
}

void bee::InputWrapper::RemoveDigitalAction(const std::string& action)
{
#ifdef STEAM_API_WINDOWS
    m_digitalActionMapping.erase(std::remove(m_digitalActionMapping.begin(), m_digitalActionMapping.end(), action),
                                 m_digitalActionMapping.end());
    const auto data = m_digitalActionData.find(action);
    if (data != m_digitalActionData.end()) m_digitalActionData.erase(data);
    bee::Engine.SteamInputSystem().RemoveDigitalAction(action);
#else
    const auto handle = m_digitalActionMapping.find(action);
    if (handle == m_digitalActionMapping.end())
    {
        bee::Log::Warn("Action Name " + action + " does not exist. Try another name.");
        return;
    }
    m_digitalActionMapping.erase(handle);
    const auto data = m_digitalActionData.find(action);
    if (data != m_digitalActionData.end()) m_digitalActionData.erase(data);
#endif
}

void InputWrapper::Update()
{
    for (auto& digitalAction : m_digitalActionMapping)
    {
#ifdef STEAM_API_WINDOWS
        m_digitalActionData[digitalAction].pressed =
            bee::Engine.SteamInputSystem().GetDigitalData(digitalAction).rawInput.bState;
        m_digitalActionData[digitalAction].pressedOnce =
            bee::Engine.SteamInputSystem().GetDigitalData(digitalAction).pressedOnce;
#else
        if (digitalAction.second > 3)
        {
            m_digitalActionData[digitalAction.first].pressed =
                bee::Engine.Input().GetKeyboardKey(static_cast<bee::Input::KeyboardKey>(digitalAction.second));
            m_digitalActionData[digitalAction.first].pressedOnce =
                bee::Engine.Input().GetKeyboardKeyOnce(static_cast<bee::Input::KeyboardKey>(digitalAction.second));
        }
        else
        {
            m_digitalActionData[digitalAction.first].pressed =
                bee::Engine.Input().GetMouseButton(static_cast<bee::Input::MouseButton>(digitalAction.second));
            m_digitalActionData[digitalAction.first].pressedOnce =
                bee::Engine.Input().GetMouseButtonOnce(static_cast<bee::Input::MouseButton>(digitalAction.second));
        }
#endif
    }

    for (auto& analogAction : m_analogActionMapping)
    {
#ifdef STEAM_API_WINDOWS
        m_analogActionData[analogAction].x = bee::Engine.SteamInputSystem().GetAnalogData(analogAction).x;
        m_analogActionData[analogAction].y = bee::Engine.SteamInputSystem().GetAnalogData(analogAction).y;
#else
        switch (analogAction.second)
        {
            case AnalogTypes::mouse:
            {
                glm::vec2 mousePos = bee::Engine.Input().GetMousePosition();
                m_analogActionData[analogAction.first].x = mousePos.x;
                m_analogActionData[analogAction.first].y = mousePos.y;
                break;
            }
            case AnalogTypes::mouseWheel:
            {
                m_analogActionData[analogAction.first].x = bee::Engine.Input().GetMouseWheel();
                break;
            }
            case AnalogTypes::mouseWheelCamera:
            {
                m_analogActionData[analogAction.first].x = bee::Engine.Input().GetMouseWheelCamera();
                break;
            }
            case AnalogTypes::mouseWheelMax:
            {
                m_analogActionData[analogAction.first].x = bee::Engine.Input().GetMouseWheelMax();
                break;
            }
            case AnalogTypes::mouseWheelMin:
            {
                m_analogActionData[analogAction.first].x = bee::Engine.Input().GetMouseWheelMin();
                break;
            }
        }
#endif
    }
    for (auto& analogAction : m_staticAnalogActionMapping)
    {
        switch (analogAction.second)
        {
            case AnalogTypes::mouse:
            {
                glm::vec2 mousePos = bee::Engine.Input().GetMousePosition();
                m_analogActionData[analogAction.first].x = mousePos.x;
                m_analogActionData[analogAction.first].y = mousePos.y;
                break;
            }
            case AnalogTypes::mouseWheel:
            {
                m_analogActionData[analogAction.first].x = bee::Engine.Input().GetMouseWheel();
                break;
            }
            case AnalogTypes::mouseWheelCamera:
            {
                m_analogActionData[analogAction.first].x = bee::Engine.Input().GetMouseWheelCamera();
                break;
            }
            case AnalogTypes::mouseWheelMax:
            {
                m_analogActionData[analogAction.first].x = bee::Engine.Input().GetMouseWheelMax();
                break;
            }
            case AnalogTypes::mouseWheelMin:
            {
                m_analogActionData[analogAction.first].x = bee::Engine.Input().GetMouseWheelMin();
                break;
            }
        }
    }
}

void bee::InputWrapper::LoadActionBinds(const std::string& loadPath)
{
    if (fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, loadPath + ".json")))
    {
        std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, loadPath + ".json"));
        cereal::JSONInputArchive archive(is);

        archive(CEREAL_NVP(m_digitalActionMapping), CEREAL_NVP(m_analogActionMapping),CEREAL_NVP(m_staticAnalogActionMapping));

        for (auto& data:m_digitalActionMapping)
        {
            m_digitalActionData.emplace(std::pair<std::string, DigitalData>(data.first, DigitalData{}));
        }
        for (auto& data : m_analogActionMapping)
        {
            m_analogActionData.emplace(std::pair<std::string, glm::vec2>(data.first, glm::vec2(0.0f)));
        }
        for (auto& data : m_staticAnalogActionMapping)
        {
            m_analogActionData.emplace(std::pair<std::string, glm::vec2>(data.first, glm::vec2(0.0f)));
        }
        
    }
}

void bee::InputWrapper::SaveActionBinds(const std::string& savePath)
{
    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, savePath + ".json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(m_digitalActionMapping), CEREAL_NVP(m_analogActionMapping),CEREAL_NVP(m_staticAnalogActionMapping));
}

void bee::InputWrapper::Initialize()
{
#ifdef STEAM_API_WINDOWS
    CreateDigitalAction("Selection");
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
    CreateAnalogAction("Ui_Movement");
#else
    CreateDigitalAction("Selection", static_cast<int>(Input::MouseButton::Left));
    CreateDigitalAction("Order", static_cast<int>(Input::MouseButton::Right));
    CreateDigitalAction("All", static_cast<int>(Input::KeyboardKey::LeftShift));
    CreateDigitalAction("Up", static_cast<int>(Input::KeyboardKey::Q));
    CreateDigitalAction("Down", static_cast<int>(Input::KeyboardKey::E));

    // Orders Button
    {
        // Unit orders
        CreateDigitalAction("None", static_cast<int>(Input::KeyboardKey::Digit0));
        CreateDigitalAction("Move", static_cast<int>(Input::KeyboardKey::Digit1));
        CreateDigitalAction("OffensiveMove", static_cast<int>(Input::KeyboardKey::Digit2));
        CreateDigitalAction("Patrol", static_cast<int>(Input::KeyboardKey::Digit3));
        CreateDigitalAction("Attack", static_cast<int>(Input::KeyboardKey::Digit4));

        // Build Orders
        CreateDigitalAction("BuildBase", static_cast<int>(Input::KeyboardKey::None));
        CreateDigitalAction("BuildMageTower", static_cast<int>(Input::KeyboardKey::None));
        CreateDigitalAction("BuildBarracks", static_cast<int>(Input::KeyboardKey::None));
        CreateDigitalAction("BuildWallStart", static_cast<int>(Input::KeyboardKey::R));
        CreateDigitalAction("BuildWallEnd", static_cast<int>(Input::KeyboardKey::None));

        CreateDigitalAction("BuildFenceStart", static_cast<int>(Input::KeyboardKey::F));
        CreateDigitalAction("BuildFenceEnd", static_cast<int>(Input::KeyboardKey::None));

        // Structure Build Orders
        CreateDigitalAction("TrainWarrior", static_cast<int>(Input::KeyboardKey::None));
        CreateDigitalAction("TrainMage", static_cast<int>(Input::KeyboardKey::None));
        CreateDigitalAction("UpgradeBuilding", static_cast<int>(Input::KeyboardKey::None));
    }

    // RTS Camera
    {
        CreateDigitalAction("Forward", static_cast<int>(bee::Input::KeyboardKey::W));
        CreateDigitalAction("Backward", static_cast<int>(bee::Input::KeyboardKey::S));
        CreateDigitalAction("Left", static_cast<int>(bee::Input::KeyboardKey::A));
        CreateDigitalAction("Right", static_cast<int>(bee::Input::KeyboardKey::D));
    }

    CreateAnalogAction("Wheel",AnalogTypes::mouseWheel);
#endif
    SetStaticAnalogAction("WheelMin", AnalogTypes::mouseWheelMin);
    SetStaticAnalogAction("WheelMax", AnalogTypes::mouseWheelMax);
    SetStaticAnalogAction("CameraWheel", AnalogTypes::mouseWheelCamera);
    SetStaticAnalogAction("Mouse", AnalogTypes::mouse);
}

void bee::InputWrapper::SetStaticAnalogAction(const std::string& action, AnalogTypes type)
{
    if (type == AnalogTypes::none)
    {
        bee::Log::Warn("There is no input to bind to the action " + action);
        return;
    }
    m_staticAnalogActionMapping.insert(std::pair<std::string, AnalogTypes>(action, type));
    m_analogActionData.insert(std::pair<std::string, glm::vec2>(action, glm::vec2{0.0f, 0.0f}));
}
