#pragma once
#if defined(BEE_PLATFORM_PC)
#include <imgui/imgui_node_editor.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "ai/Utils/editor_utils.hpp"
#include "ai/Utils/editor_variables.hpp"

namespace bee::ai
{
struct StateMachineContext;

    struct NodeWrapper
    {
        NodeWrapper(){};
        ImVec2 position{0, 0};
        size_t id = 1;
        std::string name = "Test";    
        std::string stateType;
        std::unordered_map<std::string, std::string> editorVariables{};
        std::unique_ptr<State> underlyingState;
    };

    struct NodeTransition
    {
        int nodeTo = -1;
        std::vector<ComparatorWrapper> comparators{};
    };

    struct EditorFSMWrapper
    {
        std::vector<std::unique_ptr<NodeWrapper>> nodes;
        std::unordered_map<size_t, std::vector<NodeTransition>> transitions;
    };

    class FiniteStateMachineEditor
    {
    public:
        FiniteStateMachineEditor();
        void Update();
        void PreviewContext(const FiniteStateMachine& fsm, const StateMachineContext& context);
    private:

        std::vector<size_t> m_selectedNodes{};

        size_t m_hovered = -1;

        ImVec2 m_mousePositionOnMenu = {};
        std::vector<size_t> m_removedIds;
        ax::NodeEditor::EditorContext* m_previewContext = nullptr;

        const FiniteStateMachine* m_previewedFSM = nullptr;

        EditorFSMWrapper m_editorFSM;
        EditorFSMWrapper m_previewFSM;

        bool m_clicked = false;
        bool m_justOpenedMenu = false;
        bool m_reloadPositions = true;

        ax::NodeEditor::EditorContext* m_context = nullptr;
        size_t m_defaultState = 0;
        int m_maxNumStates = 1000;

        void DrawTransitions(EditorFSMWrapper& wrapper) const;
        void DeleteNode(size_t nodeIdToRemove);
        void ResetHoverInfo();
        void DrawNode(const ai::EditorFSMWrapper& wrapper, size_t i);

        void DrawNodes(const EditorFSMWrapper& wrapper);
        void DrawNodes(const EditorFSMWrapper& wrapper, const StateMachineContext& context);
        std::vector<std::string> GetStateNames() const;

        void DrawSelectedStateDetails();
        static void SelectedStateNameInputBox(const std::unique_ptr<ai::NodeWrapper>& focusedNode);
        void SelectedStateTypeSelection(const std::unique_ptr<ai::NodeWrapper>& focusedNode) const;
        void SelectedNodeTransitionButtons(const std::unique_ptr<ai::NodeWrapper>& focusedNode, int index, int stateToGoToChoice,ai::NodeTransition& transition) const;
        void SelectedNodeTransitionComparatorButtons(int index, std::vector<int>& transitionsToRemove,ai::NodeTransition& transition) const;
        void ComparatorValueInput(ComparatorWrapper& comparator, const char* label) const;
        void ChooseComparatorTypeInput(std::vector<ai::ComparatorWrapper>::value_type& comparator, const char* label) const;
        void ChooseComparisonTypeInput(std::vector<ai::ComparatorWrapper>::value_type& comparator, const char* label) const;

        void DrawUpperMenu();

        void SaveStates(nlohmann::json& states) const;
        void SaveTransitions(nlohmann::json& json) const;
        void SaveEditorData(nlohmann::json& json) const;
        void HandleClicks();
        void DrawPopupMenu();

        void SaveFSM() const;
        size_t GetIdToSet();
        void CreateNode();
        void LoadStates(nlohmann::json& states, EditorFSMWrapper& wrapper);
        static void DeserializeComparators(nlohmann::json& comparators, NodeTransition& node_transition);
        static void LoadTransitions(nlohmann::json& transitionData, EditorFSMWrapper& wrapper);
        static void LoadEditorData(const nlohmann::json& json, EditorFSMWrapper& wrapper);
        void LoadFSM();
    };
}
#endif