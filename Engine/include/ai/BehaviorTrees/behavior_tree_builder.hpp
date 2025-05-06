#pragma once
#include <iosfwd>
#include <stack>
#include <string>
#include <nlohmann/json.hpp>
#include "behaviors.hpp"

namespace bee::ai
{
class BehaviorTree;

/**
 * \brief builder for a behavior tree. It can be used to construct a behavior tree.
 */
class BehaviorTreeBuilder
{
public:
    template <typename T>
   BehaviorTreeBuilder& Condition(T* t, bool (T::*method)(), bool isNegation = false);
    template <typename T>
   BehaviorTreeBuilder& Condition(T* t, bool (*method)(), bool isNegation = false);
    /**
     * \brief Add an action of type T into the behavior tree (a child of previously created behavior or the last
     * behavior that was lead to by Back()).
     * \tparam T - type of the action to add.
     * \return - Behavior tree builder
     */
    template <typename T, typename... Args>
    BehaviorTreeBuilder& Action(Args... args);
    /**
     * \brief Add a comparison with a comparator that compares against a variable of type T to the behavior tree(a child of previously created behavior or the last
     * behavior that was lead to by Back()).
     * \tparam T - Type to compare against
     * \param comparator - A comparator for comparison behavior
     * \param isNegation - whether or not the comparison should be negated
     * \return - behavior tree builder
     */
    template <typename T>
   BehaviorTreeBuilder& Comparison(Comparator<T> comparator, bool isNegation = false);

    /**
     * \brief Add a selector to the behavior tree (a child of previously created behavior or the last
     * behavior that was lead to by Back())
     * \return - Behavior tree builder
     */
   BehaviorTreeBuilder& Selector();

    /**
     * \brief Add a sequence to the behavior tree (a child of previously created behavior or the last
     * behavior that was lead to by Back())
     * \return - Behavior tree builder
     */
   BehaviorTreeBuilder& Sequence();

    /**
     * \brief  Add a Repeater to the behavior tree (a child of previously created behavior)
     * \param numRepeats - number of times the behavior is executed
     * \return - Behavior tree builder
     */
   BehaviorTreeBuilder& Repeater(int numRepeats);

    /**
     * \brief Add an inverter to the behavior tree (a child of previously created behavior)
     * \return - Behavior tree builder
     */
   BehaviorTreeBuilder& Inverter();

    /**
     * \brief Add an always succeed behavior to the behavior tree (a child of previously created behavior)
     * \return - Behavior tree builder
     */
   BehaviorTreeBuilder& AlwaysSucceed();

    /**
     * \brief Add an UntilFail behavior to the behavior tree (a child of previously created behavior)
     * \return - Behavior tree builder
     */
   BehaviorTreeBuilder& UntilFail();

    
    /**
     * \brief- Back out to the previously created behavior tree node
     * \return - Behavior tree builder
     */
   BehaviorTreeBuilder& Back();
    /**
     * \brief Finish building of the tree, return a unique_ptr to the tree that was created as a result
     * of previously ran builder functions
     * \return - a unique_ptr to the behavior tree
     */
    std::unique_ptr<ai::BehaviorTree> End();
    /**
     * \brief Given a root node of a behavior tree. Return a string stream that is a serialized version
     * of the given behavior tree.
     * \param root - the root node of the behavior tree
     * \return -A string stream representing serialized behavior tree
     */
    nlohmann::json SerializeToJson(const Behavior* root) const;
    /**
     * \brief Given a stringstream created by SerializeToStringStream() return a pointer to a behavior
     * tree that was serialized earlier.
     * \param serializedData - the stringstream created by the SerializeToStringStream method
     * \return - a unique_ptr to a behavior tree.
     */
    std::unique_ptr<ai::BehaviorTree> DeserializeFromJson(nlohmann::json& serializedData);
private:
    int id = 0;
    void SerializeBehavior(nlohmann::json& json, const Behavior* behavior) const;
    void DeserializeAction(std::string actionName, nlohmann::json& editorVariables);
    void DeserializeBehavior(nlohmann::json& stream);
    void AddBehavior(std::unique_ptr<Behavior> behavior);
    std::unique_ptr<Behavior> m_treeRoot = nullptr;
    std::stack<Behavior*> m_nodeStack{};
};

template <typename T>
BehaviorTreeBuilder& BehaviorTreeBuilder::Condition(T* t, bool (T::*method)(), bool isNegation)
{
    auto temp = std::make_unique<ai::Condition>(id++, t, method, isNegation);
    AddBehavior(std::move(temp));
    return *this;
}

template <typename T>
BehaviorTreeBuilder& BehaviorTreeBuilder::Condition(T* t, bool (*method)(), bool isNegation)
{
    auto temp = std::make_unique<ai::Condition>(id++, t, method, isNegation);
    AddBehavior(std::move(temp));
    return *this;
}

template <typename T,typename... Args>
BehaviorTreeBuilder& BehaviorTreeBuilder::Action(Args... args)
{
    auto temp = std::make_unique<T>(args...);
    dynamic_cast<BehaviorTreeAction*>(temp.get())->SetId(id++);
    AddBehavior(std::move(temp));
    return *this;
}

template <typename T>
BehaviorTreeBuilder& BehaviorTreeBuilder::Comparison(Comparator<T> comparator, bool isNegation)
{
    auto temp = std::make_unique<ai::Comparison<T>>(id++, comparator, isNegation);
    AddBehavior(std::move(temp));
    return *this;
}
}
