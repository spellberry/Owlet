#include "ai_behaviors/wave_system.hpp"

#include <cdt/Triangulation.h>

#include "actors/structures/structure_manager_system.hpp"
#include "ai_behaviors/enemy_ai_behaviors.hpp"
#include "game_ui/in_game_ui.hpp"
#include "rendering/debug_render.hpp"
#include "tools/convex_hull.hpp"
#include "tools/tools.hpp"
#include "user_interface/user_interface.hpp"
#include "user_interface/user_interface_serializer.hpp"

UI_FUNCTION(ToNextWave, StartNextWave,

    bee::Engine.ECS().GetSystem<WaveSystem>().StartWave(););

WaveSystem::WaveSystem()
{
    Title = "WaveSystem";
    m_waveTimer = m_waveData.delayBetweenWaves;

    if (bee::Engine.FileIO().Exists(bee::FileIO::Directory::Asset, "Wave data.json"))
    {
        bee::Engine.Serializer().SerializeFrom("Wave data.json", m_waveData);
    }
    else
    {
        bee::Engine.Serializer().SerializeTo("Wave data.json", m_waveData);
    }
    m_element = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>().serialiser->LoadElement("Wave_Counter");
    m_nextWave = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>().serialiser->LoadElement("NextWave");
    bee::Engine.ECS().GetSystem<bee::ui::UserInterface>().SetElementLayer(m_element, 5);
    m_indicatorMaterial = bee::Engine.Resources().Load<bee::Material>("materials/EnemyIndicator.pepimat");
    auto& transform = bee::Engine.ECS().Registry.get<bee::Transform>(m_element);
    transform.Translation.x = 0.8f;
    transform.Translation.y = 0.0f;
    bee::Engine.ECS().GetSystem<bee::ui::UserInterface>().SetDrawStateUIelement(m_nextWave, false);
    GetOutlinesOfAreas(0);
}

WaveSystem::~WaveSystem() {}

void WaveSystem::StartWave(int wave)
{
    if (m_isWaveOn) return;
    const int numberOfEnemiesToSpawn = GetNumEnemiesToSpawn(wave);
    if (wave != 0 && wave % m_areaIncrement == 0)
    {
        m_currentArea++;
        // hard coded, don't forget to remove or talk with Francis about it
        if (m_currentArea > 5) m_currentArea = 5;
    }

    const auto indicatorView = bee::Engine.ECS().Registry.view<WaveIndicator>();

    for (const auto entity : indicatorView)
    {
        bee::Engine.ECS().DeleteEntity(entity);
    }

    SpawnEnemies(numberOfEnemiesToSpawn);
}

void WaveSystem::Update(float dt)
{
    System::Update(dt);
    if (!active) return;
    const auto enemyUnitsView = bee::Engine.ECS().Registry.view<AttributesComponent, EnemyUnit>();
    int enemyCount = 0;
    enemyUnitsView.each([&enemyCount](auto, auto&) { enemyCount++; });
    m_isWaveOn = enemyCount != 0;
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();

    auto& enemiesLine1 = UI.getComponentItem<bee::ui::Text>(m_element, "Enemies1");
    auto& enemiesLine2 = UI.getComponentItem<bee::ui::Text>(m_element, "Enemies2");
    if (!m_isWaveOn)
    {
        m_waveTimer -= dt;
        if (m_waveTimer <= 0.0f)
        {
            bee::Engine.Audio().PlaySoundW("audio/horn_alert.wav", 1.3f, true);
            m_currentWave++;
            StartWave(m_currentWave);

            auto& item = UI.getComponentItem<bee::ui::Text>(m_element, "Wave");
            UI.ReplaceString(item.ID, "Wave: " + to_string(m_currentWave));

            m_waveTimer = m_waveData.delayBetweenWaves;
            m_spawnPositions.clear();
            return;
        }

        const std::string str1 = std::to_string(static_cast<int>(m_waveTimer));

        if (m_spawnPositions.empty())
        {
            GenerateRandomPositions(GetNumEnemiesToSpawn(m_currentWave));
        }

        enemiesLine1.text = "Wave starts:";
        enemiesLine1.start = glm::vec2(0.1075f, enemiesLine1.start.y);
        enemiesLine1.Remake(enemiesLine1.element);
        enemiesLine2.text = str1;
        enemiesLine2.start = glm::vec2(0.16f, enemiesLine2.start.y);
        enemiesLine2.Remake(enemiesLine2.element);

        if (bee::Engine.ECS().GetSystem<InGameUI>().IsTutorialDone())
        UI.SetDrawStateUIelement(m_nextWave, true);
    }
    else
    {
        UI.SetDrawStateUIelement(m_nextWave, false);
        enemiesLine1.text = "Enemies Left:";
        enemiesLine1.start = glm::vec2(0.105f, enemiesLine1.start.y);
        enemiesLine1.Remake(enemiesLine1.element);
        enemiesLine2.text = to_string(enemyCount);
        enemiesLine2.start = glm::vec2(0.16f, enemiesLine2.start.y);
        enemiesLine2.Remake(enemiesLine2.element);
    }

    const auto circlesView = bee::Engine.ECS().Registry.view<WaveIndicator, bee::Transform>();

    for (const auto circle : circlesView)
    {
        auto& transform = circlesView.get<bee::Transform>(circle);
        auto& indicator = circlesView.get<WaveIndicator>(circle);

        float angle = m_circleRotationDelta * dt;
        glm::quat incrementalRotation = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat newOrientation = incrementalRotation * transform.Rotation;

        const float verticalOffset = m_upDownAmplitude * sin(m_upDownFrequency * indicator.lifeTime) + m_upDownAmplitude;

        indicator.lifeTime += dt;

        transform.Scale.x = verticalOffset;
        transform.Scale.y = verticalOffset;

        newOrientation = glm::normalize(newOrientation);
        transform.Rotation = newOrientation;
    }
}
#ifdef BEE_INSPECTOR
void WaveSystem::Inspect()
{
    ImGui::Begin("Wave system");
    bee::Engine.Inspector().Inspect(m_waveData);
    ImGui::InputInt("current area", &m_currentArea);

    if (ImGui::Button("Start wave", ImVec2(100, 25)))
    {
        StartWave(1);
    }

    ImGui::End();

    for (auto& pos : m_spawnPositions)
    {
        bee::Engine.DebugRenderer().AddCircle(bee::DebugCategory::Gameplay, glm::vec3(pos, 0), 1.0f, glm::vec4(1, 0, 1, 1));
    }
}
#endif

