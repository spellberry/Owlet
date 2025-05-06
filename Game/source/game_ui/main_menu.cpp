#include "game_ui/main_menu.hpp"

#include <glm/glm.hpp>
#include <platform/dx12/animation_system.hpp>

#include "Starcraft.hpp"
#include "TowerDefense.hpp"
#include "camera/camera_rts_system.hpp"
#include "core/audio.hpp"
#include "core/device.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "level_editor/terrain_system.hpp"
#include "platform/dx12/RenderPipeline.hpp"
#include "rendering/render_components.hpp"
#include "tools/steam_input_system.hpp"
#include "user_interface/user_interface.hpp"
#include "user_interface/user_interface_serializer.hpp"

#ifdef STEAM_API_WINDOWS
UI_FUNCTION(ToStarCraft, StartStarCraft, auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
            bee::Engine.SteamInputSystem().ActivateActionSetLayer("InGameControls");
            UI.SetUiInputActions("Ui_Movement", "Press"); bee::Engine.SetGame<Starcraft>(););
#else
UI_FUNCTION(ToStarCraft, StartStarCraft, bee::Engine.SetGame<Starcraft>(););
#endif

#ifdef STEAM_API_WINDOWS auto& UI = bee::Engine.ECS().GetSystem < bee::ui::UserInterface>();
UI_FUNCTION(ToTower, StartTowerDefense, auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
            bee::Engine.SteamInputSystem().ActivateActionSetLayer("InGameControls");
            UI.SetUiInputActions("Ui_Movement", "Press"); bee::Engine.SetGame<TowerDefense>(););
#else
UI_FUNCTION(ToTower, StartTowerDefense, bee::Engine.SetGame<TowerDefense>(););
#endif

UI_FUNCTION(ToOp, ToOptions, dynamic_cast<MainMenu&>(bee::Engine.Game()).ToMenu(options););
UI_FUNCTION(ToMain, ToMainMenu, dynamic_cast<MainMenu&>(bee::Engine.Game()).ToMenu(mainMenu););
UI_FUNCTION(ToLev, ToLevels, dynamic_cast<MainMenu&>(bee::Engine.Game()).ToMenu(levels););
UI_FUNCTION(Ex, Exit, bee::Engine.Device().SetShouldClose(); bee::Engine.Audio().PlaySoundW("audio/click.wav", 1.5f, false););

