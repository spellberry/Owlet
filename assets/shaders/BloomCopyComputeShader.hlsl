#include "Common.hlsl"

RWTexture2D<float4> uavTextures[] : register(u0);

cbuffer ConstantBuffer : register(b0)
{
    int bloom_sigma;
    int kernel_size;
    float bloom_multiplier;
    float bloom_threshold;
};

//#define KERNEL_SIZE 32
//#define KERNEL_RADIUS (kernel_size / 2)

//float Gaussian(float x, float sigma)
//{
//    return exp(-(x * x) / (2 * sigma * sigma));
//}

[numthreads(16, 16, 1)]

void main(uint3 DTid : SV_DispatchThreadID)
{
  
    uint2 launchIndex = DTid.xy;
    float4 color = uavTextures[0][launchIndex*2];


    float3 originalHSV = RGBtoHSV(color.rgb);


    if (originalHSV.z > bloom_threshold)
    {

        uavTextures[6][launchIndex] = color;
    }
    else
    {

        uavTextures[6][launchIndex] = float4(0, 0, 0, 1);
    }
    
   
}