#include "ai/Blackboards/blackboard.hpp"

std::vector<std::pair<std::string, std::string>> bee::ai::Blackboard::PreviewToString()
{
    std::vector<std::pair<std::string, std::string>> toReturn = {};

    for (auto& pair :  m_Map)
    {
        toReturn.push_back({pair.first, pair.second->ToString()});
    }

    return toReturn;
}
