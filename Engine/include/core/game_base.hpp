#pragma once
#include <string>

namespace bee
{
class GameBase
{
public:
    virtual ~GameBase() = default;
    GameBase() : gameName("Game") {}

    virtual void Init() {}
    virtual void Update(float dt) {}
    virtual void Render() {}
    virtual void ShutDown() {}
    virtual void Pause(bool pause){}

protected:
    std::string gameName;
};
}  // namespace bee