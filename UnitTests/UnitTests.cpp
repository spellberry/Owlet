#include <pch.h>
#include "CppUnitTest.h"
#include "actors/attributes.hpp"
#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "core/ecs.hpp"
#include "core/transform.hpp"
#include "tools/3d_utility_functions.hpp"
#include "core/engine.hpp"
#include "physics/physics_components.hpp"

struct IdleState : public bee::ai::State
{
    
};

namespace UnitTests
{
TEST_CLASS(UnitTests){public :

    TEST_METHOD(TestMethod1){
        Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(true, true);
}

TEST_METHOD(RaycastSystemTest)
{
    bee::Engine.InitializeHeadless();
    const auto entity = bee::Engine.ECS().CreateEntity();
    auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(entity);
    bee::Engine.ECS().CreateComponent<bee::physics::DiskCollider>(entity, 1.0f);
    bee::Engine.ECS().CreateComponent<AttributesComponent>(entity);
    transform.Translation.x = 10;
    transform.Translation.y = 10;
    bee::Entity a;

    // This should hit
    bool response = bee::HitResponse(a, glm::vec3(5, 5, 0), 100, glm::vec3(11, 11, 0));
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(true, response);

    // This should not hit: cause length
    transform.Translation = {1000, 1000, 0};
    response = bee::HitResponse(a, glm::vec3(5, 5, 0), 10, glm::vec3(11, 11, 0));
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(false, response);

    // This shouldn't hit- Z
    response = bee::HitResponse(a, glm::vec3(5, 5, 0), 1000, glm::vec3(11, 11, 11));
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(false, response);

    // This shouldn't hit the object is in the opposite direction of the ray
    transform.Translation = {-10, -10, 0};
    response = bee::HitResponse(a, glm::vec3(5, 5, 0), 100, glm::vec3(11, 11, 0));
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(false, response);
    bee::Engine.Shutdown();
}

TEST_METHOD(BehaviorSelectionSystemTest)
{
    bee::ai::FiniteStateMachine fsm{};
    auto idle1 = fsm.AddState<IdleState>();
    auto idle2 = fsm.AddState<IdleState>();
    auto idle3 = fsm.AddState<IdleState>();

    // Finite state machine supports multiple states of the same type
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(fsm.GetStateIDsOfType<IdleState>().empty(), false);
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(fsm.GetStateIDsOfType<IdleState>().size(), static_cast<size_t>(3));
}
}
;
}
