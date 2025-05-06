#include "core/ecs.hpp"
#include "glm/glm.hpp"
#include <imgui/imgui.h>

#include "particle_emitter.hpp"
#include "platform/dx12/RenderPipeline.hpp"
#include "tools/serialize_imgui.hpp"
//#include "platform/dx12/mesh_dx12.h"
namespace bee
{
class ParticleEmitter;
struct Transform;

//for emitters

enum class ParticleType
{
    Billboard,
    Mesh,
};


struct ParticleProps
{
    glm::vec3 position;
    glm::vec3 velocity, velocityVariation;
    glm::vec3 sizeBegin = glm::vec3(1.0);
    glm::vec4 colorBegin, colorEnd;
    glm::vec3 sizeVariation = glm::vec3(1.0f);
    float lifeTime = 1.0f;
    float gravity = 9.8f;
    bool hasGravity = false;
    ParticleType type = ParticleType::Billboard;

    //for curves
    ImVec2 sizeCurvePoints[10];

    template <class Archive>
        void serialize(Archive& archive)
        {
            archive(CEREAL_NVP(position), CEREAL_NVP(velocity), CEREAL_NVP(velocityVariation),CEREAL_NVP(sizeBegin), CEREAL_NVP(colorBegin), CEREAL_NVP(colorEnd),
                    CEREAL_NVP(sizeVariation), CEREAL_NVP(lifeTime), CEREAL_NVP(gravity), CEREAL_NVP(hasGravity), CEREAL_NVP(type),CEREAL_NVP(sizeCurvePoints));
        }
    
};

//for particles


struct ParticleComponent
{
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec4 colorBegin, colorEnd;
    glm::vec3 rotation = glm::vec3(0.0f);
    float lifeTime = 1.0f;
    float lifeRemaining = 0.0f;
    float gravity = 9.8f;
    glm::vec3 sizeVariation = glm::vec3(1.0f);
    ParticleType type = ParticleType::Billboard;


    //for curves
    ImVec2 sizeCurvePoints[10];
    glm::vec3 emitterInitialSize = glm::vec3(1.0);
    Entity spawnEmitter = entt::null;
};

class ParticleSystem : public System
{
public:
    ParticleSystem();
    void Run(float deltaTime);

    /// <summary>
    /// Spawn one emitter in the particle system.
    /// </summary>
    void SpawnEmitter();
    void SpawnEmitter(const Transform& transform,const ParticleEmitter& emitterComponent);
    
    /// <summary>
    /// Update all the particles based on delta time.
    /// </summary>
    /// <param name="deltaTime">"Delta Time</param>
    void Update(float deltaTime);
    
    /// <summary>
    /// Show UI for Particle System.
    /// </summary>
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif

    /// <summary>
    /// Save all emitters in current scene
    /// </summary>
    void SaveEmitters(std::string& fileName);

    void SaveEmitterAsTemplate(const ParticleEmitter& emitter, const std::string& templateName);

    /// <summary>
    /// Load all emitters from a file
    /// </summary>
    void LoadEmitters(std::string& fileName);

    void LoadEmitterFromTemplate(ParticleEmitter& emitter, const std::string& filename);

    
   // SimpleMeshRender* particleMesh = nullptr;
private:

      

    ParticleProps* m_props = nullptr;
    float m_accumulator = 0.0f;

    friend class ParticleEmitter;

};
}