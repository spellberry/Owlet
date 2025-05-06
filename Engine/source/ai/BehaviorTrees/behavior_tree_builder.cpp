
#include "ai/BehaviorTrees/behavior_tree_builder.hpp"

#include "ai/BehaviorTrees/behavior_tree.hpp"
#include "ai/Utils/editor_variables.hpp"
#include "ai/Utils/generic_factory.hpp"

bee::ai::BehaviorTreeBuilder& bee::ai::BehaviorTreeBuilder::Selector()
{
    auto temp = std::make_unique<ai::Selector>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

void bee::ai::BehaviorTreeBuilder::AddBehavior(std::unique_ptr<Behavior> behavior)
{
    const auto ptr = behavior.get();
    if (m_treeRoot == nullptr)
    {
        m_treeRoot = std::move(behavior);
    }
    else
    {
        m_nodeStack.top()->AddChild(std::move(behavior));
    }

    m_nodeStack.push(ptr);
}

bee::ai::BehaviorTreeBuilder& bee::ai::BehaviorTreeBuilder::Sequence()
{
    auto temp = std::make_unique<ai::Sequence>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

bee::ai::BehaviorTreeBuilder& bee::ai::BehaviorTreeBuilder::Repeater(int numRepeats)
{
    auto temp = std::make_unique<ai::Repeater>(id++, numRepeats);
    AddBehavior(std::move(temp));
    return *this;
}

bee::ai::BehaviorTreeBuilder& bee::ai::BehaviorTreeBuilder::Inverter()
{
    auto temp = std::make_unique<ai::Inverter>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

bee::ai::BehaviorTreeBuilder& bee::ai::BehaviorTreeBuilder::AlwaysSucceed()
{
    auto temp = std::make_unique<ai::AlwaysSucceed>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

bee::ai::BehaviorTreeBuilder& bee::ai::BehaviorTreeBuilder::UntilFail()
{
    auto temp = std::make_unique<ai::UntilFail>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

bee::ai::BehaviorTreeBuilder& bee::ai::BehaviorTreeBuilder::Back()
{
    m_nodeStack.pop();
    return *this;
}

std::unique_ptr<bee::ai::BehaviorTree> bee::ai::BehaviorTreeBuilder::End()
{
    auto behavior_tree = std::make_unique<ai::BehaviorTree>(m_treeRoot);
    return std::move(behavior_tree);
}

nlohmann::json bee::ai::BehaviorTreeBuilder::SerializeToJson(const Behavior* root) const
{
    nlohmann::json toReturn;

    toReturn["version"] = "1.0";
    toReturn["children"] = nlohmann::json::array();
    toReturn["behavior-structure-type"] = "BehaviorTree";

    SerializeBehavior(toReturn, root);
    return toReturn;
}

std::unique_ptr<bee::ai::BehaviorTree> bee::ai::BehaviorTreeBuilder::DeserializeFromJson(nlohmann::json& serializedData)
{
    std::string version = serializedData["version"];
    if (serializedData["behavior-structure-type"].get<std::string>() != "BehaviorTree") return {};
    if (!serializedData.contains("children")) return {};
    auto toDeserialize = serializedData.at("children")[0];
    id = 0;
    DeserializeBehavior(toDeserialize);
    return End();
}

void bee::ai::BehaviorTreeBuilder::SerializeBehavior(nlohmann::json& json, const Behavior* behavior) const
{
    nlohmann::json node;
    node["children"] = nlohmann::json::array({});

    auto name = std::string(typeid(*behavior).name());

    size_t pos = name.find('<');

    if (pos != std::string::npos)
    {
        name = name.substr(0, pos);
    }

    std::size_t i = name.find("class");
    if (i != std::string::npos)
    {
        name.erase(i, 5);
    }

    i = name.find("ai::");
    if (i != std::string::npos)
    {
        name.erase(i, 4);
    }

    const auto action = dynamic_cast<const ai::BehaviorTreeAction*>(behavior);

    node["name"] = name;

    if (action != nullptr)
    {
        node["name"] = "Action";
        node["type"] = name;
        node["editor-variables"] = nlohmann::json();

        for (auto& variable : action->editorVariables)
        {
            nlohmann::json jsonVariable;
            jsonVariable["name"] = variable.first;
            jsonVariable["value"] = variable.second->ToString();
            node["editor-variables"].push_back(jsonVariable);
        }
    }

    const auto composite = dynamic_cast<const Composite*>(behavior);
    const auto decorator = dynamic_cast<const Decorator*>(behavior);
    if (composite != nullptr)
    {
        for (const auto& element : composite->GetChildren())
        {
            SerializeBehavior(node, element.get());
        }
    }
    else if (decorator != nullptr)
    {
        decorator->Serialize(node);
        SerializeBehavior(node, decorator->GetChild().get());
    }

    nlohmann::json children = json.at("children");
    children.push_back(node);
    json["children"] = children;
}

void bee::ai::BehaviorTreeBuilder::DeserializeAction( std::string actionName,nlohmann::json& editorVariables)
{
    actionName.erase(std::remove_if(actionName.begin(), actionName.end(), isspace), actionName.end());
    auto temp = GenericFactory<ai::BehaviorTreeAction>::Instance().CreateProduct(actionName);
    if (temp.get() != nullptr)
    {
        temp->SetId(id++);

        for (auto variable : editorVariables)
        {
            if (temp->editorVariables.find(variable["name"]) == temp->editorVariables.end()) continue;
            temp->editorVariables[variable["name"]]->Deserialize(variable["value"]);
        }

        AddBehavior(std::move(temp));
        return;
    }

    AddBehavior(std::make_unique<BehaviorTreeAction>());
}

#define PARSE(type, comparisonType) \
    type value;                                 \
    stream >> value;                            \
    Comparison(ai::Comparator<type>(comparisonKey, static_cast<ComparisonType>(comparisonType), value));

void bee::ai::BehaviorTreeBuilder::DeserializeBehavior(nlohmann::json& json)
{
    std::string name = json.at("name").get<std::string>();
    name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());

    nlohmann::json children = json["children"];
    size_t numChildren = children.size();


    if (name == "Selector")
    {
        Selector();
    }
    if (name == "Sequence")
    {
        Sequence();
    }
    if (name == "Repeater")
    {
        int numRepeats = json["num-repeats"];
        Repeater(numRepeats);
    }
    if (name == "Inverter")
    {
        Inverter();
    }
    if (name == "AlwaysSucceed")
    {
        AlwaysSucceed();
    }
    if (name == "UntilFail")
    {
        UntilFail();
    }
    if (name.find("Comparison") != std::string::npos)
    {
        std::stringstream stream = std::stringstream(json["comparator"].dump());

        std::string comparisonKey;
        std::string typeName;
        int comparisonType;

        stream >> comparisonKey;
        stream >> typeName;
        stream >> comparisonType;

        auto new_end = std::remove_if(comparisonKey.begin(), comparisonKey.end(), [](char c) { return !isalpha(c); });
        comparisonKey.erase(new_end, comparisonKey.end());

        if (typeName == "float")
        {
            PARSE(float, comparisonType)
        }
        if (typeName == "double")
        {
            PARSE(double, comparisonType)
        }
        if (typeName == "bool")
        {
            PARSE(bool, comparisonType)
        }
        if (typeName == "int")
        {
            PARSE(int, comparisonType)
        }
        if (typeName.find("string") != std::string::npos)
        {
            PARSE(std::string, comparisonType)
        }
    }
    if (name == "Action")
    {
        std::string actionName = json["type"];
        DeserializeAction(actionName, json["editor-variables"]);
    }

    for (int i = 0; i < numChildren; i++)
    {
        DeserializeBehavior(children[i]);
    }

    if (m_nodeStack.empty()) return;
    Back();
};
