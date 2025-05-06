#include "ai/BehaviorTrees/behavior_tree.hpp"
#include "ai/BehaviorTrees/behavior_tree_builder.hpp"

void bee::ai::BehaviorTree::Execute(bee::ai::BehaviorTreeContext& context) const
{
    m_root->Execute(context);
}

std::stringstream bee::ai::BehaviorTree::Serialize() const
{
    BehaviorTreeBuilder builder{};
    return std::stringstream(builder.SerializeToJson(m_root.get()).dump());
}

std::unique_ptr<bee::ai::BehaviorTree> bee::ai::BehaviorTree::Deserialize(std::stringstream& stringstream)
{
    BehaviorTreeBuilder builder{};
    if (stringstream.str().empty()) return {};
    nlohmann::json json = nlohmann::json::parse(stringstream.str());
    auto temp = builder.DeserializeFromJson(json);
    return temp;
}

bee::ai::BehaviorTree::BehaviorTree(std::unique_ptr<Behavior>& rootToSet)
{
    m_root.swap(rootToSet);
}
