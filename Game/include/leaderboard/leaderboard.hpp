#pragma once
#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include <string>
#include <vector>
#include <entt/entity/entity.hpp>

#include "user_interface/user_interface_structs.hpp"

struct Leaderboard
{
    void SaveScores();
    void LoadScores();
    void RemoveScore(int index);
    int AddScore(std::pair<std::string, int> player);
    void Sort();
    void ShowLeaderboard(int index);
    void UpdateUserName(int index, const std::string& newName);
    const std::vector<std::pair<std::string, int>> GetScores();

private:
    std::string m_path = "Leaderboard";
    bee::ui::UIElementID m_element = entt::null;
    bee::ui::UIElementID m_background = entt::null;
    std::vector<std::pair<std::string, int>> m_scores;
    template <class Archive>
    void serialize(Archive& archive)
    {
        CEREAL_NVP(m_scores);
    }
};