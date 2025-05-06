#include "leaderboard/leaderboard.hpp"

#include <algorithm>
#include <cereal/archives/json.hpp>
#include <cereal/types/utility.hpp>
#include "core/engine.hpp"
#include "core/fileio.hpp"
#include <fstream>
#include "user_interface/user_interface_serializer.hpp"

#include "tools/tools.hpp"

void Leaderboard::SaveScores()
{
    std::ofstream os(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, m_path + ".json"));
    cereal::JSONOutputArchive archive(os);
    archive(CEREAL_NVP(m_scores));
}

void Leaderboard::LoadScores()
{
    if (bee::fileExists(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, m_path + ".json")))
    {
        std::ifstream is(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, m_path + ".json"));
        cereal::JSONInputArchive archive(is);

        archive(CEREAL_NVP(m_scores));
    }
}

void Leaderboard::RemoveScore(int index) { m_scores.erase(m_scores.begin() + index); }

int Leaderboard::AddScore(std::pair<std::string, int> player)
{
    std::string userName = player.first;
    player.first += std::to_string(m_scores.size());
    m_scores.push_back(player);
    Sort();
    auto it = std::find(m_scores.begin(), m_scores.end(), player);
    m_scores[it - m_scores.begin()].first = userName;
    return it - m_scores.begin();
}

void Leaderboard::Sort()
{
    std::sort(m_scores.begin(), m_scores.end(),
              [](const std::pair<std::string, int>& lhs, const std::pair<std::string, int>& rhs) { return lhs.second > rhs.second; });
}

void Leaderboard::ShowLeaderboard(int index)
{
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    m_element = UI.serialiser->LoadElement("Leaderboard");
    m_background=UI.serialiser->LoadElement("LeaderboardBackground");
    int indexElement=1;
    for (;indexElement < m_scores.size() && indexElement<6; indexElement++)
    {
        UI.ReplaceString(UI.GetComponentID(m_element, std::to_string(indexElement) + "Place"),
                         std::to_string(indexElement) + "   " + m_scores[indexElement-1].first);
        UI.ReplaceString(UI.GetComponentID(m_element, std::to_string(indexElement) + "PlaceScore"),
                         ": " + std::to_string(m_scores[indexElement-1].second));
    }
    for (;indexElement<6;indexElement++)
    {
        UI.ReplaceString(UI.GetComponentID(m_element, std::to_string(indexElement) + "Place"),"");
        UI.ReplaceString(UI.GetComponentID(m_element, std::to_string(indexElement) + "PlaceScore"),"");
    }

   /* UI.ReplaceString(UI.serialiser->GetComponentID(m_element, "You"), std::to_string(index+1) + "   " +
                                                                               m_scores[index].first );
    UI.ReplaceString(UI.serialiser->GetComponentID(m_element, "YourScore"), ": " + std::to_string(m_scores[index].second));*/
}

void Leaderboard::UpdateUserName(int index, const std::string& newName)
{
    if (index == -1) return;
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    m_scores[index].first = newName;
    /*UI.ReplaceString(UI.serialiser->GetComponentID(m_element, "You"), std::to_string(index+1) + "   " + m_scores[index].first );
    UI.ReplaceString(UI.serialiser->GetComponentID(m_element, "YourScore"), ": " + std::to_string(m_scores[index].second));*/

    if (index < 5)
    {
        UI.ReplaceString(UI.GetComponentID(m_element, std::to_string(index+1) + "Place"),
                         std::to_string(index+1) + "   " + m_scores[index].first);
        UI.ReplaceString(UI.GetComponentID(m_element, std::to_string(index+1) + "PlaceScore"),
                         ": " + std::to_string(m_scores[index].second));
    }
}

const std::vector<std::pair<std::string, int>> Leaderboard::GetScores() { return m_scores; }
