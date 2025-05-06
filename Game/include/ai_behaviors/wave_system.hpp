#pragma once
#include "core/ecs.hpp"
#include "tools/serializable.hpp"
#include "visit_struct/visit_struct.hpp"
#include "core/geometry2d.hpp"
#include "ai/wave_data.hpp"
#include "rendering/render_components.hpp"
#include "user_interface/user_interface_structs.hpp"


struct WaveIndicator
{
    bee::Entity ent{};
    float lifeTime = 0.0f;
};

class WaveSystem : public bee::System
{
public:
    WaveSystem();
    ~WaveSystem();
    void StartWave(int wave);
    void Update(float dt) override;
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif

    void CalculatePlayerBaseCenter();
    static bee::geometry2d::Polygon GetOutlinesOfAreas(int areaIndex);
    std::vector<std::string>::value_type GetRandomEnemyWeighted();
    int GetCurrentWave() const;
    void ClearText();
    void StartWave();
    const std::vector<glm::vec2>& GetEnemySpawnLocation(); 
    bool active = true;
    float m_circleRotationDelta =1.0f;
    float m_upDownFrequency = 5.0f;
    float m_upDownAmplitude = 0.3f;

private:
    void GenerateRandomPositions(int numberOfEnemies);
    void SpawnEnemies(int waveNumber);
    glm::vec2 GetPointOnARing(glm::vec2 ringCenter, float ringRadius, float ringThickness) const;

        std::shared_ptr<bee::Material> m_indicatorMaterial{};
    int GetNumEnemiesToSpawn(int wave) const;
    std::vector<int> EnemiesToSpawnPerType(const std::vector<int>& weights, int totalNumberOfEnemiesToSpawn);
    WaveData m_waveData{};
    std::vector<int> m_numberOfEnemiesPerType{};
    float m_waveTimer = 0.0f;
    glm::vec2 m_ringCenter = {0, 0};
    bool m_isWaveOn = false;
    bool m_paused = false;
    int m_currentWave = 0;
    int m_areaIncrement = 3;
    std::vector<glm::vec2> m_spawnPositions{};
    bee::ui::UIElementID m_element = entt::null;
    bee::ui::UIElementID m_nextWave = entt::null;
    int m_currentArea = 1;
};


