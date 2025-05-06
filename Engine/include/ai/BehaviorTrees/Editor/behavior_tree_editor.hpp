#pragma once
#if defined(BEE_PLATFORM_PC)
#include <imgui/imgui.h>

#include <any>
#include <optional>
#include <string>
#include <imgui/imgui_node_editor.h>

#include <vector>

#include "ai/BehaviorTrees/behaviors.hpp"
#include "ai/BehaviorTrees/behavior_tree.hpp"
#include "ai/Utils/editor_utils.hpp"

namespace bee::ai
{
    inline constexpr int maxAmountNodes = 1000;

    struct BehaviorWrapper
    {
        int id = 1;
        ImVec2 position;

        std::string name = "root";
        std::string behaviorType;
        std::vector<BehaviorWrapper> children{};
        BehaviorsInBTEditor::BehaviorWrapperType type = BehaviorsInBTEditor::BehaviorWrapperType::DECORATOR;

        bool justAdded = true;
        std::any additionalInfo;
        std::unique_ptr<BehaviorTreeAction> underlyingAction{};
        std::optional<std::string> action{};

        void DrawNode(size_t transitionIndex, const BehaviorTreeContext& context);
        void DrawNode(size_t transitionIndex);
        void AddChild(BehaviorWrapper child)
        {
            if (type == BehaviorsInBTEditor::BehaviorWrapperType::ACTION) return;
            if (type == BehaviorsInBTEditor::BehaviorWrapperType::DECORATOR && children.size() == 1)
            {
                return;
            }

            children.push_back(std::move(child));
        }
        void ClearChildren(std::vector<int>& ids);
    private:
        void DrawNodeContent();
    };

    class BehaviorTreeEditor
    {
    public:
        BehaviorTreeEditor();
        void Update();
        void PreviewContext(const BehaviorTree& bt, const BehaviorTreeContext& context);
    private:
        void DrawSelectedStateDetails();
        void DrawComparisonDetails(ai::BehaviorWrapper& selected) const;
        void DrawDecoratorChildMenu(BehaviorWrapper& selected);
        void DrawCompositeChildMenu(BehaviorWrapper& selected);
        void DrawActionContextMenu(BehaviorWrapper& selected) const;
        void DrawChildMenu(ai::BehaviorWrapper& selected);
        void CheckSelectedNode(BehaviorWrapper& behavior);
        void DrawNodes();
        void DrawNodesPreview(const BehaviorTreeContext& context);
        void DrawUpperMenu();

        void LoadBehavior(nlohmann::json& json, BehaviorWrapper& behavior, int& idToSet);
        void LoadBT();
        void SaveBT();

        void SaveBehavior(nlohmann::json& stream, const BehaviorWrapper& behavior);

        int GetBiggestId(const BehaviorWrapper& node);
        int GetIdToSet();

        BehaviorWrapper editorRoot;
        BehaviorWrapper previewRoot;

        const BehaviorTree* previewedTree = nullptr;

        ax::NodeEditor::EditorContext* m_context = nullptr;
        ax::NodeEditor::EditorContext* m_previewContext = nullptr;
        std::vector<std::reference_wrapper<BehaviorWrapper>> m_selectedNodes{};
        std::vector<int> m_removedIds;
    };

}
#endif
