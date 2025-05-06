#include <platform/dx12/skeleton.hpp>
#include <iostream>

#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "rendering/model.hpp"

using namespace bee;

void Skeleton::SetJoint(const Model& model, int gltfNodeIndex, int parent)
{
    const auto& document = model.GetDocument();

    int jointIndex = m_NodeToJointIndex[gltfNodeIndex];
    auto& joint = m_Joints[jointIndex];

    joint.m_ParentIndex = parent;

    size_t children_num = document.nodes[gltfNodeIndex].children.size();

    if (children_num > 0)
    {
        joint.m_Children.resize(children_num);

        for (size_t i = 0; i < children_num; ++i)
        {
            unsigned int childGltfNodeIndex = document.nodes[gltfNodeIndex].children[i];
            joint.m_Children[i] = m_NodeToJointIndex[childGltfNodeIndex];
            SetJoint(model, childGltfNodeIndex, jointIndex);
        }
    }
}

Skeleton::Skeleton(const Model& model, int index)
{
    const auto& document = model.GetDocument();
    auto skin = document.skins[index];

    m_Name = skin.name;

    if (skin.inverseBindMatrices > -1)
    {
        size_t num_joints = skin.joints.size();

        m_Joints.resize(num_joints);
        m_ComputedJointMatrices.resize(num_joints);

        std::vector<DirectX::XMFLOAT4X4> inverseBindMatrices;
        {
            const tinygltf::Accessor& accessor = document.accessors[skin.inverseBindMatrices];
            const tinygltf::BufferView& bufferView = document.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = document.buffers[bufferView.buffer];

            const auto* matrixStart =
                reinterpret_cast<const DirectX::XMMATRIX*>(&(buffer.data[accessor.byteOffset + bufferView.byteOffset]));

            size_t matrixCount = accessor.count;

            inverseBindMatrices.clear();
            inverseBindMatrices.reserve(matrixCount);

            for (size_t i = 0; i < matrixCount; ++i)
            {
                DirectX::XMFLOAT4X4 matrix;
                DirectX::XMStoreFloat4x4(&matrix, matrixStart[i]);
                inverseBindMatrices.push_back(matrix);
            }
        }

       /* const DirectX::XMMATRIX* inverseBindMatrices;
        {
            unsigned int count = 0;
            int type;
            auto componentType = model.LoadAccessor<DirectX::XMMATRIX>(document.accessors[skin.inverseBindMatrices],
                                                                       inverseBindMatrices, &count, &type);
            std::cout << count;
        }*/


        for (int i = 0; i < num_joints; i++)
        {
            int m_GltfNodeIndex = skin.joints[i];

            auto& joint = m_Joints[i];

            const auto& node = document.nodes[m_GltfNodeIndex];

            joint.m_GltfNodeIndex = m_GltfNodeIndex;
            joint.m_Name = node.name;

           DirectX::XMMATRIX inverseBindMatrix = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&inverseBindMatrices[i]));
           DirectX::XMStoreFloat4x4(&joint.m_InverseBindMatrix, inverseBindMatrix);
         
            if (node.translation.size() == 3)
            {
                DirectX::XMFLOAT4 directXTranslation(node.translation[0], node.translation[1], node.translation[2],1);

                joint.m_Translation = directXTranslation;
            }

            if (node.rotation.size() == 4)
            {
               
  
                DirectX::XMFLOAT4 directXRotation(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);

              //  DirectX::XMFLOAT4 directXRotation(0, 0, 0.7071, 0.7071);

                joint.m_Rotation = directXRotation;
            }

            if (node.scale.size() == 3)
            {
                DirectX::XMFLOAT4 directXScale(node.scale[0], node.scale[1], node.scale[2],1);

                joint.m_Scale = directXScale;
            }

            if (node.matrix.size() == 16)
            {
                DirectX::XMFLOAT4X4 dxMatrix(node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3], node.matrix[4],
                                             node.matrix[5], node.matrix[6], node.matrix[7], node.matrix[8], node.matrix[9],
                                             node.matrix[10], node.matrix[11], node.matrix[12], node.matrix[13],
                                             node.matrix[14], node.matrix[15]);

                 DirectX::XMMATRIX tempMatrix = XMLoadFloat4x4(&dxMatrix);

                 DirectX::XMMATRIX transposedMatrix = XMMatrixTranspose(tempMatrix);

                 DirectX::XMStoreFloat4x4(&dxMatrix, transposedMatrix);

                joint.m_DefaultNodeMatrix = dxMatrix;
            }
            else
            {
            /*    DirectX::XMStoreFloat4x4(&joint.m_DefaultNodeMatrix, DirectX::XMMatrixIdentity());

               DirectX::XMFLOAT4X4 ok = joint.GetDeformedMatrixOk();

                DirectX::XMMATRIX inverse = DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&ok));*/

                joint.m_DefaultNodeMatrix = joint.GetDeformedMatrixOk();

             //  DirectX::XMStoreFloat4x4(&joint.m_DefaultNodeMatrix, inverse);
                  joint.m_Translation = DirectX::XMFLOAT4(0, 0, 0, 1);
                 joint.m_Rotation = DirectX::XMFLOAT4(0, 0, 0, 1);
                  joint.m_Scale = DirectX::XMFLOAT4(1, 1, 1, 1);


                

            }
            m_NodeToJointIndex[m_GltfNodeIndex] = i;
        }

        int root = skin.joints[0];
        SetJoint(model, root, -1);
   
    }

  /*  int joint_num = m_Joints.size();

    for (int i = 0; i < joint_num; ++i)
    {
      
        m_ComputedJointMatrices[i] = m_Joints[i].GetDeformedMatrix();
       
    }

    UpdateJoint(0);

     for (int i = 0; i < joint_num; ++i)
    {
         m_Joints[i].m_DefaultNodeMatrix = m_ComputedJointMatrices[i];

    }*/

   /* for (int i=0;i<m_Joints.size();i++)
    {
        std::cout<< m_Joints[i].m_Name << ": ";

        for (int j = 0; j < m_Joints[i].m_Children.size(); j++)
        {
            std::cout << m_Joints[m_Joints[i].m_Children[j]].m_Name << ", ";

        }
        std::cout << std::endl;
    }*/

}
void Skeleton::Update()
{
    int joint_num = m_Joints.size();

    if (!b_Animated)
    {
        for (int i = 0; i < joint_num; ++i)
        {
            DirectX::XMStoreFloat4x4(&m_ComputedJointMatrices[i], DirectX::XMMatrixIdentity());
        }
    }
    else
    {
        

        for (int i = 0; i < joint_num; ++i)
        {
         
            m_ComputedJointMatrices[i] = m_Joints[i].GetDeformedMatrixOk();
            
        }

       UpdateJoint(0);

        for (int i = 0; i < joint_num; ++i)
        {

           /* if (i == 2)
            {

                std::cout << m_Joints[i].m_Rotation.x << m_Joints[i].m_Rotation.y
                          << m_Joints[i].m_Rotation.z << m_Joints[i].m_Rotation.w<<std::endl;
            }*/
          
            DirectX::XMMATRIX computed = 
                DirectX::XMLoadFloat4x4(&m_ComputedJointMatrices[i]);

            DirectX::XMMATRIX inverse =DirectX::XMLoadFloat4x4(&m_Joints[i].m_InverseBindMatrix);
           DirectX::XMMATRIX inverse2 =
                
               DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&m_Joints[i].m_InverseBindMatrix));

       
            computed = computed * inverse;
           //int v = 0;* inverse
           //if (i > v)
           //{

           //  //  computed = DirectX::XMMatrixIdentity();
           //        DirectX::XMLoadFloat4x4(&m_ComputedJointMatrices[v]);
           //    inverse = DirectX::XMLoadFloat4x4(&m_Joints[v].m_InverseBindMatrix);

           //}
 
            DirectX::XMStoreFloat4x4(&m_ComputedJointMatrices[i], computed);

        }

       
    }
}