void WaveSystem::CalculatePlayerBaseCenter()
{
    const auto playerBuildingsView = bee::Engine.ECS().Registry.view<AllyStructure, bee::Transform>();
    if (playerBuildingsView.size_hint() == 0) return;
    m_ringCenter = glm::vec2();

    for (const auto entity : playerBuildingsView)
    {
        const auto position = playerBuildingsView.get<bee::Transform>(entity).Translation;
        m_ringCenter += glm::vec2(position.x, position.y);
    }

    m_ringCenter /= playerBuildingsView.size_hint();
}

bee::geometry2d::Polygon WaveSystem::GetOutlinesOfAreas(int areaIndex)
{
    bee::geometry2d::Polygon polygon{};
    auto view = bee::Engine.ECS().Registry.view<lvle::TerrainDataComponent>();
    for (auto entity : view)
    {
        auto [data] = view.get(entity);
        std::vector<glm::vec2> hull;

        for (const auto& tile : data.m_tiles)
        {
            if (tile.area != areaIndex) continue;
            hull.emplace_back(tile.centralPos.x, tile.centralPos.y);
        }

        hull = QuickHull(hull);
        for (const auto vec : hull)
        {
            polygon.push_back(glm::vec2(vec.x, vec.y));
        }
    }
    return polygon;
}

int WaveSystem::GetCurrentWave() const { return m_currentWave; }

void WaveSystem::ClearText()
{
    auto& UI = bee::Engine.ECS().GetSystem<bee::ui::UserInterface>();
    auto enemiesLine1 = UI.GetComponentID(m_element, "Enemies1");
    auto enemiesLine2 = UI.GetComponentID(m_element, "Enemies2");
    UI.ReplaceString(enemiesLine1, "");
    UI.ReplaceString(enemiesLine2, "");

    UI.SetDrawStateUIelement(m_nextWave, false);
}

void WaveSystem::StartWave()
{
    m_waveTimer = 0.0f; 

}

const std::vector<glm::vec2>& WaveSystem::GetEnemySpawnLocation()
{ return m_spawnPositions; }

