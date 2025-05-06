#include <iostream>
#include <platform/dx12/skeletal_animation.hpp>
#include <platform/dx12/skeleton.hpp>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "rendering/model.hpp"
//#include "platform/dx12/Helpers.hpp"
using namespace bee;

void DecomposeMatrixDx(const DirectX::XMFLOAT4X4& matrix, DirectX::XMVECTOR& translation, DirectX::XMVECTOR& rotation,
                       DirectX::XMVECTOR& scale)
{
    // Load the matrix
    DirectX::XMMATRIX M = DirectX::XMLoadFloat4x4(&matrix);

    // Decompose the matrix
    DirectX::XMMatrixDecompose(&scale, &rotation, &translation, M);
}

SkeletalAnimation::SkeletalAnimation(const Model& model, int index, const bee::Skeleton& skeleton)
    : bee::Resource(ResourceType::Animation)
{
    const auto& document = model.GetDocument();
    auto& animation = document.animations[index];

    m_Name = animation.name;
    m_path = model.GetPath() + "/" + animation.name;

    size_t sampler_num = animation.samplers.size();
    m_Samplers.resize(sampler_num);

    for (int i = 0; i < sampler_num; ++i)
    {
        tinygltf::AnimationSampler GltfSampler = animation.samplers[i];
        auto& sampler = m_Samplers[i];

        sampler.m_Interpolation = SkeletalAnimation::Interpolation::Linear;
        if (GltfSampler.interpolation == "STEP")
        {
            sampler.m_Interpolation = SkeletalAnimation::Interpolation::Step;
        }
        else if (GltfSampler.interpolation == "CUBICSPLINE")
        {
            sampler.m_Interpolation = SkeletalAnimation::Interpolation::CubicSpline;
        }

        {
            unsigned int count = 0;
            const float* timeStampBuffer;

            model.LoadAccessor<float>(document.accessors[GltfSampler.input], timeStampBuffer, &count);

            sampler.m_TimeStamps.resize(count);
            for (size_t index = 0; index < count; ++index)
            {
                sampler.m_TimeStamps[index] = timeStampBuffer[index];
            }
        }

        {
            unsigned int count = 0;
            int type;
            const unsigned int* buffer;

            model.LoadAccessor<unsigned int>(document.accessors[GltfSampler.output], buffer, &count, &type);

            switch (type)
            {
                case TINYGLTF_TYPE_VEC3:
                {
                    const DirectX::XMFLOAT3* oBuffer = reinterpret_cast<const DirectX::XMFLOAT3*>(buffer);
                    sampler.m_InterpolatedData.resize(count);

                    for (size_t index = 0; index < count; ++index)
                    {
                        sampler.m_InterpolatedData[index] =
                            DirectX::XMFLOAT4(oBuffer[index].x, oBuffer[index].y, oBuffer[index].z, 0.0f);
                    }
                    break;
                }
                case TINYGLTF_TYPE_VEC4:
                {
                    const DirectX::XMFLOAT4* oBuffer = reinterpret_cast<const DirectX::XMFLOAT4*>(buffer);
                    sampler.m_InterpolatedData.resize(count);

                    for (size_t index = 0; index < count; ++index)
                    {
                        sampler.m_InterpolatedData[index] = oBuffer[index];
                    }
                    break;
                }
                default:
                {
                }
            }
        }
    }
    if (m_Samplers.size())
    {
        auto& sampler = m_Samplers[0];
        m_FirstKeyFrame = sampler.m_TimeStamps[0];
        m_LastKeyFrame = sampler.m_TimeStamps.back();
    }

    size_t num_channels = animation.channels.size();
    size_t num_joints = skeleton.m_Joints.size();

    // m_Channels.resize(num_channels);
    m_Channels.resize(num_joints * 3);
    // std::unordered_set<int> existingNodes;
    /*{
        bee::SkeletalAnimation::Sampler defaultSampler;
        defaultSampler.m_Interpolation = SkeletalAnimation::Interpolation::Linear;
        defaultSampler.m_TimeStamps = {m_FirstKeyFrame, m_LastKeyFrame};

        

        defaultSampler.m_InterpolatedData = {DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
                                             DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)};
        
        m_Samplers.push_back(defaultSampler);
    }
    {
    
    bee::SkeletalAnimation::Sampler defaultSampler;
    defaultSampler.m_Interpolation = SkeletalAnimation::Interpolation::Linear;
    defaultSampler.m_TimeStamps = {m_FirstKeyFrame, m_LastKeyFrame};
    defaultSampler.m_InterpolatedData = {DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)};

    m_Samplers.push_back(defaultSampler);

    }
    {
        bee::SkeletalAnimation::Sampler defaultSampler;
        defaultSampler.m_Interpolation = SkeletalAnimation::Interpolation::Linear;
        defaultSampler.m_TimeStamps = {m_FirstKeyFrame, m_LastKeyFrame};
        defaultSampler.m_InterpolatedData = {DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
                                             DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)};

        m_Samplers.push_back(defaultSampler);
    }*/


    int c = 0;

    for (size_t i = 0; i < num_joints; ++i)
    {
       bool try_channel;
        tinygltf::AnimationChannel gltfChannel;


        if (c < num_channels)
       {
            gltfChannel = animation.channels[c];
           try_channel = true;
       }
       else
           try_channel = false;


          if (try_channel && gltfChannel.target_node == skeleton.m_Joints[i].m_GltfNodeIndex &&
           gltfChannel.target_path == "translation")
          {
               Channel& channel = m_Channels[i*3];
              channel.m_SamplerIndex = gltfChannel.sampler;
              channel.m_Node = gltfChannel.target_node;
              channel.m_Path = Path::Translation;
              c++;
          }
          else
          {

              
              DirectX::XMVECTOR translation;
              DirectX::XMVECTOR rotation;
              DirectX::XMVECTOR scale;

              DirectX::XMMATRIX mt =
                  DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&skeleton.m_Joints[i].m_DefaultNodeMatrix));
              DirectX::XMFLOAT4X4 m;

              DirectX::XMStoreFloat4x4(&m, mt);
              DecomposeMatrixDx(m, translation, rotation, scale);
              DirectX::XMFLOAT4 t;
              DirectX::XMStoreFloat4(&t, translation);
              {
                  bee::SkeletalAnimation::Sampler defaultSampler;
                  defaultSampler.m_Interpolation = SkeletalAnimation::Interpolation::Linear;
                  defaultSampler.m_TimeStamps = {m_FirstKeyFrame, m_LastKeyFrame};

                  defaultSampler.m_InterpolatedData = {t,
                                                       t};

                  m_Samplers.push_back(defaultSampler);
              }


              Channel& channel = m_Channels[i * 3];
              channel.m_SamplerIndex = m_Samplers.size()-1;
              channel.m_Node = skeleton.m_Joints[i].m_GltfNodeIndex;
              channel.m_Path = Path::Translation;
          }

             
          
        if (c < num_channels)
          {
              gltfChannel = animation.channels[c];
              try_channel = true;
          }
          else
              try_channel = false;

          if (try_channel && gltfChannel.target_node == skeleton.m_Joints[i].m_GltfNodeIndex &&
              gltfChannel.target_path == "rotation")
          {
              Channel& channel = m_Channels[i * 3+1];
              channel.m_SamplerIndex = gltfChannel.sampler;
              channel.m_Node = gltfChannel.target_node;
              channel.m_Path = Path::Rotation;
              c++;
             
          }
          else
          {

               DirectX::XMVECTOR translation;
              DirectX::XMVECTOR rotation;
              DirectX::XMVECTOR scale;

              DirectX::XMMATRIX mt =
                  DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&skeleton.m_Joints[i].m_DefaultNodeMatrix));
              DirectX::XMFLOAT4X4 m;

              DirectX::XMStoreFloat4x4(&m, mt);
              DecomposeMatrixDx(m, translation, rotation, scale);
              DirectX::XMFLOAT4 r;
              DirectX::XMStoreFloat4(&r, rotation);
              {
                  bee::SkeletalAnimation::Sampler defaultSampler;
                  defaultSampler.m_Interpolation = SkeletalAnimation::Interpolation::Linear;
                  defaultSampler.m_TimeStamps = {m_FirstKeyFrame, m_LastKeyFrame};

                  defaultSampler.m_InterpolatedData = {r, r};

                  m_Samplers.push_back(defaultSampler);
              }

              Channel& channel = m_Channels[i * 3+1];
              channel.m_SamplerIndex = m_Samplers.size()-1;
              channel.m_Node = skeleton.m_Joints[i].m_GltfNodeIndex;
              channel.m_Path = Path::Rotation;

          }


      
        if (c < num_channels)
          {
              gltfChannel = animation.channels[c];
              try_channel = true;
          }
          else
              try_channel = false;

          if (try_channel && gltfChannel.target_node == skeleton.m_Joints[i].m_GltfNodeIndex &&
              gltfChannel.target_path == "scale")
          {
              Channel& channel = m_Channels[i * 3+2];
              channel.m_SamplerIndex = gltfChannel.sampler;
              channel.m_Node = gltfChannel.target_node;
              channel.m_Path = Path::Scale;
            
              c++;
             
          }
          else
          {
              DirectX::XMVECTOR translation;
              DirectX::XMVECTOR rotation;
              DirectX::XMVECTOR scale;

              DirectX::XMMATRIX mt =
                  DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&skeleton.m_Joints[i].m_DefaultNodeMatrix));
              DirectX::XMFLOAT4X4 m;

              DirectX::XMStoreFloat4x4(&m, mt);
              DecomposeMatrixDx(m, translation, rotation, scale);
              DirectX::XMFLOAT4 s;
              DirectX::XMStoreFloat4(&s, scale);
              {
                  bee::SkeletalAnimation::Sampler defaultSampler;
                  defaultSampler.m_Interpolation = SkeletalAnimation::Interpolation::Linear;
                  defaultSampler.m_TimeStamps = {m_FirstKeyFrame, m_LastKeyFrame};

                  defaultSampler.m_InterpolatedData = {s, s};

                  m_Samplers.push_back(defaultSampler);
              }


              Channel& channel = m_Channels[i * 3 + 2];
              channel.m_SamplerIndex = m_Samplers.size()-1;
              channel.m_Node = skeleton.m_Joints[i].m_GltfNodeIndex;
              channel.m_Path = Path::Scale;
          }
      
    }

}
