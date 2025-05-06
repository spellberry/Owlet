#pragma once
#include "ai/Blackboards/blackboard.hpp"

namespace bee::ai
{
    class BlackboardInspector
    {
    public:
        BlackboardInspector() = default;
        void Inspect(ai::Blackboard& blackboardToInspect);
        bool open = false;
    };
}
