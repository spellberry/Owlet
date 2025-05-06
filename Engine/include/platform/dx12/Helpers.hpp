#pragma once

#include <exception>
#include <string>
#include <sstream>
#include <glm/glm.hpp>
#include <DirectXMath.h>

//Chat GPT magic
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        LPVOID errorMsg;
        DWORD error = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
                                    0,  
                                    (LPWSTR)&errorMsg, 0, NULL);

        std::wstringstream ss;
        if (error)  
        {
            ss << L"HRESULT failed. Code: " << std::hex << hr << L". " << (LPWSTR)errorMsg;
            LocalFree(errorMsg);
        }
        else
        {
            ss << L"HRESULT failed. Code: " << std::hex << hr << L". Message unavailable.";
        }

        auto str = ss.str();

        throw std::runtime_error(std::string(str.begin(), str.end()));
    }
}

inline DirectX::XMMATRIX ConvertGLMToDXMatrix(const glm::mat4& glmMatrix)
{

    DirectX::XMMATRIX dxMatrix(glmMatrix[0][0], glmMatrix[0][1], glmMatrix[0][2], glmMatrix[0][3], glmMatrix[1][0],
                               glmMatrix[1][1], glmMatrix[1][2], glmMatrix[1][3], glmMatrix[2][0], glmMatrix[2][1],
                               glmMatrix[2][2], glmMatrix[2][3], glmMatrix[3][0], glmMatrix[3][1], glmMatrix[3][2],
                               glmMatrix[3][3]);

    return dxMatrix;
}

inline glm::mat4 ConvertDXMatrixToGLM(const DirectX::XMMATRIX& dxMatrix)
{
    return glm::mat4(dxMatrix.r[0].m128_f32[0], dxMatrix.r[1].m128_f32[0], dxMatrix.r[2].m128_f32[0], dxMatrix.r[3].m128_f32[0],
                     dxMatrix.r[0].m128_f32[1], dxMatrix.r[1].m128_f32[1], dxMatrix.r[2].m128_f32[1], dxMatrix.r[3].m128_f32[1],
                     dxMatrix.r[0].m128_f32[2], dxMatrix.r[1].m128_f32[2], dxMatrix.r[2].m128_f32[2], dxMatrix.r[3].m128_f32[2],
                     dxMatrix.r[0].m128_f32[3], dxMatrix.r[1].m128_f32[3], dxMatrix.r[2].m128_f32[3],
                     dxMatrix.r[3].m128_f32[3]);
}

inline DirectX::XMFLOAT4 ConvertGLMToDXFLOAT4(const glm::vec4& vec)
{
    DirectX::XMFLOAT4 vector = DirectX::XMFLOAT4(vec.x, vec.y, vec.z, vec.w);
    return vector;
}

inline DirectX::XMFLOAT4 ConvertGLMToDXFLOAT4(const glm::vec3& vec,float value)
{
    DirectX::XMFLOAT4 vector = DirectX::XMFLOAT4(vec.x, vec.y, vec.z, value);
    return vector;
}