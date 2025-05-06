#include "animation/animation_state.hpp"

#include <platform/dx12/animation_system.hpp>
#include <platform/dx12/skeletal_animation.hpp>
#include <platform/dx12/skeleton.hpp>
#include "rendering/model.hpp"
#include "tools/tools.hpp"

AnimationState::AnimationState() : speed(1)
{

}

void AnimationState::LoadAnimations()
{
    if (modelPath.empty()) return;

    auto tempString = modelPath.string();
    bee::RemoveSubstring(tempString,"assets/");
    modelPath = tempString;

    if (m_model == nullptr && !modelPath.empty())
    {
        m_model = bee::Engine.Resources().Load<bee::Model>(modelPath.string());
    }

    if (m_animation == nullptr && !animationName.empty())
    {
        m_animation = bee::Engine.Resources().Find<bee::SkeletalAnimation>(std::hash<std::string>()(modelPath.string() + "/" + animationName));
    }
}

void AnimationState::Initialize(bee::ai::StateMachineContext& context)
{
    State::Initialize(context);

    context.blackboard->SetData("Ended", false);

    if (!context.blackboard->HasKey<float>("CurrentFrameSecond"))
    {
        context.blackboard->SetData("CurrentFrameSecond", 1.0f);
    }

    context.blackboard->SetData("Ended", false);
    context.blackboard->SetData("CurrentFrame", 1.0f);
    context.blackboard->SetData("Alpha", 1.0f);
}

