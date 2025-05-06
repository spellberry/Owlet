//#include "Common.hlsl"
//RWTexture2D<float4> uavTexture : register(u0);
RWTexture2D<float4> uavTextures[] : register(u0);

cbuffer ConstantBuffer : register(b0)
{
    int bloom_sigma;
    int kernel_size;
    float bloom_multiplier;
    float dummy1;
};

//#define KERNEL_SIZE 32
#define KERNEL_RADIUS (kernel_size / 2)


float Gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2 * sigma * sigma));
}

[numthreads(16, 16, 1)]

void main(uint3 DTid : SV_DispatchThreadID)
{
  
    uint2 launchIndex = DTid.xy;
    float3 color = float3(0, 0, 0);
    float totalWeight = 0.0;

    for (int y = -KERNEL_RADIUS; y <= KERNEL_RADIUS; y++)
    {
        float weight = Gaussian(y, bloom_sigma);
        
        color += uavTextures[6][(DTid.xy + uint2(0, y))/2].rgb * weight;
     
        totalWeight += weight;
    }

    color /= totalWeight;
    
    
   // color = uavTextures[6][launchIndex / 2].rgb;
 
  //  float4(uavTextures[0][launchIndex].rgb+color, 1);
    
   // uavTextures[0][launchIndex] = uavTextures[6][launchIndex/2];;
    
   // uavTextures[0][launchIndex] = float4(uavTextures[0][launchIndex].r,0,0.5, 1);
    uavTextures[0][launchIndex] = float4(uavTextures[0][launchIndex].rgb + (color * bloom_multiplier),
    1);
   // uavTextures[0][launchIndex] = float4(color, 1.0f);
}