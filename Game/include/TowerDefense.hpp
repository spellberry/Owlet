#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/game_base.hpp"
#include "rendering/image.hpp"
#include "tools/log.hpp"

class TowerDefense : public bee::GameBase
{
public:
    void Init() override;
    void ShutDown() override;
    void Update(float dt) override;
    void Render() override;

private: // functions
    void SpawnWarrior();

private:  // params
    float m_spawnTimer = 0.0f;
    float m_spawnCooldown = 5.0f;
    int m_enemySpawnCount = 0;
    std::string m_levelName = "TowerDefense";
    bool orderUnitToggle = false;
};
