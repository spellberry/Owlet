#pragma once
#include "core/ecs.hpp"


namespace bee
{
struct Transform;
struct Light;
class LightSystem : public System
{
public:
    LightSystem();
#ifdef BEE_INSPECTOR
    void Inspect() override;
#endif

    void AddLight();

    void SaveLights(const std::string& fileName);
    void LoadLights(const std::string& fileName);

    entt::view<entt::get_t< Transform, Light>>ReturnRelevantLights();
    
};

} // namespace bee