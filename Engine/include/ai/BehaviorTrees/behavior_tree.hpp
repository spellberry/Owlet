#pragma once
#include <memory>
#include "behaviors.hpp"

namespace bee::ai
{
/**
 * \brief A class for a behavior tree. It can be executed using an Execute() function and a
 * behavior tree execution context.
 */
class BehaviorTree
    {
    public:
        BehaviorTree(std::unique_ptr<Behavior>& rootToSet);
        /**
         * \brief This function executes its children based on their behaviors.
         * \param context - A behavior tree execution context
         */
        void Execute(BehaviorTreeContext& context) const;
        /**
         * \brief A getter for the root 
         * \return - a pointer to the root behavior
         */
        std::unique_ptr<Behavior>& GetRoot()  { return m_root; }

        std::stringstream Serialize() const;
        static std::unique_ptr<BehaviorTree> Deserialize(std::stringstream& stringstream);
    private:
        std::unique_ptr<Behavior> m_root = {};
    };
}

