#include "ai/BehaviorTrees/behaviors.hpp"



void bee::ai::Behavior::Reset(BehaviorTreeContext& context)
{
    context.statuses[m_id] = Status::INVALID;
}

bee::ai::Status bee::ai::Behavior::Execute(BehaviorTreeContext& context)
{
    Status& status = context.statuses[m_id];
    if (status != Status::RUNNING)
    {
        Initialize(context);
    }

    status = Tick(context);

    if (status != Status::RUNNING)
    {
        End(context, status);
    }

    context.statuses[m_id] = status;
    return status;
}

void bee::ai::Composite::Reset(BehaviorTreeContext& context)
{
    Behavior::Reset(context);
    for (auto& child : m_children)
    {
        child->Reset(context);
    }
}

void bee::ai::Composite::RemoveChild(Behavior* child)
{
    m_children.erase(std::remove_if(m_children.begin(), m_children.end(),[child](const std::unique_ptr<Behavior>& behavior)
    {
        return behavior.get() == child;
    }), m_children.end());
}

void bee::ai::Decorator::Reset(BehaviorTreeContext& context)
{
    Behavior::Reset(context);
    m_child->Reset(context);
}

bee::ai::Status bee::ai::Condition::Tick(BehaviorTreeContext& context)
{
    if (m_function())
    {
        if (!m_isNegation)
        {
            m_child->Execute(context);
        }
        return m_isNegation ? Status::FAILURE : Status::SUCCESS;
    }

    if (m_isNegation)
    {
        m_child->Execute(context);
    }
    return m_isNegation ? Status::SUCCESS : Status::FAILURE;
}

bee::ai::Status bee::ai::Sequence::Tick(BehaviorTreeContext& context)
{
    bool allSuccess = true;
    for (const auto& child : m_children)
    {
        if (context.statuses[child->GetId()] == Status::SUCCESS) continue;
        allSuccess = false;
        const Status childStatus = child->Execute(context);
        if (childStatus != Status::SUCCESS)
        {
            return childStatus;
        }
    }

    if (allSuccess)
    {
        for (const auto& child : m_children)
        {
            child->Reset(context);
        }
    }

    return Status::SUCCESS;
}

bee::ai::Status bee::ai::Selector::Tick(BehaviorTreeContext& context)
{
    for (const auto& child : m_children)
    {
        const Status childStatus = child->Execute(context);
        if (childStatus != Status::FAILURE)
        {
            for (auto& element : m_children)
            {
                if (element == child)continue;
                element->Reset(context);
            }
            return childStatus;
        }
    }

    return Status::FAILURE;
}

bee::ai::Status bee::ai::Repeater::Tick(BehaviorTreeContext& context)
{
    for (int i = 0 ; i < m_numRepeats;i++)
    {
        m_child->Execute(context);
        if (context.statuses[m_id] == Status::RUNNING) return Status::SUCCESS;
        if (context.statuses[m_id] == Status::FAILURE) return Status::FAILURE;
        m_child->Reset(context);
    }

    return Status::SUCCESS;
}

bee::ai::Status bee::ai::Inverter::Tick(BehaviorTreeContext& context)
{
    const Status status = m_child->Execute(context);
    if(status == Status::FAILURE) return Status::SUCCESS;
    if(status == Status::SUCCESS) return Status::FAILURE;
    return Status::INVALID;
}

bee::ai::Status bee::ai::AlwaysSucceed::Tick(BehaviorTreeContext& context)
{
    m_child->Execute(context);
    return Status::SUCCESS;
}

bee::ai::Status bee::ai::UntilFail::Tick(BehaviorTreeContext& context)
{
    auto status = m_child->Execute(context);
    if ( status != Status::FAILURE)
    {
        return Status::SUCCESS;
    }

    return Status::FAILURE;
}
