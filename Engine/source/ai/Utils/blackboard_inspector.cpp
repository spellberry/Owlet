
#include "ai/Utils/blackboard_inspector.hpp"
#include <imgui/imgui.h>
#include "ai/Blackboards/blackboard.hpp"

void bee::ai::BlackboardInspector::Inspect(ai::Blackboard& blackboardToInspect)
{
    ImGui::Begin("Blackboard", &open);
    for (auto& preview : blackboardToInspect.PreviewToString())
    {
        if (preview.second.find('\n') != std ::string::npos)
        {
            if(ImGui::CollapsingHeader(preview.first.c_str()))
            {
                ImGui::Indent(20);
                ImGui::Text(preview.second.c_str());
                ImGui::Indent(-20);
            }
        }
        else
        {
            ImGui::Text(std::string(preview.first + ": ").c_str());
            ImGui::SameLine();
            ImGui::Text(preview.second.c_str());
        }
    }
    ImGui::End();
}