void WaveSystem::GenerateRandomPositions(int numberOfEnemiesToSpawn)
{
    m_spawnPositions.clear();
    m_numberOfEnemiesPerType.clear();

    const auto indicatorView = bee::Engine.ECS().Registry.view<WaveIndicator>();

    for (const auto entity : indicatorView)
    {
        bee::Engine.ECS().DeleteEntity(entity);
    }

    bee::geometry2d::PolygonList polygons{};
    m_numberOfEnemiesPerType = EnemiesToSpawnPerType(m_waveData.enemyWeights, numberOfEnemiesToSpawn);

    for (int i = 1; i <= m_currentArea; i++)
    {
        auto polygon = GetOutlinesOfAreas(i);
        if (polygon.empty()) continue;
        polygons.push_back(polygon);
    }
    const auto selectionModel = bee::Engine.Resources().Load<bee::Model>("models/SelectionPlane.glb");

    for (size_t i = 0; i < m_numberOfEnemiesPerType.size(); i++)
    {
        for (int j = 0; j < m_numberOfEnemiesPerType[i]; j++)
        {
            if (polygons.empty()) return;
            bee::geometry2d::Polygon hull = polygons[bee::GetRandomNumberInt(0, polygons.size() - 1)];
            auto aabb = bee::geometry2d::GetPolygonBounds(hull);
            glm::vec2 randomPos = glm::vec2(bee::GetRandomNumber(aabb.GetMin().x, aabb.GetMax().x),
                                            bee::GetRandomNumber(aabb.GetMin().y, aabb.GetMax().y));
            m_spawnPositions.push_back(randomPos);

            const auto seletionEntity = bee::Engine.ECS().CreateEntity();
            bee::Engine.ECS().CreateComponent<WaveIndicator>(seletionEntity);
            auto& selectionTransform = bee::Engine.ECS().CreateComponent<bee::Transform>(seletionEntity);
            selectionTransform.Translation = glm::vec3(randomPos, 0.5f);
            glm::vec3 result{};
            bee::Engine.ECS().GetSystem<lvle::TerrainSystem>().FindRayMeshIntersection(selectionTransform.Translation,
                                                                                       glm::vec3(0, 0, -10), result);

            selectionTransform.Translation = result;
            selectionTransform.Translation.z += 0.5f;

            selectionModel->Instantiate(seletionEntity);
            auto& mesh = bee::GetComponentInChildren<bee::MeshRenderer>(seletionEntity);
            mesh.Material = m_indicatorMaterial;
        }
    }
}

void WaveSystem::SpawnEnemies(int numberOfEnemiesToSpawn)
{
    CalculatePlayerBaseCenter();

    int index = 0;
    int actualSpawnedEnemies = 0;
    for (int i = 0; i < m_numberOfEnemiesPerType.size(); i++)
    {
        for (int j = 0; j < m_numberOfEnemiesPerType[i]; j++)
        {
            // int random = bee::GetRandomNumberInt(0, totalWeight);
            glm::vec3 randomOffset = glm::vec3(bee::GetRandomNumber(0, 1), bee::GetRandomNumber(0, 1), 0);
            auto handle = m_waveData.enemiesToSpawn[i];
            bee::Engine.ECS().GetSystem<UnitManager>().SpawnUnit(
                handle,
                glm::vec3(bee::Engine.ECS().GetSystem<bee::ai::GridNavigationSystem>().GetGrid().SampleWalkablePoint(
                    m_spawnPositions[index]),
                    0) +
                randomOffset,
                Team::Enemy);
            actualSpawnedEnemies++;
            index++;
        }
    }
}

glm::vec2 WaveSystem::GetPointOnARing(glm::vec2 ringCenter, float ringRadius, float ringThickness) const
{
    // Generate a random angle within [0, 2*pi]
    const float randomAngle = bee::GetRandomNumber(0.0f, glm::two_pi<float>());

    // Generate a random distance between the inner and outer radius of the ring
    const float randomDistance = bee::GetRandomNumber(ringRadius, ringRadius + ringThickness);

    const float x = ringCenter.x + randomDistance * cos(randomAngle);
    const float y = ringCenter.y + randomDistance * sin(randomAngle);

    return {x, y};
}

int WaveSystem::GetNumEnemiesToSpawn(int wave) const
{
    const size_t vectorSize = m_waveData.numberOfEnemiesPerWave.size();
    if (vectorSize <= wave)
    {
        return m_waveData.numberOfEnemiesPerWave[vectorSize - 1] + wave;
    }
    else
    {
        return m_waveData.numberOfEnemiesPerWave[wave];
    }
}

std::vector<int> WaveSystem::EnemiesToSpawnPerType(const std::vector<int>& weights, int totalNumberOfEnemiesToSpawn)
{
    // 1) Sum of weights
    int sumWeights = 0;
    for (int weight : weights)
    {
        sumWeights += weight;
    }

    // 2) Calculate scaling factor
    float scalingFactor = static_cast<float>(totalNumberOfEnemiesToSpawn) / sumWeights;

    // 3) Calculate number of enemies per type
    std::vector<int> result;
    int accumulatedSum = 0;
    for (size_t i = 0; i < weights.size(); ++i)
    {
        int scaledValue = std::round(weights[i] * scalingFactor);
        result.push_back(scaledValue);
        accumulatedSum += scaledValue;
    }

    return result;
}