void Skeleton::UpdateJoint(int jointIndex)
{
    auto& currentJoint = m_Joints[jointIndex];
    int parent = currentJoint.m_ParentIndex;

    if (parent != -1)
    {
        DirectX::XMMATRIX JointMatrix = DirectX::XMLoadFloat4x4(&m_ComputedJointMatrices[jointIndex]);
        DirectX::XMMATRIX ParentMatrix = DirectX::XMLoadFloat4x4(&m_ComputedJointMatrices[parent]);

       // DirectX::XMMATRIX inverse = DirectX::XMMatrixInverse(nullptr, ParentMatrix);

       // DirectX::XMStoreFloat4x4(&mat, inverse2);

        JointMatrix =  ParentMatrix*JointMatrix  ;

        DirectX::XMStoreFloat4x4(&m_ComputedJointMatrices[jointIndex], JointMatrix); 
      
    }
   /* else
    {
        DirectX::XMStoreFloat4x4(&m_ComputedJointMatrices[jointIndex], DirectX::XMMatrixIdentity()); 
      
    }*/

   

    size_t children_num = currentJoint.m_Children.size();

    for (size_t i = 0; i < children_num; ++i)
    {
        int childJoint = currentJoint.m_Children[i];
        UpdateJoint(childJoint);
    }
}


