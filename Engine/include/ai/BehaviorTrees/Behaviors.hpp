#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "ai/Blackboards/blackboard.hpp"
#include "ai/Blackboards/comparator.hpp"
#include "ai/Utils/execution_context.hpp"

namespace bee::ai
{
class EditorVariable;
struct BehaviorTreeContext;

    /**
     * \brief Execution status of a given behavior
     */
    enum class Status
    {
        INVALID = 0,
        SUCCESS = 1,
        RUNNING = 2,
        FAILURE = 3,
        ABORTED = 4
    };

    /**
     * \brief Base class for behavior tree execution context. Contains all useful elements for
     * Behavior Tree execution in a fly weight fashion.
     */
    struct BehaviorTreeContext :public AIExecutionContext
    {
        float deltaTime = 0.0f;
        std::unique_ptr<Blackboard> blackboard = std::make_unique<Blackboard>();
        std::unordered_map<int, ai::Status> statuses;
    };


    /**
     * \brief Base class for a behavior tree behavior. 
     */
    class Behavior
    {
    public:
        Behavior(int id) : m_id(id){}
        virtual ~Behavior() = default;

        /**
         * \brief Resets this behavior's status within a given BehaviorTreeContext
         * \param context - a behavior tree execution context
         */
        virtual void Reset(BehaviorTreeContext& context);
        /**
         * \brief Execute a behavior, return the status of the execution.
         * \param context -a behavior tree execution context
         * \return - A status of the behavior
         */
        virtual Status Execute(BehaviorTreeContext& context);
        /**
         * \brief Start function called when the behavior starts- initializes
         * all the important data
         * \param context - a behavior tree execution context
         */
        virtual void Initialize(BehaviorTreeContext& context) {}
        /**
         * \brief An update function of a behavior. Called on execution after initialize
         * and until the behavior ends
         * \param context - a behavior tree execution context
         * \return - Status at the end of the tick. If it is not running the behavior ends with a
         * given status.
         */
        virtual Status Tick(BehaviorTreeContext& context) { return Status::SUCCESS; }

        /**
         * \brief Called as a 'clean up' after the behavior stops running.
         * \param context - a behavior tree execution context
         * \param status - The status with which the behavior execution ends
         */
        virtual void End(BehaviorTreeContext& context, Status status) {}


        /**
         * \brief A base function for adding a child to the behavior
         * \param child - a unique_ptr to a child to add
         */
        virtual void AddChild(std::unique_ptr<Behavior> child) { }
        /**
         * \brief remove a given child from the behavior
         * \param child - child to remove
         */
        virtual void RemoveChild(Behavior* child){};
        /**
         * \brief Remove all children from the behavior
         */
        virtual void ClearChild() {}
        /**
         * \brief A getter for the behavior id
         * \return - behavior id
         */
        int GetId() const { return m_id; }

    protected:
        int m_id = 0;
    };

    /**
     * \brief A base for an action behavior. It is meant to be a leaf of the behavior tree
     * it is supposed to execute actions.
     */
    class BehaviorTreeAction : public Behavior
    {
    public:
        BehaviorTreeAction() : Behavior(-1){}
        void SetId(int id) { m_id = id; }

        /**
         * \brief Register a variable for serialization
         * \param name - the name of the variable
         * \param variable - a reference to the variable to be serialized
         */
        void RegisterEditorVariable(const std::string& name, EditorVariable* variable)
        {
            editorVariables[name] = variable;
        }

        std::unordered_map<std::string, EditorVariable*> editorVariables{};
    };

    /**
     * \brief A base behavior with multiple children.
     */
    class Composite : public Behavior
    {
    public:
        Composite(int id) : Behavior(id) {}

        void Reset(BehaviorTreeContext& context) override;

        /**
         * \brief A base function for adding a child to the behavior
         * \param child - a unique_ptr to a child to add
         */
        virtual void AddChild( std::unique_ptr<Behavior> child)
        {
            m_children.push_back(std::move(child));
        }
        /**
         * \brief remove a given child from the behavior
         * \param child - child to remove
         */
        void RemoveChild(Behavior* child);
        /**
         * \brief Remove all children from the behavior
         */
        void ClearChild() { m_children.clear(); }
        /**
         * \brief A getter function for all children of the behavior
         * \return - the vector of children
         */
        const std::vector<std::unique_ptr<Behavior>>& GetChildren()const { return m_children; }
    protected:
        std::vector<std::unique_ptr<Behavior>> m_children;
    };

    /**
     * \brief A base class for a behavior with one child. 
     */
    class Decorator : public Behavior
    {
    public:
        Decorator(int id) : Behavior(id){}

        void Reset(BehaviorTreeContext& context) override;

