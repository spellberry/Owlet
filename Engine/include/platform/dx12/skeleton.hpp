#pragma once

#include <DirectXMath.h>

#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "core/resource.hpp"
#include <iostream>
namespace bee
{
class Model;
struct Joint
{
    int m_GltfNodeIndex;

    std::string m_Name;

    DirectX::XMFLOAT4X4 m_DefaultNodeMatrix;

    DirectX::XMFLOAT4X4 m_InverseBindMatrix;

    DirectX::XMFLOAT4 m_Translation;

    DirectX::XMFLOAT4 m_Rotation= {0.0f, 0.0f, 0.0f, 1.0f};  // 

    DirectX::XMFLOAT4 m_Scale = {1,1,1,1};

    int m_ParentIndex;

    std::vector<int> m_Children;

    DirectX::XMFLOAT4X4 GetDeformedMatrix() 
    { 
        DirectX::XMFLOAT4X4 mat;

        DirectX::XMStoreFloat4x4(&mat, 
      
         DirectX::XMMatrixTranspose(

             DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat4(&m_Scale))*
            DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&m_Rotation)) *
             DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&m_Translation))
        )
              * DirectX::XMLoadFloat4x4(&m_DefaultNodeMatrix)

            );

     
        return mat;
    }
    DirectX::XMFLOAT4X4 GetDeformedMatrixOk()
    {
        DirectX::XMFLOAT4X4 mat;

        DirectX::XMStoreFloat4x4(&mat,
            DirectX::XMMatrixTranspose(
                                     DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat4(&m_Scale)) *
                                     DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&m_Rotation)) *

                                     DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&m_Translation)) 
            )
        );

     
        return mat;
    }
   
};

struct Skeleton
{
public:
    void Update();
    void UpdateJoint(int jointIndex);

    Skeleton(const Model& model, int index);

    //   ~Skeleton() override;

    std::vector<Joint> m_Joints;

    void SetJoint(const Model& model, int gltfNodeIndex, int parent);

    int GiveNameGetIndex(std::string name);  // stfu I like this name 

    DirectX::XMFLOAT4X4 GiveIndexGetMatrix(int index);
    glm::mat4 GiveIndexGetMatrixGLM(int index);

    std::string m_Name;

    bool b_Animated = true;

    std::vector<DirectX::XMFLOAT4X4> m_ComputedJointMatrices;
    std::map<int, int> m_NodeToJointIndex;
};

}  // namespace bee