void MainMenu::ToMenu(const state stat) const
{
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();

    switch (stat)
    {
        case options:
        {
            UI.SetDrawStateUIelement(m_options, true);
            UI.SetDrawStateUIelement(m_element, false);
            UI.SetDrawStateUIelement(m_levels, false);
            UI.SetSelectedInteractable(m_optionsReturnButt);
            break;
        }
        case mainMenu:
        {
            UI.SetDrawStateUIelement(m_options, false);
            UI.SetDrawStateUIelement(m_levels, false);
            UI.SetDrawStateUIelement(m_element, true);
            UI.SetSelectedInteractable(m_startSC);
            break;
        }
        case levels:
        {
            UI.SetDrawStateUIelement(m_options, false);
            UI.SetDrawStateUIelement(m_element, false);
            UI.SetDrawStateUIelement(m_levels, true);
            UI.SetSelectedInteractable(m_levelsReturnButt);
            break;
        }
    }
}
void MainMenu::Init()
{
#ifdef STEAM_API_WINDOWS
    bee::Engine.SteamInputSystem().ActivateActionSetLayer("MainMenuControls");
#endif
    bee::Engine.Device().SetWindowName("Owlet");
    bee::Engine.Audio().LoadSound("audio/music.mp3", true);
    bee::Engine.Audio().PlaySoundW("audio/music.mp3", 0.2f, false);

    gameName = "mainMenu";
    GameBase::Init();
    SetUpMenu();
    // const float height = static_cast<float>(bee::Engine.Device().GetHeight());
    // const float width = static_cast<float>(bee::Engine.Device().GetWidth());
    // const float norWidth = width / height;

    ///*bee::Engine.ECS().CreateSystem<bee::CameraSystemRTS>();
    // bee::Engine.ECS().CreateSystem<bee::RenderPipeline>();
    // bee::Engine.ECS().CreateSystem<bee::AnimationSystem>();*/
    // auto& UI =
    //     /*bee::Engine.ECS().CreateSystem<bee::ui::UserInterface>();*/ bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    // bee::Engine.WasPreInitialized() = true;
    // UI.LoadFont(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "fonts/DroidSans.sff"));
    // std::string str0 =
    //     bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "textures/UI/Game UI collection/png files/Bars-01.png");
    // m_hover.Img = m_buttonBack.Img = UI.LoadTexture(str0);

    // bee::ui::UIComponentID m_start;
    // m_hoverid = UI.AddOverlayImage(m_hover);
    ////
    //// main menu
    ////
    //{
    //    m_element = UI.CreateUIElement(0, bee::ui::top, bee::ui::left);
    //    glm::vec2 middle = glm::vec2((norWidth / 2) - ((m_buttonBack.GetNormSize(m_elementSize).x) / 2),
    //                                 0.5f - (m_buttonBack.GetNormSize(m_elementSize).y) / 2);
    //    bee::ui::UIComponentID levelButt;
    //    bee::ui::UIComponentID optionsButt;
    //    bee::ui::UIComponentID exitButt;
    //    int mainMenuImage = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "textures/Back.png"));
    //    UI.CreateTextureComponentFromAtlas(m_element, mainMenuImage, glm::vec4(0.0f, 0.0f, 1792, 1024), glm::vec2(0.f, 0.f),
    //                                       glm::vec2(norWidth, 1.0f));
    //    {
    //        std::string gameName = "RTS GAME";
    //        UI.CreateString(m_element, gameName, glm::vec2(middle.x - 0.15f, middle.y - 0.1f), 0.8f);
    //    }
    //    {
    //        m_start = UI.CreateTextureComponentFromAtlas(m_element, m_buttonBack.Img, m_buttonBack.GetAtlas(), middle,
    //                                                     m_buttonBack.GetNormSize(m_elementSize));
    //        m_startSC =
    //            UI.CreateButton(m_element, middle, m_buttonBack.GetNormSize(m_elementSize),
    //                            bee::ui::interaction(bee::ui::SwitchType::none, "StartStarCraft"),
    //                            bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single, m_hoverid);

    //        std::string str1 = "Start RTS";
    //        UI.CreateString(m_element, str1,
    //                        glm::vec2(middle.x + ((m_buttonBack.GetNormSize(m_elementSize).x) * 0.2),
    //                                  middle.y + ((m_buttonBack.GetNormSize(m_elementSize).y) * 0.65)),
    //                        0.2f, 0);
    //    }
    //    {
    //        middle = glm::vec2(middle.x, middle.y + 0.1f);
    //        m_start = UI.CreateTextureComponentFromAtlas(m_element, m_buttonBack.Img, m_buttonBack.GetAtlas(), middle,
    //                                                     m_buttonBack.GetNormSize(m_elementSize));
    //        m_startTD =
    //            UI.CreateButton(m_element, middle, m_buttonBack.GetNormSize(m_elementSize),
    //                            bee::ui::interaction(bee::ui::SwitchType::none, "StartTowerDefense"),
    //                            bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single, m_hoverid);

    //        std::string str1 = "Tower Defense";
    //        UI.CreateString(m_element, str1,
    //                        glm::vec2(middle.x + ((m_buttonBack.GetNormSize(m_elementSize).x) * 0.2),
    //                                  middle.y + ((m_buttonBack.GetNormSize(m_elementSize).y) * 0.65)),
    //                        0.2f, 0);
    //    }
    //    {
    //        middle = glm::vec2(middle.x, middle.y + 0.1f);
    //        m_start = UI.CreateTextureComponentFromAtlas(m_element, m_buttonBack.Img, m_buttonBack.GetAtlas(), middle,
    //                                                     m_buttonBack.GetNormSize(m_elementSize));
    //        levelButt =
    //            UI.CreateButton(m_element, middle, m_buttonBack.GetNormSize(m_elementSize),
    //                            bee::ui::interaction(bee::ui::SwitchType::none, "ToLevels"),
    //                            bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single, m_hoverid);

    //        std::string str1 = "Levels";
    //        UI.CreateString(m_element, str1,
    //                        glm::vec2(middle.x + ((m_buttonBack.GetNormSize(m_elementSize).x) * 0.2),
    //                                  middle.y + ((m_buttonBack.GetNormSize(m_elementSize).y) * 0.65)),
    //                        0.2f, 0);
    //    }
    //    {
    //        middle = glm::vec2(middle.x, middle.y + 0.1f);
    //        m_start = UI.CreateTextureComponentFromAtlas(m_element, m_buttonBack.Img, m_buttonBack.GetAtlas(), middle,
    //                                                     m_buttonBack.GetNormSize(m_elementSize));
    //        optionsButt =
    //            UI.CreateButton(m_element, middle, m_buttonBack.GetNormSize(m_elementSize),
    //                            bee::ui::interaction(bee::ui::SwitchType::none, "ToOptions"),
    //                            bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single, m_hoverid);

    //        std::string str1 = "Options";
    //        UI.CreateString(m_element, str1,
    //                        glm::vec2(middle.x + ((m_buttonBack.GetNormSize(m_elementSize).x) * 0.2),
    //                                  middle.y + ((m_buttonBack.GetNormSize(m_elementSize).y) * 0.65)),
    //                        0.2f, 0);
    //    }
    //    {
    //        middle = glm::vec2(middle.x, middle.y + 0.1f);
    //        m_start = UI.CreateTextureComponentFromAtlas(m_element, m_buttonBack.Img, m_buttonBack.GetAtlas(), middle,
    //                                                     m_buttonBack.GetNormSize(m_elementSize));
    //        exitButt = UI.CreateButton(m_element, middle, m_buttonBack.GetNormSize(m_elementSize),
    //                                   bee::ui::interaction(bee::ui::SwitchType::none, "Exit"),
    //                                   bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single,
    //                                   m_hoverid);

    //        std::string str1 = "Exit";
    //        UI.CreateString(m_element, str1,
    //                        glm::vec2(middle.x + ((m_buttonBack.GetNormSize(m_elementSize).x) * 0.2),
    //                                  middle.y + ((m_buttonBack.GetNormSize(m_elementSize).y) * 0.65)),
    //                        0.2f, 0);
    //    }
    //    UI.SetComponentNeighbourInDirection(m_startSC, m_startTD, bee::ui::bottom);

    //    UI.SetComponentNeighbourInDirection(m_startTD, m_startSC, bee::ui::top);
    //    UI.SetComponentNeighbourInDirection(m_startTD, levelButt, bee::ui::bottom);

    //    UI.SetComponentNeighbourInDirection(levelButt, m_startTD, bee::ui::top);
    //    UI.SetComponentNeighbourInDirection(levelButt, optionsButt, bee::ui::bottom);

    //    UI.SetComponentNeighbourInDirection(optionsButt, levelButt, bee::ui::top);
    //    UI.SetComponentNeighbourInDirection(optionsButt, exitButt, bee::ui::bottom);

    //    UI.SetComponentNeighbourInDirection(exitButt, optionsButt, bee::ui::top);
    //}
    ////
    //// options
    ////
    //{
    //    m_options = UI.CreateUIElement(0, bee::ui::Alignment::top, bee::ui::Alignment::left);
    //    glm::vec2 middle = glm::vec2((norWidth / 2) - ((m_buttonBack.GetNormSize(m_elementSize).x) / 2),
    //                                 0.5f - (m_buttonBack.GetNormSize(m_elementSize).y) / 2);
    //    UI.SetDrawStateUIelement(m_options, false);
    //    {
    //        {
    //            middle = glm::vec2(0.1f, 0.1f);
    //            m_start = UI.CreateTextureComponentFromAtlas(m_options, m_buttonBack.Img, m_buttonBack.GetAtlas(), middle,
    //                                                         m_buttonBack.GetNormSize(m_elementSize));
    //            m_optionsReturnButt =
    //                UI.CreateButton(m_options, middle, m_buttonBack.GetNormSize(m_elementSize),
    //                                bee::ui::interaction(bee::ui::SwitchType::none, "ToMainMenu"),
    //                                bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single, m_hoverid);
    //            std::string str1 = "Return";
    //            UI.CreateString(m_options, str1,
    //                            glm::vec2(middle.x + ((m_buttonBack.GetNormSize(m_elementSize).x) * 0.2),
    //                                      middle.y + ((m_buttonBack.GetNormSize(m_elementSize).y) * 0.65)),
    //                            0.2f, 0);
    //        }
    //    }
    //}

    ////
    //// levels
    ////
    //{
    //    m_levels = UI.CreateUIElement(0, bee::ui::Alignment::top, bee::ui::Alignment::left);
    //    UI.SetDrawStateUIelement(m_levels, false);
    //    {
    //        bee::ui::UIComponentID lvl1Butt;
    //        {
    //            glm::vec2 middle = glm::vec2((norWidth / 2) - ((m_buttonBack.GetNormSize(m_elementSize).x) / 2),
    //                                         0.5f - (m_buttonBack.GetNormSize(m_elementSize).y) / 2);
    //            middle = glm::vec2(0.1f, 0.1f);
    //            m_start = UI.CreateTextureComponentFromAtlas(m_levels, m_buttonBack.Img, m_buttonBack.GetAtlas(), middle,
    //                                                         m_buttonBack.GetNormSize(m_elementSize));
    //            m_levelsReturnButt =
    //                UI.CreateButton(m_levels, middle, m_buttonBack.GetNormSize(m_elementSize),
    //                                bee::ui::interaction(bee::ui::SwitchType::none, "ToMainMenu"),
    //                                bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single, m_hoverid);
    //            std::string str1 = "Return";
    //            UI.CreateString(m_levels, str1,
    //                            glm::vec2(middle.x + ((m_buttonBack.GetNormSize(m_elementSize).x) * 0.2),
    //                                      middle.y + ((m_buttonBack.GetNormSize(m_elementSize).y) * 0.65)),
    //                            0.2f, 0);
    //        }
    //        {
    //            glm::vec2 middle = glm::vec2((norWidth / 2) - ((m_buttonBack.GetNormSize(m_elementSize).x) / 2),
    //                                         0.5f - (m_buttonBack.GetNormSize(m_elementSize).y) / 2);
    //            m_start = UI.CreateTextureComponentFromAtlas(m_levels, m_buttonBack.Img, m_buttonBack.GetAtlas(), middle,
    //                                                         m_buttonBack.GetNormSize(m_elementSize));
    //            lvl1Butt =
    //                UI.CreateButton(m_levels, middle, m_buttonBack.GetNormSize(m_elementSize),
    //                                bee::ui::interaction(bee::ui::SwitchType::none, "StartStarCraft"),
    //                                bee::ui::interaction(bee::ui::SwitchType::none), bee::ui::ButtonType::single, m_hoverid);
    //            std::string str1 = "level 1";
    //            UI.CreateString(m_levels, str1,
    //                            glm::vec2(middle.x + ((m_buttonBack.GetNormSize(m_elementSize).x) * 0.2),
    //                                      middle.y + ((m_buttonBack.GetNormSize(m_elementSize).y) * 0.65)),
    //                            0.2f, 0);
    //        }
    //        UI.SetComponentNeighbourInDirection(lvl1Butt, m_levelsReturnButt, bee::ui::top);
    //        UI.SetComponentNeighbourInDirection(m_levelsReturnButt, lvl1Butt, bee::ui::bottom);
    //    }
    //}

    //// create scene
    // bee::Engine.ECS().CreateSystem<lvle::TerrainSystem>();

#ifdef STEAM_API_WINDOWS
    UI.SetUiInputActions("Menu", "Click");
    UI.SetInputMode(bee::ui::InputMode::controller);
#endif
}
void MainMenu::ShutDown()
{
    // Clear produces bugs in the game
    /*auto& UI = */ bee::Engine.ECS().GetSystem<bee::ui::UserInterface>().Clean();
    /*UI.DeleteUIElement(m_element);
    UI.DeleteUIElement(m_options);
    UI.DeleteUIElement(m_levels);*/

    // also wipe scene
}
void MainMenu::Update(float dt) {}

void MainMenu::SetUpMenu()
{
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();

    /*const float height = static_cast<float>(bee::Engine.Device().GetHeight());
    const float width = static_cast<float>(bee::Engine.Device().GetWidth());
    const float norWidth = width / height;
    int mainMenuImage = UI.LoadTexture(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, "textures/Back.png"));
    UI.CreateTextureComponentFromAtlas(m_menuPicture, mainMenuImage, glm::vec4(0.0f, 0.0f, 1792, 1024), glm::vec2(0.f, 0.f),
    glm::vec2(norWidth, 1.0f));*/

    m_menuButtons = UI.serialiser->LoadElement("New_Menu_Buttons");
    m_menuBackground = UI.serialiser->LoadElement("New_Menu_Background");
    m_menuPicture = UI.serialiser->LoadElement("New_Menu_Picture");
}