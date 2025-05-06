#include "core/engine.hpp"
#include "mission_editor.hpp"
#include "../../Game/include/game_ui/main_menu.hpp"
#include "platform/pc/core/device_pc.hpp"
//#include "steam/steam_api.h"

using namespace bee;

int main()
{
    Engine.Initialize();
    Engine.SetGame<MissionEditor>();
    Engine.Device().SetFullScreen(true);
    Engine.Run();
    Engine.Shutdown();
}
