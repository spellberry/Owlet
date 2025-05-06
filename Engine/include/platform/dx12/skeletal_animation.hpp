#pragma once
#include <DirectXMath.h>
#include <vector>
#include <string>
#include "core/resource.hpp"

namespace bee
{
class Model;
struct Skeleton;
class SkeletalAnimation : public bee::Resource
{
public:
    enum class Path
    {
        Translation,
        Rotation,
        Scale

    };
    enum class Interpolation
    {
        Linear,
        Step,
        CubicSpline

    };

    struct Channel
    {
        Path m_Path;
        int m_SamplerIndex;
        int m_Node;
    };

    struct Sampler
    {
        std::vector<float> m_TimeStamps;
        std::vector<DirectX::XMFLOAT4> m_InterpolatedData;

        Interpolation m_Interpolation;
    };


    SkeletalAnimation(const Model& model, int index, const bee::Skeleton& skeleton);

 // void Start();
 // void Stop();
 //   bool IsRunning();

 //   void SetRepeat(bool repeat);

 //   void Update(float deltaTime, Skeleton& skeleton);

    float GetLastKeyframe() 
    {
        return m_LastKeyFrame;
    }
    float GeFirstKeyframe() 
    { 
        return m_FirstKeyFrame;
    }
    std::string GetName()
    { 
        return m_Name;
    }

    std::vector<Sampler> m_Samplers;
    std::vector<Channel> m_Channels;

private:
    std::string m_Name;

 // bool b_Repeat;

    float m_FirstKeyFrame=0;
    float m_LastKeyFrame=0;

 // float m_CurrentKeyFrame = 0.0f;



};

}  // namespace bee