void AnimationState::Update(bee::ai::StateMachineContext& context)
{
    if (m_animation == nullptr) return;
    auto& m_alpha = context.blackboard->GetData<float>("Alpha");

    auto& currentFrame = context.blackboard->GetData<float>("CurrentFrame");
    auto& currentFrameSecond = context.blackboard->GetData<float>("CurrentFrameSecond");
    auto skeleton = context.blackboard->GetData<std::shared_ptr<bee::Skeleton>>("Skeleton");

    std::shared_ptr<bee::SkeletalAnimation> animationSecond; 
    if (context.blackboard->HasKey<std::shared_ptr<bee::SkeletalAnimation>>("SecondAnimation") &&
        context.blackboard->GetData<std::shared_ptr<bee::SkeletalAnimation>>("SecondAnimation")!=nullptr)
    {
        animationSecond = context.blackboard->GetData<std::shared_ptr<bee::SkeletalAnimation>>("SecondAnimation");
       
    }
    else
    {
        animationSecond = m_animation;
    }


    if (skeleton == nullptr)
    {
        return;
    }

    m_alpha -= context.deltaTime*3;
    m_alpha = std::clamp(m_alpha, 0.0f, 1.0f);

    currentFrame += context.deltaTime * speed;
    currentFrameSecond += context.deltaTime * speed;

    if (repeat && (currentFrame > m_animation->GetLastKeyframe()))
    {
        currentFrame = m_animation->GeFirstKeyframe() + currentFrame - m_animation->GetLastKeyframe();
    }
    else if (!repeat && (currentFrame > m_animation->GetLastKeyframe()))
    {
        context.blackboard->SetData("Ended", true);
    }

    if (repeat && (currentFrameSecond > animationSecond->GetLastKeyframe()))
    {
        currentFrameSecond = animationSecond->GeFirstKeyframe() + currentFrameSecond - animationSecond->GetLastKeyframe();
    }

    for (int c = 0; c < m_animation->m_Channels.size(); c++)
    {
        auto& channel = m_animation->m_Channels[c];
        auto& sampler = m_animation->m_Samplers[channel.m_SamplerIndex];
        int jointIndex = skeleton->m_NodeToJointIndex[channel.m_Node];
        auto& joint = skeleton->m_Joints[jointIndex];

        auto& channel_second = animationSecond->m_Channels[c];
        auto& sampler_second = animationSecond->m_Samplers[channel_second.m_SamplerIndex];

        float first = 0;
        float second = 0;

        size_t stamp = 0;

        for (size_t i = 0; i < sampler.m_TimeStamps.size() - 1; i++)
        {
            first = sampler.m_TimeStamps[i];
            second = sampler.m_TimeStamps[i + 1];
            stamp = i;
            if (currentFrame >= first && currentFrame <= second)
            {
                break;
            }
        }

        float first_2 = 0;
        float second_2 = 0;
        size_t stamp_2 = 0;
        for (size_t j = 0; j < sampler_second.m_TimeStamps.size() - 1; j++)
        {
            first_2 = sampler_second.m_TimeStamps[j];
            second_2 = sampler_second.m_TimeStamps[j + 1];
            stamp_2 = j;
            if (currentFrameSecond >= first_2 && currentFrameSecond <= second_2)
            {
                break;
            }
        }

        switch (sampler.m_Interpolation)
        {
            case bee::SkeletalAnimation::Interpolation::Linear:
            {
                float alpha = (currentFrame - first) / (second - first);

                float alpha_2 = (currentFrameSecond - first_2) / (second_2 - first_2);

                switch (channel.m_Path)
                {
                    case bee::SkeletalAnimation::Path::Translation:
                    {
                        DirectX::XMVECTOR k1 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp]);
                        DirectX::XMVECTOR k2 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp + 1]);

                        DirectX::XMVECTOR translation1 = DirectX::XMVectorLerp(k1, k2, alpha);

                        DirectX::XMVECTOR k1_2 = DirectX::XMLoadFloat4(&sampler_second.m_InterpolatedData[stamp_2]);
                        DirectX::XMVECTOR k2_2 = DirectX::XMLoadFloat4(&sampler_second.m_InterpolatedData[stamp_2 + 1]);

                        DirectX::XMVECTOR translation2 = DirectX::XMVectorLerp(k1_2, k2_2, alpha_2);

                        DirectX::XMVECTOR translation = DirectX::XMVectorLerp(translation1, translation2, m_alpha);

                        DirectX::XMStoreFloat4(&joint.m_Translation, translation);

                        break;
                    }
                    case bee::SkeletalAnimation::Path::Rotation:
                    {
                        DirectX::XMVECTOR k1 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp]);
                        DirectX::XMVECTOR k2 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp + 1]);

                        DirectX::XMVECTOR rotation1 = DirectX::XMQuaternionSlerp(k1, k2, alpha);

                        DirectX::XMVECTOR k1_2 = DirectX::XMLoadFloat4(&sampler_second.m_InterpolatedData[stamp_2]);
                        DirectX::XMVECTOR k2_2 = DirectX::XMLoadFloat4(&sampler_second.m_InterpolatedData[stamp_2 + 1]);

                        DirectX::XMVECTOR rotation2 = DirectX::XMQuaternionSlerp(k1_2, k2_2, alpha_2);

                        DirectX::XMVECTOR rotation = DirectX::XMQuaternionSlerp(rotation1, rotation2, m_alpha);

                        DirectX::XMStoreFloat4(&joint.m_Rotation, rotation);

                        break;
                    }
                    case bee::SkeletalAnimation::Path::Scale:
                    {
                        DirectX::XMVECTOR k1 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp]);
                        DirectX::XMVECTOR k2 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp + 1]);

                        DirectX::XMVECTOR scale1 = DirectX::XMVectorLerp(k1, k2, alpha);

                        DirectX::XMVECTOR k1_2 = DirectX::XMLoadFloat4(&sampler_second.m_InterpolatedData[stamp_2]);
                        DirectX::XMVECTOR k2_2 = DirectX::XMLoadFloat4(&sampler_second.m_InterpolatedData[stamp_2 + 1]);

                        DirectX::XMVECTOR scale2 = DirectX::XMVectorLerp(k1_2, k2_2, alpha_2);

                        DirectX::XMVECTOR scale = DirectX::XMVectorLerp(scale1, scale2, m_alpha);

                        DirectX::XMStoreFloat4(&joint.m_Scale, scale);

                        break;
                    }

                    default:
                        break;
                }

                break;
            }

            case bee::SkeletalAnimation::Interpolation::Step:
            {
                switch (channel.m_Path)
                {
                    case bee::SkeletalAnimation::Path::Translation:
                    {
                        DirectX::XMVECTOR k1 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp]);

                        DirectX::XMStoreFloat4(&joint.m_Translation, k1);

                        break;
                    }
                    case bee::SkeletalAnimation::Path::Rotation:
                    {
                        DirectX::XMVECTOR k1 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp]);

                        DirectX::XMVECTOR rotationQuaternion =
                            DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), DirectX::XM_PIDIV2);

                        DirectX::XMVECTOR correctedRotation = DirectX::XMQuaternionMultiply(rotationQuaternion, k1);

                        DirectX::XMStoreFloat4(&joint.m_Rotation, correctedRotation);

                        break;
                    }
                    case bee::SkeletalAnimation::Path::Scale:
                    {
                        DirectX::XMVECTOR k1 = DirectX::XMLoadFloat4(&sampler.m_InterpolatedData[stamp]);

                        DirectX::XMStoreFloat4(&joint.m_Scale, k1);

                        break;
                    }

                    default:
                        break;
                }

                break;
            }
            case bee::SkeletalAnimation::Interpolation::CubicSpline:
            {
                break;
            }
            default:
                break;
        }
    }
}

void AnimationState::End(bee::ai::StateMachineContext& context)
{
    State::End(context);
    const auto currentFrame = context.blackboard->GetData<float>("CurrentFrame");

    context.blackboard->SetData<float>("Alpha", 1.0f);
    context.blackboard->SetData<float>("CurrentFrameSecond",currentFrame);
    context.blackboard->SetData("SecondAnimation", m_animation);
}
