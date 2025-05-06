#pragma once
#include "core/ecs.hpp"
#include "core/resource.hpp"
#include <string>
namespace bee
{
class SkeletalAnimation;
class AnimationSystem : public System
{

public:
    AnimationSystem();
    ~AnimationSystem();

    void AddAnimation(std::shared_ptr<SkeletalAnimation> animation);

    std::shared_ptr<bee::SkeletalAnimation> GetAnimation(int index);

    std::shared_ptr<bee::SkeletalAnimation> GetAnimation(std::string name);

    void Update(float dt) override;
#ifdef BEE_INSPECTOR
    void Inspect(Entity e) override;
#endif


    std::vector<std::shared_ptr<SkeletalAnimation>> m_animations;
};

}  // namespace bee