        /**
         * \brief Set the child of the behavior
         * \param child - a unique_ptr to a child to add
         */
        virtual void AddChild(std::unique_ptr<Behavior> child)
        {
            m_child = std::move(child);
        }

        /**
         * \brief - Remove the child
         */
        void RemoveChild() { m_child.reset(); }
        /**
         * \brief A getter function for the child of the decorator
         * \return - a unique_ptr to the child
         */
        const std::unique_ptr<Behavior>& GetChild() const { return m_child;}
        /**
         * \brief A custom serialization function for a decorator behavior
         * \param stream - a stream the decorator is going to be serialized into.
         */
        virtual void Serialize(nlohmann::json& json) const{}
    protected:
        std::unique_ptr<Behavior> m_child{};
    };

    /**
     * \brief Given a comparator of type T, returns a fail or success based on its evaluation of
     * the given execution context
     * \tparam T - Type of the variable the comparator is going to compare against
     */
    template<typename T>
    class Comparison :public Decorator
    {
    public:
        Comparison(int id, Comparator<T> comparator, bool isNegation = false): Decorator(id), m_comparator(comparator), m_isNegation(isNegation){};
        /**
         * \brief Tick returns the result of the evaluation of the Comparator based on the blackboard
         * of the context. If the evaluation succeeds: it returns SUCCESS, if it fails: it returns FAIL
         * \param context - a behavior tree execution context
         * \return - The result of the evaluation
         */
        Status Tick(BehaviorTreeContext& context) override;

        /**
         * \brief Serialization function of the comparator
         * \param stream - the stream the comparator is going to be serialized to
         */
        void Serialize(nlohmann::json& json) const override
        {
            json["comparator"]= m_comparator.ToString();
        }
    private:
        Comparator<T> m_comparator;
        bool m_isNegation = false;
    };

    class Condition : public Decorator
    {
    public:
        template<typename T>
        Condition(int id, T* t, bool (T::*method)(), bool isNegation = false) : Decorator(id), m_isNegation(isNegation)
        {
            const std::function<bool()> tempFunction = [method,t] { return (t->*method)(); };
            m_function = tempFunction;
        }

        Condition(int id, bool (*method)(), bool isNegation = false) : Decorator(id), m_isNegation(isNegation)
        {
            const std::function<bool()> tempFunction = [method]() { return (*method)(); };
            m_function = tempFunction;
        }
        Status Tick(BehaviorTreeContext& context) override;
    private:
        bool m_isNegation = false;
        std::function<bool()> m_function;
    };

    template <typename T>
    Status Comparison<T>::Tick(BehaviorTreeContext& context)
    {
        if (m_comparator.Evaluate(*context.blackboard))
        {
            if (!m_isNegation)
            {
                auto execute = m_child->Execute(context);
                return execute;
            }
            else
            {
                return Status::FAILURE;
            }
        }

        if (m_isNegation)
        {
            auto execute = m_child->Execute(context);
            return execute;   
        }

        return Status::FAILURE;
    }

    /**
     * \brief A composite that executes its children until one of them fails.
     */
    class Sequence : public Composite
    {
    public:
        Sequence(int id) : Composite(id){}
        Status Tick(BehaviorTreeContext& context) override;
    };

    /**
     * \brief A composite that executes its children until one of them succeeds
     */
    class Selector : public Composite
    {
    public:
        Selector(int id) : Composite(id) {}
        Status Tick(BehaviorTreeContext& context) override;
    };

    /**
     * \brief A decorator that executes its child a given amount of time
     */
    class Repeater : public Decorator
    {
    public:
        Repeater(int id, int repeats) : Decorator(id), m_numRepeats(repeats) {}
        Status Tick(BehaviorTreeContext& context) override;

        void Serialize(nlohmann::json& json) const override
        {
            json["num-repeats"] = m_numRepeats;
        }
    private:
        int m_numRepeats = 0;
    };

    /**
     * \brief A behavior that returns an inverted result of its child execution-
     * Fail is a SUCCESS and a success is a FAIL.
     */
    class Inverter : public Decorator
    {
    public:
        Inverter(const int id): Decorator(id){}
        Status Tick(BehaviorTreeContext& context) override;
    };


    /**
     * \brief A behavior that returns a SUCCESS whatever happens when this behavior executes
     */
    class AlwaysSucceed : public Decorator
    {
    public:
        AlwaysSucceed(const int id) : Decorator(id) {}
        Status Tick(BehaviorTreeContext& context) override;
    };

    /**
     * \brief A behavior that executes its child until it fails.
     */
    class UntilFail: public Decorator
    {
    public:
        UntilFail(const int id) : Decorator(id) {}
        Status Tick(BehaviorTreeContext& context) override;
    };
}
