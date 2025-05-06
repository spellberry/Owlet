#include "Common.hlsl"
//RWTexture2D<float4> uavTexture : register(u0);
RWTexture2D<float4> uavTextures[] : register(u0);

struct Light
{
    float3 light_position;
    float size;
    float4 light_color;
    float intensity;
    int type;
    int has_shadows;
    int dummy;
    
};


StructuredBuffer<Light> Lights : register(t0);

cbuffer ConstantBuffer : register(b0)
{
    float4x4 viewMat;
    float4x4 projMat;
    int ray_tracing_enabled;
    int light_count;
    int unlit_scene;
    float dummy;
    float3 ambient_light;
 
};

#define KERNEL_SIZE 4
#define KERNEL_RADIUS (KERNEL_SIZE / 2)

float Gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2 * sigma * sigma));
}

[numthreads(16, 16, 1)]

void main(uint3 DTid : SV_DispatchThreadID)
{
  
    float sigma = 4;
    
    float3 color = float3(1, 1, 1);
    if (ray_tracing_enabled)
    {
        color = float3(0, 0, 0);
        float totalWeight = 0.0;
        
        [unroll]
        for (int y = -KERNEL_RADIUS; y <= KERNEL_RADIUS; y++)
        {
            [unroll]
            for (int x = -KERNEL_RADIUS; x <= KERNEL_RADIUS; x++)
            {
                float weight = Gaussian(length(float2(x, y)), sigma);
                color += uavTextures[5][(DTid.xy + uint2(x, y))].rgb * weight;
                totalWeight += weight;
            }
        }

        color /= totalWeight;
    }

    uint2 launchIndex = DTid.xy;
    
    float3 hitLocation = uavTextures[1][launchIndex].rgb;
    float4 albedo = uavTextures[3][launchIndex];
    float3 norm = uavTextures[2][launchIndex].rgb;
    float roughness = uavTextures[4][launchIndex].g;
    float metallic = uavTextures[4][launchIndex].b;
    float3 emissive = float3(uavTextures[1][launchIndex].a, uavTextures[2][launchIndex].a, uavTextures[4][launchIndex].a);
    
    uavTextures[6][launchIndex / 2] = float4(0, 0, 0, 0);
    
    
    float3 dir = normalize(float3(viewMat[0][3], viewMat[1][3], viewMat[2][3]) - hitLocation);
    
    //if (albedo.r == 0 && albedo.g == 0 && albedo.b == 0 && albedo.a == 0)
    //{
     
    //    uavTextures[0][launchIndex] = float4(0.8, 0.9, 1.0, 1.0);
     
    //    return;
    //}
    
    if (unlit_scene)
    {
        uavTextures[0][launchIndex] = albedo;
        return;
    }
    
    
    


    albedo = pow(albedo, gamma);
    emissive = pow(emissive, gamma);
  
    float irradiance;
    
    float3 final_color = float3(0, 0, 0);
    for (int i = 0; i < light_count; i++)
    {
        Light light = Lights[i]; //
      
        float3 lit = light.light_position - hitLocation;
        float lengthL = length(lit);
        float attenuation = 1.0f / (lengthL * lengthL);
         
        float3 lightDir = light.light_position - hitLocation;
        
        if (light.has_shadows)
        {
            lightDir = light.light_position;
            attenuation = 1.0f;
        }
        
    
        
       
        if (uavTextures[4][launchIndex].r)
            irradiance = max(abs(dot(normalize(lightDir), norm)), 0.3f);
        else
          irradiance = max(dot(normalize(lightDir), norm), 0.0f);
        
        float3 radiance = float3(1, 1, 1);
        
        
        radiance = irradiance * BRDF(normalize(lightDir), dir, norm, metallic, roughness, albedo.rgb) * light.intensity * light.light_color.rgb * attenuation;
        
        if (light.has_shadows)
            radiance *= color.r;
        final_color += radiance; 
        
    }
    
//float AO = color.b ;
//    if (uavTextures[4][launchIndex].r)
//        AO =1.0f;
//    //* AO
    
    
    float3 ambient = ambient_light * albedo.rgb;
    final_color = pow(final_color + ambient, 1.0f / gamma) + pow(emissive, 1.0f / gamma);
 
  
    uavTextures[0][launchIndex] = float4(final_color.rgb, 1.0);
   
}