int Skeleton::GiveNameGetIndex(std::string name)  // stfu I like this name  hehe
{

    for (int i = 0; i < m_Joints.size(); i++)
    {
        if (name == m_Joints[i].m_Name) return i;
    }
    return -1;
}
DirectX::XMFLOAT4X4 Skeleton::GiveIndexGetMatrix(int index)
{
    DirectX::XMMATRIX inverseMatrix =
        DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&m_Joints[index].m_InverseBindMatrix));

    DirectX::XMFLOAT4X4 deformedMatrix = m_ComputedJointMatrices[index];

    DirectX::XMMATRIX deformedMatrixXMM = DirectX::XMLoadFloat4x4(&deformedMatrix);

     DirectX::XMMATRIX resultMatrixXMM = DirectX::XMMatrixMultiply(deformedMatrixXMM, inverseMatrix);


      DirectX::XMStoreFloat4x4(&deformedMatrix, resultMatrixXMM);

      return deformedMatrix;
}



glm::mat4 Skeleton::GiveIndexGetMatrixGLM(int index)
{
    DirectX::XMMATRIX inverseMatrix =
        DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&m_Joints[index].m_InverseBindMatrix));

    DirectX::XMFLOAT4X4 deformedMatrix = m_ComputedJointMatrices[index];

    DirectX::XMMATRIX deformedMatrixXMM = DirectX::XMLoadFloat4x4(&deformedMatrix);

   
    DirectX::XMMATRIX dxMatrix = DirectX::XMMatrixMultiply(deformedMatrixXMM, inverseMatrix);

    return glm::mat4(dxMatrix.r[0].m128_f32[0], dxMatrix.r[1].m128_f32[0], dxMatrix.r[2].m128_f32[0], dxMatrix.r[3].m128_f32[0],
                     dxMatrix.r[0].m128_f32[1], dxMatrix.r[1].m128_f32[1], dxMatrix.r[2].m128_f32[1], dxMatrix.r[3].m128_f32[1],
                     dxMatrix.r[0].m128_f32[2], dxMatrix.r[1].m128_f32[2], dxMatrix.r[2].m128_f32[2], dxMatrix.r[3].m128_f32[2],
                     dxMatrix.r[0].m128_f32[3], dxMatrix.r[1].m128_f32[3], dxMatrix.r[2].m128_f32[3],
                     dxMatrix.r[3].m128_f32[3]);
}