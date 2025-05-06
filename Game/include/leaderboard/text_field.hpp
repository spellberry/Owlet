
#include <string>

struct TextField
{
public:
    static void StartKeyboardHook();

    static void StopKeyboardHook();

    static void SetUpdateString(bool updateStringState);
    static bool GetUpdateString();
    static std::string GetTextInput();
    static void ResetTextInput();
    static void SetRunningState(bool newRunningState);
    static bool GetRunningState();
    char dummy = 'd';
};