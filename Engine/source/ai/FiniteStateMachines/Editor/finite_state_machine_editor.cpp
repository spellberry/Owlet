#if defined(BEE_PLATFORM_PC)
#include "ai/FiniteStateMachines/Editor/finite_state_machine_editor.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_node_editor.h>

#include <algorithm>
#include <filesystem>
#include <glm/common.hpp>


#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "ai/Utils/editor_utils.hpp"
#include "ai/Utils/generic_factory.hpp"
#include "core/engine.hpp"
#include "core/fileio.hpp"

bee::ai::FiniteStateMachineEditor::FiniteStateMachineEditor()
{
    ax::NodeEditor::Config config;
    config.SettingsFile = "SimpleFSM.json";
    m_context = ax::NodeEditor::CreateEditor(&config);
    m_previewContext = ax::NodeEditor::CreateEditor(&config);
}

void bee::ai::FiniteStateMachineEditor::SelectedStateNameInputBox(const std::unique_ptr<ai::NodeWrapper>& focusedNode)
{
    char* buffer = const_cast<char*>(focusedNode->name.c_str());
    ImGui::Text("Name");
    ImGui::SameLine();
    if (ImGui::InputText("##Name", buffer, 20))
    {
        focusedNode->name = buffer;
    }
}

void bee::ai::FiniteStateMachineEditor::SelectedStateTypeSelection(const std::unique_ptr<ai::NodeWrapper>& focusedNode) const
{
    ImGui::Text("State");
    ImGui::SameLine();

    std::vector<std::string> types = GenericFactory<ai::State>::Instance().GetKeys();

    if (types.empty()) return;

    std::string choice = focusedNode->stateType.empty() ? types[0] : focusedNode->stateType;

    if (ImGui::BeginCombo("##stateSelection", choice.c_str()))
    {
        for (auto value : types)
        {
            const bool is_selected = (choice == value);
            if (ImGui::Selectable(value.c_str(), is_selected)) choice = value;
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    choice.erase(std::remove_if(choice.begin(), choice.end(), [](unsigned char c) { return std::isspace(c); }), choice.end());


    if (choice != focusedNode->stateType)
    {
        focusedNode->underlyingState = std::move(GenericFactory<State>::Instance().CreateProduct(choice));
    }

    focusedNode->stateType = choice;
} 

void bee::ai::FiniteStateMachineEditor::SelectedNodeTransitionButtons(const std::unique_ptr<ai::NodeWrapper>& focusedNode, int index,int stateToGoToChoice, ai::NodeTransition& transition) const
{
    if (ImGui::BeginCombo(std::string(std::string("##StateSelection") + std::to_string(index)).c_str(),transition.nodeTo == -1? "Select state": std::string(m_editorFSM.nodes[transition.nodeTo - 1]->name + "(" +std::to_string(m_editorFSM.nodes[transition.nodeTo - 1]->id) + ")").c_str()))
    {
        for (auto& value : m_editorFSM.nodes)
        {
            if (value->id == focusedNode->id) continue;
            const bool is_selected = (stateToGoToChoice == static_cast<int>(value->id));
            if (ImGui::Selectable(std::string(value->name + " (" + std::to_string(value->id) + ")").c_str(), is_selected))
            {
                transition.nodeTo = static_cast<int>(value->id);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void bee::ai::FiniteStateMachineEditor::SelectedNodeTransitionComparatorButtons(int index, std::vector<int>& transitionsToRemove,ai::NodeTransition& transition) const
{
    ImGui::SameLine();

    if (ImGui ::Button(std::string("+ ##" + std::to_string(index)).c_str(), ImVec2(30, 30)))
    {
        transition.comparators.emplace_back();
    }

    ImGui::SameLine();
    if (ImGui ::Button(std::string("- ##" + std::to_string(index)).c_str(), ImVec2(30, 30)))
    {
        transitionsToRemove.push_back(index);

        ax::NodeEditor::SetCurrentEditor(m_context);

        size_t temp = 0;
        for (auto type : m_editorFSM.transitions)
        {
            for (auto p : type.second)
            {
                ax::NodeEditor::DeleteLink(temp);
                temp++;
            }
        }

        ax::NodeEditor::SetCurrentEditor(nullptr);
    }
}

void bee::ai::FiniteStateMachineEditor::ComparatorValueInput(ComparatorWrapper& comparator, const char* label) const
{
    char* buffer = comparator.value.data();
    std::string toString;
    if (comparator.type == "int")
    {
        int val = std::string(buffer).empty() ? 0 : std::stoi(buffer);
        if (ImGui::InputInt(label, &val))
        {
            toString = std::to_string(val);
            buffer = toString.data();
        }
    }

    if (comparator.type == "float" || comparator.type == "double")
    {
        float val = std::string(buffer).empty() ? 0.0f : std::stof(buffer);
        if (ImGui::InputFloat(label, &val))
        {
            toString = std::to_string(val);
            buffer = toString.data();
        }
    }

    if (comparator.type == "bool")
    {
        int intVal = std::string(buffer).empty() ? 0 : std::stoi(buffer);
        bool val = intVal == 1;
        ImGui::Checkbox(label, &val);
        intVal = val ? 1 : 0;
        toString = std::to_string(intVal);
        buffer = toString.data();
    }

    if (comparator.type == "string")
    {
        ImGui::InputText(label, buffer, 31);
    }

    comparator.value = buffer;
}

void bee::ai::FiniteStateMachineEditor::ChooseComparatorTypeInput(ai::ComparatorWrapper& comparator, const char* label) const
{
    const std::string typeToChoose = comparator.type;
    if (ImGui::BeginCombo(label,
                          comparator.type.empty() ? ComparatorsInEditor::comparatorTypes[0].c_str() : comparator.type.c_str()))
    {
        for (auto& value : ComparatorsInEditor::comparatorTypes)
        {
            const bool is_selected = (typeToChoose == value);
            if (ImGui::Selectable(std::string(value).c_str(), is_selected))
            {
                comparator.type = value;
                comparator.value = std::string();
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void bee::ai::FiniteStateMachineEditor::ChooseComparisonTypeInput(ai::ComparatorWrapper& comparator, const char* label) const
{
    if (ImGui::BeginCombo(label, comparator.comparisonType.empty() ? ComparatorsInEditor::comparisonTypes[0].c_str()
                                                                   : comparator.comparisonType.c_str()))
    {
        for (auto& value : ComparatorsInEditor::comparisonTypes)
        {
            std::string typeToChoose;
            const bool is_selected = (typeToChoose == value);
            if (ImGui::Selectable(std::string(value).c_str(), is_selected))
            {
                comparator.comparisonType = value;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void bee::ai::FiniteStateMachineEditor::DrawSelectedStateDetails()
{
    if (m_editorFSM.nodes.empty()) return;
    ImGui::BeginChild("FSM Details", ImVec2(400, ImGui::GetWindowSize().y * 0.90f));

    if (m_selectedNodes.size() == 1)
    {
        auto& focusedNode = m_editorFSM.nodes[m_selectedNodes[0]];
        SelectedStateNameInputBox(focusedNode);

        bool isDefault = m_defaultState == m_selectedNodes[0];
        ImGui::Checkbox("IsDefault", &isDefault);
        if (isDefault)
        {
            m_defaultState = m_selectedNodes[0];
        }

        SelectedStateTypeSelection(focusedNode);

        if (ImGui::Button("Add transition", ImVec2(100, 40)))
        {
            NodeTransition transition{};
            m_editorFSM.transitions[focusedNode->id].push_back(transition);
        }

        int index = 0;
        int stateToGoToChoice = -1;

        std::vector<int> comparatorsToRemove = {};
        std::vector<int> transitionsToRemove = {};

        for (NodeTransition& transition : m_editorFSM.transitions[focusedNode->id])
        {
            SelectedNodeTransitionButtons(focusedNode, index, stateToGoToChoice, transition);
            SelectedNodeTransitionComparatorButtons(index, transitionsToRemove, transition);

            int comparatorIndex = 0;

            ImGui::Text("Comparators:");
            for (auto& comparator : transition.comparators)
            {
                ImGui::Text("Type");
                ImGui::SameLine();
                ChooseComparatorTypeInput(comparator, std::string(std::string("##TypeSelection") + std::to_string(index) + " " +std::to_string(comparatorIndex)).c_str());

                char* buffer = const_cast<char*>(comparator.key.c_str());

                ImGui::Text("Key");
                ImGui::SameLine();
                ImGui::InputText(std::string(std::string("##Key") + std::to_string(index) + " " + std::to_string(comparatorIndex)).c_str(),buffer, 31);
                comparator.key = buffer;

                ImGui::Text("Comparison");
                ImGui::SameLine();
                ChooseComparisonTypeInput(comparator, std::string(std::string("##ComparisonSelection") + std::to_string(index) +std::to_string(comparatorIndex)).c_str());

                ImGui::Text("Value");
                ImGui::SameLine();
                ComparatorValueInput(comparator, std::string(std::string("##Value" + std::to_string(index) + " " +std::to_string(comparatorIndex))).c_str());

                if (ImGui::Button(std::string(std::string("Remove") + " ##" + std::to_string(index) + " " + std::to_string(comparatorIndex)).c_str()))
                {
                    comparatorsToRemove.push_back(comparatorIndex);
                }

                comparatorIndex++;
            }


            // Remove comparisons to remove
            int numRemoved = 0;
            for (const auto indexToRemove : comparatorsToRemove)
            {
                transition.comparators.erase(transition.comparators.begin() + indexToRemove - numRemoved);
                numRemoved++;
            }
            comparatorsToRemove.clear();

            index++;
        }

        // Remove transitions to remove
        int removed = 0;
        for (const int toRemove : transitionsToRemove)
        {
            m_editorFSM.transitions[focusedNode->id].erase(m_editorFSM.transitions[focusedNode->id].begin() + toRemove -
                                                             removed);
            removed++;
        }

        ImGui::Text("Editor variables:");

        if (focusedNode->underlyingState == nullptr)
        {
            ImGui::EndChild();
            return;
        }
        
        for (auto& editorVariable : focusedNode->underlyingState->editorVariables)
        {
            DrawSerializedField(editorVariable);
        }
    }

    ImGui::EndChild();
}

void bee::ai::FiniteStateMachineEditor::DrawTransitions(EditorFSMWrapper& wrapper) const
{
    int index = 0;
    for (const auto& element : wrapper.transitions)
    {
        for (auto& transition : element.second)
        {
            if (glm::sign(ax::NodeEditor::GetNodePosition(element.first).x -ax::NodeEditor::GetNodePosition(transition.nodeTo).x) > 0)
            {
                ax::NodeEditor::Link(index, m_maxNumStates + element.first * 2 - 1, m_maxNumStates + transition.nodeTo * 2);
            }
            else
            {
                ax::NodeEditor::Link(index, m_maxNumStates + element.first * 2, m_maxNumStates + transition.nodeTo * 2 - 1);
            }
            index++;
        }
    }
}

void bee::ai::FiniteStateMachineEditor::DeleteNode(size_t nodeIdToRemove)
{
    m_selectedNodes.clear();

    m_editorFSM.nodes.erase(std::remove_if(m_editorFSM.nodes.begin(), m_editorFSM.nodes.end(),[nodeIdToRemove](const std::unique_ptr<NodeWrapper>& node) { return node->id == nodeIdToRemove; }),m_editorFSM.nodes.end());

    m_editorFSM.transitions.erase(nodeIdToRemove);

    for (auto& element : m_editorFSM.transitions)
    {
        element.second.erase(std::remove_if(element.second.begin(), element.second.end(),[nodeIdToRemove](const NodeTransition& t) { return nodeIdToRemove == t.nodeTo; }),element.second.end());
    }

    m_removedIds.push_back(nodeIdToRemove);
}

void bee::ai::FiniteStateMachineEditor::ResetHoverInfo() { m_hovered = -1; }

void bee::ai::FiniteStateMachineEditor::DrawNode(const ai::EditorFSMWrapper& wrapper, size_t i)
{
    if (ax::NodeEditor::IsNodeSelected(wrapper.nodes[i]->id))
    {
        m_selectedNodes.push_back(i);
    }

    auto& nodeWrapper = wrapper.nodes[i];

    ax::NodeEditor::BeginNode(wrapper.nodes[i]->id);

    ax::NodeEditor::BeginPin(m_maxNumStates + wrapper.nodes[i]->id * 2, ax::NodeEditor::PinKind::Input);
    ImGui::Dummy(ImVec2(1, 1));
    ax::NodeEditor::EndPin();
    ImGui::SameLine();
    ImGui::Text(std::string(nodeWrapper->name + " (" + std::to_string(nodeWrapper->id) + ")").c_str());
    ImGui::SameLine();
    ax::NodeEditor::BeginPin(m_maxNumStates + wrapper.nodes[i]->id * 2 - 1, ax::NodeEditor::PinKind::Output);
    ImGui::Dummy(ImVec2(1, 1));
    ax::NodeEditor::EndPin();
    ax::NodeEditor::EndNode();

    if (m_reloadPositions)
    {
        for (int i = 0; i < wrapper.nodes.size(); i++)
        {
            ax::NodeEditor::SetNodePosition(i + 1, wrapper.nodes[i]->position);
        }
        ax::NodeEditor::NavigateToContent(false, 0);

        m_reloadPositions = false;
    }

   wrapper.nodes[i]->position = ax::NodeEditor::GetNodePosition(i+1);
}

void bee::ai::FiniteStateMachineEditor::DrawNodes(const EditorFSMWrapper& wrapper)
{
    ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinArrowSize, 50.0f);
    ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinArrowWidth, 10.0f);

    ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, ImVec4(0, 0, 0, 1.0f));

    for (size_t i = 0; i < wrapper.nodes.size(); i++)
    {
        DrawNode(wrapper, i);
    }
}

void bee::ai::FiniteStateMachineEditor::DrawNodes(const EditorFSMWrapper& wrapper, const StateMachineContext& context)
{
    ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinArrowSize, 50.0f);
    ax::NodeEditor::PushStyleVar(ax::NodeEditor::StyleVar_PinArrowWidth, 10.0f);

    for (size_t i = 0; i < wrapper.nodes.size(); i++)
    {
        ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, ImVec4(0, 0, 0, 1.0f));
        if (context.GetCurrentState().has_value())
        {
            if (context.GetCurrentState().value() == i)
            {
                ax::NodeEditor::PushStyleColor(ax::NodeEditor::StyleColor_NodeBg, ImVec4(0.5f, 0, 0.5f, 1));
            }
        }
        
        DrawNode(wrapper, i);
    }
}

std::vector<std::string> bee::ai::FiniteStateMachineEditor::GetStateNames() const
{
    std::vector<std::string> toReturn;

    for (auto& node : m_editorFSM.nodes)
    {
        toReturn.push_back(node->name);
    }

    return toReturn;
}

void bee::ai::FiniteStateMachineEditor::DrawUpperMenu()
{
    if (ImGui::Button("Save", ImVec2(50, 30)))
    {
        SaveFSM();
    }

    ImGui::SameLine(0);

    if (ImGui::Button("Load", ImVec2(50, 30)))
    {
        LoadFSM();
    }

    ImGui::SameLine(0);

    if (ImGui::Button("Clear", ImVec2(50, 30)))
    {
        m_editorFSM.nodes.clear();
        m_editorFSM.transitions.clear();
        m_removedIds.clear();
    }

    ImGui::SameLine(ImGui::GetWindowSize().x * 0.99f - 30);

    if (ImGui::Button("+", ImVec2(30, 30)))
    {
        CreateNode();
    }
}

void bee::ai::FiniteStateMachineEditor::SaveStates(nlohmann::json& states) const
{
    if (m_editorFSM.nodes.empty()) return;
    for (auto& element : m_editorFSM.nodes)
    {
        nlohmann::json stateObject = nlohmann::json::object();
        stateObject["name"] = element->stateType;
        stateObject["default"] = m_defaultState+1 == element->id;
        stateObject["editor-variables"] = nlohmann::json();

        if (element->underlyingState == nullptr) continue;

        for (auto& variable : element->underlyingState->editorVariables)
        {
            auto toAdd = nlohmann::json();
            toAdd["name"] = variable.first;
            toAdd["value"] = variable.second->ToString();
            stateObject["editor-variables"].push_back(toAdd);
        }
        states.push_back(stateObject);
    }
}

void bee::ai::FiniteStateMachineEditor::SaveTransitions(nlohmann::json& json) const
{
    nlohmann::json transitions = nlohmann::json::array();
    if (m_editorFSM.transitions.empty()) return;
    for (auto& pair : m_editorFSM.transitions)
    {
        nlohmann::json transitionData = nlohmann::json::object();

        // from
        transitionData["from"] = pair.first - 1;
        std::string fromIndex = std::to_string(pair.first);

        for (const auto& data : pair.second)
        {
            nlohmann::json transition = nlohmann::json::object();
            transition["to"] = data.nodeTo - 1;
            nlohmann::json comparators = nlohmann::json::array();

            for (const auto& comparator : data.comparators)
            {
                std::string comparatorString = comparator.ToString();
                if (comparatorString.empty()) continue;
                comparators.emplace_back(comparatorString);
            }

            transition["comparators"] = comparators;
            transitionData["transitions"].push_back(transition);
        }

        transitions.push_back(transitionData);
    }
    json["transition-data"] = transitions;
}

void bee::ai::FiniteStateMachineEditor::HandleClicks()
{
    const auto hoveredNode = ax::NodeEditor::GetHoveredNode();
    if (hoveredNode)
    {
        m_hovered = ax::NodeEditor::GetHoveredNode().Get();
    }

    if (ImGui::IsMouseClicked(1) && m_hovered != -1)
    {
        if (hoveredNode.Get() == 0)
        {
            m_clicked = false;
            m_hovered = -1;
        }
        else
        {
            m_mousePositionOnMenu = ImGui::GetIO().MousePos;
            m_clicked = true;
        }
    }
}

void bee::ai::FiniteStateMachineEditor::DrawPopupMenu()
{
    if (m_clicked && m_hovered != -1)
    {
        ImGui::OpenPopup("contextMenu");

        ImGui::SetCursorPos(m_mousePositionOnMenu);

        if (ImGui::BeginPopup("contextMenu"))
        {
            if (ImGui::Button("Delete"))
            {
                DeleteNode(m_hovered);
                ResetHoverInfo();
                m_clicked = false;
                m_justOpenedMenu = false;
            }

            ImGui::EndPopup();
        }
    }
}

void bee::ai::FiniteStateMachineEditor::SaveEditorData(nlohmann::json& json) const
{
    for (int i = 1; i < m_editorFSM.nodes.size() + 1; i++)
    {
        auto nodePosition = m_editorFSM.nodes[i - 1]->position;
        nlohmann::json node = {{"position", {{"x", nodePosition.x}, {"y", nodePosition.y}}},{"name", m_editorFSM.nodes[i - 1]->name}};
        json.push_back(node);
    }
}

void bee::ai::FiniteStateMachineEditor::Update()
{
    auto& io = ImGui::GetIO();
    bool open = true;

    ImGui::Begin("FSM editor", &open);
    DrawUpperMenu();

    ImGui::Separator();

    DrawSelectedStateDetails();

    m_selectedNodes.clear();

    ImGui::SameLine(400);

    ax::NodeEditor::SetCurrentEditor(m_context);
    ax::NodeEditor::Begin("FSM editor", ImVec2(0.0, 0.0f));

    HandleClicks();
    DrawNodes(m_editorFSM);
    DrawTransitions(m_editorFSM);
    ax::NodeEditor::End();
    ax::NodeEditor::SetCurrentEditor(nullptr);

    DrawPopupMenu();

    ImGui::End();
}

void bee::ai::FiniteStateMachineEditor::PreviewContext(const FiniteStateMachine& fsm, const StateMachineContext& context)
{
    auto& io = ImGui::GetIO();
    bool open = true;

    if (&fsm != m_previewedFSM)
    {
        ax::NodeEditor::DestroyEditor(m_previewContext);

        ax::NodeEditor::Config config;
        config.SettingsFile = "SimpleFSM.json";
        m_previewContext = ax::NodeEditor::CreateEditor(&config);

        m_previewFSM.nodes.clear();
        m_previewFSM.transitions.clear();

        m_previewedFSM = &fsm;
        nlohmann::json serialized = nlohmann::json::parse(fsm.Serialize().str());

        LoadStates(serialized["states"], m_previewFSM);
        LoadTransitions(serialized["transition-data"], m_previewFSM);
        m_reloadPositions = true;

        if (serialized.contains("editor-data"))
        {
            LoadEditorData(serialized["editor-data"], m_previewFSM);
        }
        else
        {
            const float xStart = 0.0f;
            const float yStart = 0.0f;

            for (size_t i = 0; i < m_previewFSM.nodes.size(); ++i)
            {
                const auto& node = m_previewFSM.nodes[i];

                const size_t row = i / 2;
                const size_t col = i % 2;

                node->position = {xStart + col* 200.0f, yStart + row *200.0f };
            } 
        }
    }

    ImGui::Begin("FSM preview", &open);
    ax::NodeEditor::SetCurrentEditor(m_previewContext);
    ax::NodeEditor::Begin("FSM preview", ImVec2(0.0, 0.0f));

    DrawNodes(m_previewFSM, context);
    DrawTransitions(m_previewFSM);

    ax::NodeEditor::NavigateToContent(0);
    ax::NodeEditor::End();
    ax::NodeEditor::SetCurrentEditor(nullptr);

    ImGui::End();
}

void bee::ai::FiniteStateMachineEditor::SaveFSM() const
{
    auto directory = Dialogs::OpenFileSaveDialog();
    nlohmann::json save;

    save["states"] = nlohmann::json::array();
    save["transitions"] = nlohmann::json::array();
    save["behavior-structure-type"] = "FSM";
    save["editor-data"] = nlohmann::json::array();

    SaveStates(save["states"]);
    SaveTransitions(save);
    SaveEditorData(save["editor-data"]);

    bee::Engine.FileIO().WriteTextFile(bee::FileIO::Directory::None, directory, save.dump());
}

size_t bee::ai::FiniteStateMachineEditor::GetIdToSet()
{
    if (m_removedIds.empty())
    {
        return m_editorFSM.nodes.size() + 1u;
    }
    else
    {
        std::sort(m_removedIds.begin(), m_removedIds.end());
        size_t toReturn = m_removedIds[0];
        m_removedIds.erase(m_removedIds.begin());
        return toReturn;
    }
}

void bee::ai::FiniteStateMachineEditor::CreateNode()
{
    if (m_editorFSM.nodes.size() + 1 >= m_maxNumStates) return;
    auto nodeToAdd = std::make_unique<NodeWrapper>();
    nodeToAdd->id = GetIdToSet();
    m_editorFSM.transitions[nodeToAdd->id] = {};
    if (m_editorFSM.nodes.empty())
    {
        m_reloadPositions = true;
    }

    std::vector<std::string> types = GenericFactory<ai::State>::Instance().GetKeys();
    if (!types.empty())
    {
        nodeToAdd->underlyingState = std::move(GenericFactory<State>::Instance().CreateProduct(types[0]));
    }

    m_editorFSM.nodes.push_back(std::move(nodeToAdd));
}

void bee::ai::FiniteStateMachineEditor::LoadStates(nlohmann::json& states, EditorFSMWrapper& wrapper)
{
    for (size_t i = 0; i < states.size(); i++)
    {
        if (states[i]["default"].get<bool>())
        {
            m_defaultState = i;
        }

        auto temp = std::make_unique<NodeWrapper>();
        temp->id = i + 1;
        temp->stateType = states[i]["name"].get<std::string>();

        temp->stateType.erase(std::remove_if(temp->stateType.begin(), temp->stateType.end(), [](unsigned char c) { return std::isspace(c); }),temp->stateType.end());

        auto state =GenericFactory<State>::Instance().CreateProduct(temp->stateType);
        if (state == nullptr) continue;
        temp->underlyingState = std::move(state);

        for (auto element : states[i]["editor-variables"])
        {
            if (!element.contains("name")) continue;
            auto name = element["name"].get<std::string>();
            if (temp->underlyingState->editorVariables.find(name) == temp->underlyingState->editorVariables.end()) continue;
            auto value = element["value"].get<std::string>();

            temp->editorVariables[name] = value;
            temp->underlyingState->editorVariables[name]->Deserialize(value);
        }

        wrapper.nodes.push_back(std::move(temp));
    }
}

void bee::ai::FiniteStateMachineEditor::DeserializeComparators(nlohmann::json& comparators, NodeTransition& node_transition)
{
    for (size_t i = 0; i < comparators.size(); i++)
    {
        std::stringstream stream = std::stringstream(comparators[i].get<std::string>());
        std::string comparisonKey;
        std::string typeName;
        int comparisonType;
        std::string value;

        stream >> comparisonKey;
        stream >> typeName;
        stream >> comparisonType;
        stream >> value;

        ComparatorWrapper temp{comparisonKey, typeName, ComparatorsInEditor::comparisonTypes[comparisonType], value};
        node_transition.comparators.push_back(temp);
    }
}

void bee::ai::FiniteStateMachineEditor::LoadTransitions(nlohmann::json& transitionData, EditorFSMWrapper& wrapper)
{
    for (int i = 0; i < transitionData.size(); i++)
    {
        size_t stateFrom = transitionData[i]["from"];
        for (int j = 0; j < transitionData[i]["transitions"].size(); j++)
        {
            size_t stateTo = transitionData[i]["transitions"][j]["to"];
            auto tempData = NodeTransition();
            tempData.nodeTo = static_cast<int>(stateTo + 1u);

            DeserializeComparators(transitionData[i]["transitions"][j]["comparators"], tempData);

            if (wrapper.transitions.find(stateFrom + 1u) == wrapper.transitions.end())
            {
                wrapper.transitions[stateFrom + 1u] = {};
                wrapper.transitions[stateFrom + 1u].push_back(tempData);
            }
            else
            {
                wrapper.transitions[stateFrom + 1u].push_back(tempData);
            }
        }
    }
}

void bee::ai::FiniteStateMachineEditor::LoadEditorData(const nlohmann::json& json, EditorFSMWrapper& wrapper)
{
    for (size_t i = 0; i < json.size(); i++)
    {
        if (i >= wrapper.nodes.size()) return;
        float posX = json[i]["position"]["x"].get<float>();
        float posY = json[i]["position"]["y"].get<float>();
        wrapper.nodes[i]->position = ImVec2(posX, posY);
        wrapper.nodes[i]->name = json[i]["name"].get<std::string>();
        
    }
}

void bee::ai::FiniteStateMachineEditor::LoadFSM()
{
    auto path = Dialogs::OpenFileLoadDialog();
    if (path.empty()) return;
    m_editorFSM.nodes.clear();
    m_editorFSM.transitions.clear();

    std::stringstream stream = std::stringstream(bee::Engine.FileIO().ReadTextFile(bee::FileIO::Directory::None, path));
    nlohmann::json json = nlohmann::json::parse(stream);

    if (!json.contains("behavior-structure-type")) return;
    if (json["behavior-structure-type"] != "FSM") return;

    LoadStates(json["states"], m_editorFSM);
    LoadTransitions(json["transition-data"], m_editorFSM);

    if (json.contains("editor-data"))
    {
        LoadEditorData(json["editor-data"], m_editorFSM);
    }

    m_reloadPositions = true;
}
#endif
