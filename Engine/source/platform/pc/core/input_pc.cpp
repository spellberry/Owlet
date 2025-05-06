#include <GLFW/glfw3.h>

#include "core/device.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"

using namespace bee;

class Input::Impl
{
};

enum KeyAction
{
    Release = 0,
    Press = 1,
    None = 2
};

constexpr int nr_keys = 350;
bool keys_down[nr_keys];
bool prev_keys_down[nr_keys];
KeyAction keys_action[nr_keys];

constexpr int nr_mousebuttons = 8;
bool mousebuttons_down[nr_mousebuttons];
bool prev_mousebuttons_down[nr_mousebuttons];
KeyAction mousebuttons_action[nr_mousebuttons];

constexpr int max_nr_gamepads = 4;
bool gamepad_connected[max_nr_gamepads];
GLFWgamepadstate gamepad_state[max_nr_gamepads];
GLFWgamepadstate prev_gamepad_state[max_nr_gamepads];

glm::vec2 mousepos;
float mousewheel = 0;
// The value of the mouse scroll, mainly used for the FOV of the camera
// Could make a separate variable that is used only for the camera
// moved defenition to input.hpp for compatibily with prospero
float mouseWheelCamera = MOUSEWHEELCAMERA;
float mouseWheelCameraMin = MOUSEWHEELCAMERAMIN;
float mouseWheelCameraMax = MOUSEWHEELCAMERAMAX;
float zoomSpeed = ZOOMSPEED;
bool paused = false;

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    mousepos.x = (float)xpos;
    mousepos.y = (float)ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!paused)
    {
        mousewheel += (float)yoffset;
        mouseWheelCamera -= (float)yoffset * zoomSpeed;

        if (mouseWheelCamera > mouseWheelCameraMax) mouseWheelCamera = mouseWheelCameraMax;
        if (mouseWheelCamera < mouseWheelCameraMin) mouseWheelCamera = mouseWheelCameraMin;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) keys_action[key] = static_cast<KeyAction>(action);
}

void mousebutton_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) mousebuttons_action[button] = static_cast<KeyAction>(action);
}

Input::Input()
{
    if (!Engine.IsHeadless())
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Engine.Device().GetWindow());

        // glfwSetJoystickCallback(joystick_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetMouseButtonCallback(window, mousebutton_callback);
        glfwSetScrollCallback(window, scroll_callback);

        Update();
    }
}

Input::~Input()
{
    GLFWwindow* window = static_cast<GLFWwindow*>(Engine.Device().GetWindow());

    // glfwSetJoystickCallback(NULL);
    glfwSetCursorPosCallback(window, NULL);
}

void Input::Update()
{
    // update keyboard key states
    for (int i = 0; i < nr_keys; ++i)
    {
        prev_keys_down[i] = keys_down[i];

        if (keys_action[i] == KeyAction::Press)
        {
            keys_down[i] = true;
            lastPressedKey = static_cast<KeyboardKey>(i);
        }
        else if (keys_action[i] == KeyAction::Release)
            keys_down[i] = false;

        keys_action[i] = KeyAction::None;
    }

    // update mouse button states
    for (int i = 0; i < nr_mousebuttons; ++i)
    {
        prev_mousebuttons_down[i] = mousebuttons_down[i];

        if (mousebuttons_action[i] == KeyAction::Press)
            mousebuttons_down[i] = true;
        else if (mousebuttons_action[i] == KeyAction::Release)
            mousebuttons_down[i] = false;

        mousebuttons_action[i] = KeyAction::None;
    }

    // update gamepad states
    for (int i = 0; i < max_nr_gamepads; ++i)
    {
        prev_gamepad_state[i] = gamepad_state[i];

        if (glfwJoystickPresent(i) && glfwJoystickIsGamepad(i))
            gamepad_connected[i] = static_cast<bool>(glfwGetGamepadState(i, &gamepad_state[i]));
    }
}

bool Input::IsGamepadAvailable(int gamepadID) const { return gamepad_connected[gamepadID]; }

float Input::GetGamepadAxis(int gamepadID, GamepadAxis axis) const
{
    if (!IsGamepadAvailable(gamepadID)) return 0.0;

    int a = static_cast<int>(axis);
    assert(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);
    return gamepad_state[gamepadID].axes[a];
}

float Input::GetGamepadAxisPrevious(int gamepadID, GamepadAxis axis) const
{
    if (!IsGamepadAvailable(gamepadID)) return 0.0;

    int a = static_cast<int>(axis);
    assert(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);
    return prev_gamepad_state[gamepadID].axes[a];
}

bool Input::GetGamepadButton(int gamepadID, GamepadButton button) const
{
    if (!IsGamepadAvailable(gamepadID)) return false;

    int b = static_cast<int>(button);
    assert(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::GetGamepadButtonOnce(int gamepadID, GamepadButton button) const
{
    if (!IsGamepadAvailable(gamepadID)) return false;

    int b = static_cast<int>(button);

    assert(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return !static_cast<bool>(prev_gamepad_state[gamepadID].buttons[b]) &&
           static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool Input::IsMouseAvailable() const { return true; }

bool Input::GetMouseButton(MouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b];
}

bool Input::GetMouseButtonOnce(MouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b] && !prev_mousebuttons_down[b];
}

glm::vec2 Input::GetMousePosition() const { return mousepos; }

float Input::GetMouseWheel() const { return mousewheel; }

float Input::GetMouseWheelCamera() const { return mouseWheelCamera; }

float Input::GetMouseWheelMax() const { return mouseWheelCameraMax; }

float Input::GetMouseWheelMin() const { return mouseWheelCameraMin; }

bool Input::IsKeyboardAvailable() const { return true; }

bool Input::GetKeyboardKey(KeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k];
}

bool Input::GetKeyboardKeyOnce(KeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k] && !prev_keys_down[k];
}

void Input::SetPaused(bool pause)
{ paused = pause; }
