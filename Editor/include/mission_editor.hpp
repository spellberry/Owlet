#pragma once
#include "core/game_base.hpp"


class MissionEditor : public bee::GameBase
{
public:
    void Init() override;
    void ShutDown() override;
    void Update(float dt) override;

private:
};