#pragma once
#include "core/ecs.hpp"
#include "rendering/model.hpp"

//#include "platform/dx12/mesh_dx12.h"
namespace bee
{
struct ParticleProps;
class Mesh;

class ParticleEmitter
{
public:
    ParticleEmitter();
    ~ParticleEmitter();
    void SaveAsTemplate();
    /// <summary>
    /// Tell this emitter to emit only one particle.
    /// </summary>
    void Emit() const;

    /// <summary>
    /// Tell this emitter to emit multiple particles.
    /// </summary>
    /// <param name="amount">"Amount of particles you wish to emit."</param>
    void Emit(const int amount) const;

    void SetLoop(bool toggle) { m_isLoop = toggle; }

    /// <summary>
    /// Assign an entt::entity to this emitter. Useful for managing the emitter in the scene, allowing different components for this emitter.
    /// </summary>
    void AssignEntity(Entity entity);


    void ReloadModels();

    std::shared_ptr<ParticleProps> particleProps = nullptr;
    //ParticleProps* particleProps = nullptr;

    template <class Archive>
        void serialize(Archive& archive)
        {
                archive(CEREAL_NVP(particleProps), CEREAL_NVP(m_isLoop), CEREAL_NVP(m_EmitAmount), CEREAL_NVP(m_emitRate), CEREAL_NVP(m_particleModelPath), CEREAL_NVP(m_materialPath));
        }
private:

    std::shared_ptr < Mesh> m_interMesh;
    std::shared_ptr < Model> m_particleModel;

    std::shared_ptr<Material> m_particleMaterial;
    
    std::string m_particleModelPath = "";
    
    std::string m_materialPath = "";
    

    Entity m_Entity = entt::null;
    bool m_isLoop = false;
    
    int m_EmitAmount = 1; //particles to emit per burst
    float m_emitRate = 1.0f; //particles per second
    float m_timeAccumulator = 0.0f; // time since last emission

    friend class ParticleSystem;
    friend class Inspector;
};
}  // namespace bee