#include "mission_editor.hpp"

#include <platform/dx12/animation_system.hpp>

#include "actors/selection_system.hpp"
#include "actors/units/unit_manager_system.hpp"
#include "ai/behavior_editor_system.hpp"
#include "animation/animation_state.hpp"
#include "camera/camera_editor_system.hpp"
#include "camera/camera_rts_system.hpp"
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/transform.hpp"
#include "level_editor/level_editor_system.hpp"
#include "light_system/light_system.hpp"
#include "material_system/material_system.hpp"
#include "particle_system/particle_system.hpp"
#include "rendering/render_components.hpp"
#include "tools/asset_explorer_system.hpp"
#include "user_interface/user_interface_editor.hpp"
#include "ai/wave_data.hpp"

#pragma once

void MissionEditor::Init()
{
    // GameBase::Init();

    bee::Engine.ECS().CreateSystem<bee::MaterialSystem>();
    bee::Engine.ECS().CreateSystem<bee::RenderPipeline>();
    bee::Engine.ECS().CreateSystem<bee::AssetExplorer>();
    bee::Engine.ECS().CreateSystem<bee::CameraSystemEditor>();
    bee::Engine.ECS().CreateSystem<bee::AnimationSystem>();
    bee::Engine.ECS().CreateSystem<lvle::LevelEditor>();
    bee::Engine.ECS().CreateSystem<bee::ai::BehaviorEditorSystem>();
    bee::Engine.ECS().CreateSystem<bee::ui::UserInterface>();
    bee::Engine.ECS().CreateSystem<bee::ui::UIEditor>();
    bee::Engine.ECS().CreateSystem<bee::ParticleSystem>();

    bee::Engine.ECS().CreateSystem<bee::LightSystem>();
    {
        // Create key directional light
        auto lightEntity = bee::Engine.ECS().CreateEntity();
        auto& light = bee::Engine.ECS().CreateComponent<bee::Light>(lightEntity, glm::vec3(1.f), 100.f, 100.f,
                                                                    bee::Light::Type::Directional);
        light.ShadowExtent = 0.2f;
        light.Intensity = 1000.0f;
        light.CastShadows = true;
        auto& transform = bee::Engine.ECS().CreateComponent<bee::Transform>(lightEntity);
        transform.Rotation = glm::quatLookAt(glm::normalize(glm::vec3(-1, -2, -3)), glm::vec3(0, 0, 1));
        transform.Name = "Directional Light";
        transform.Translation.z = 0.330f;
        transform.Translation.x = 0.460f;
        transform.Translation.y = 0.330f;

    }

}

void MissionEditor::ShutDown()
{
    // GameBase::ShutDown();
}

void MissionEditor::Update(float dt)
{
    // GameBase::Update(dt);
}
