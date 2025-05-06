#include "Common.hlsl"

RWTexture2D<float4> uavTextures[] : register(u0);

RaytracingAccelerationStructure SceneBVH : register(t0);


cbuffer CameraParams : register(b0)
{
    float4x4 viewI;
    float4x4 projectionI;
  
    int light_count;
    
    
    int shadow_SC;
    int AO_SC;
    int reflection_SC;
    int reflected_shadow_SC;

    int show_Shadow;
    int show_AO;
    int show_Reflection;
    int show_DirectI;
    float firefly_reduction;

    float light_posx;
    float light_posy;
    float light_posz;

    int frame_accumulated;

}

uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

struct ShadowHitInfo
{
    bool isHit;
};
struct AOHitInfo
{
    float distance;
};

float3 RandomInUnitSphere(uint seed1, uint seed2, uint seed3)
{
    float3 randomVec;

    randomVec =
        float3(float(pcg_hash(seed1)) / 0xFFFFFFFF, float(pcg_hash(seed2)) / 0xFFFFFFFF, float(pcg_hash(seed3)) / 0xFFFFFFFF);

    float z = lerp(-1.0, 1.0, randomVec.x);
    float a = lerp(0.0, 2.0 * 3.14159265, randomVec.y);
    float r = sqrt(1.0 - z * z);
    float x = r * cos(a);
    float y = r * sin(a);

    float randomRadius = 1 * pow(randomVec.z, 1.0 / 3.0);

    return float3(x, y, z) * randomRadius;
}

float3 RandomInHemisphere(float3 normal, uint seed1, uint seed2)  // Chat GPT magic
{
    float r1 = float(pcg_hash(seed1)) / 0xFFFFFFFF;
    float r2 = float(pcg_hash(seed2)) / 0xFFFFFFFF;

    float sinTheta = sqrt(1.0 - r1 * r1);
    float phi = 2 * 3.1415f * r2;
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);

    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);

    return x * right + r1 * normal + y * up;
}
float2 RandomInUnitDisk(uint seed1, uint seed2)
{
    float r1 = float(pcg_hash(seed1)) / 0xFFFFFFFF;
    float r2 = float(pcg_hash(seed2)) / 0xFFFFFFFF;

    float angle = 2.0 * 3.14159265358979323846 * r1;
    float radius = sqrt(r2);

    return float2(cos(angle), sin(angle)) * radius;
}
float TraceShadowRays(float3 hitLocation, int sample_count, uint2 launchIndex, int frame)
{
    
    int hitcount = 0;

   

    const float seed1 = launchIndex.x * 4057 + launchIndex.y * 17929 + frame * 7919;
    const float seed2 = launchIndex.x * 7919 + launchIndex.y * 5801 + frame * 4273;
    const float seed3 = launchIndex.x * 5801 + launchIndex.y * 7127 + frame * 13591;
    float3 lightDir = normalize(float3(viewI[1][0], viewI[2][0], viewI[3][0]));
    float3 lightRight = cross(lightDir, abs(lightDir.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0));
    float3 lightUp = cross(lightRight, lightDir);
    
    
    for (int i = 0; i < sample_count; i++)
    {
   
        float2 diskSample = RandomInUnitDisk(i * 7127 + seed1, i * 20749 + seed2);
        float3 offset = (diskSample.x * lightRight + diskSample.y * lightUp) * viewI[0][1];

        float3 distributedLightDir = normalize(lightDir + offset);
  
        RayDesc ray;
        ray.Origin = hitLocation;
        ray.Direction = distributedLightDir;
        ray.TMin = 0.05f;
        ray.TMax = 40.0f;

        ShadowHitInfo shadowPayload;
        shadowPayload.isHit = true;

        uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

        TraceRay(SceneBVH, rayFlags, 0xFF, 0, 0, 0, ray, shadowPayload);

        hitcount += !shadowPayload.isHit;
     
    }

   // return hitcount;
    return (float(hitcount) / float(sample_count));
}
float TraceAORays(float3 hitLocation, float3 normal, int sample_count, uint2 launchIndex, int frame)
{
    float randAo1 = launchIndex.x * 32707 + launchIndex.y * 17929 + frame * 7919 + hitLocation.x * 32099 +
                    hitLocation.y * 46183 + hitLocation.z * 67607;
    float randAo2 = launchIndex.x * 7919 + launchIndex.y * 5801 + frame * 22147 + hitLocation.x * 61339 +
                    hitLocation.y * 33353 + hitLocation.z * 47057;

    float g_RayLength = 1;
    float g_OcclusionPower =1;
    float occlusion = 0;
    for (int i = 0; i < sample_count; i++)
    {
        float3 dir = RandomInHemisphere(normal, float(pcg_hash(i * 7127 + randAo1)), float(pcg_hash(i * 20749 + randAo2)));

        RayDesc ray;
        ray.Origin = hitLocation;
        ray.Direction = normalize(dir);
        ray.TMin = 0.01f;
        ray.TMax = g_RayLength;

        AOHitInfo aoPayload;
        aoPayload.distance = 0;

        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 2, ray, aoPayload);

        if (aoPayload.distance > 0)
        {
            float occlusionFactor = (aoPayload.distance / g_RayLength);
            occlusion += pow(occlusionFactor, g_OcclusionPower);
          //  float occlusionFactor = dot(normal, normalize(dir)) *(aoPayload.distance / g_RayLength)/3.14159;
            occlusion += occlusionFactor;
        }
        else
            occlusion += 1;
    }

    return (occlusion / float(sample_count));

}

[shader("raygeneration")] void RayGen()
{
   
    const uint2 launchIndex = DispatchRaysIndex().xy;
    const uint2 launchIndexDraw = DispatchRaysIndex().xy;
    
   
    const float3 hitLocation = uavTextures[1][launchIndex].rgb;
    const float3 norm = uavTextures[2][launchIndex].rgb;
    
 

  
    
    float shadow = 0;
   
    shadow = TraceShadowRays(hitLocation,8, launchIndex, 0);
        
    shadow = max(shadow, 0.0f);

  //  float ambient_occlusion = 1;
  //  ambient_occlusion = TraceAORays(hitLocation, norm, 16, launchIndex, 0);

    
 //   float fin = shadow * ambient_occlusion;
    const float4 oldPixel = uavTextures[5][launchIndex];
    float4 newPixel = (0.12f * oldPixel + 0.88f * float4(shadow, 0, 0, 1));
    uavTextures[5][launchIndexDraw] = newPixel;
   // uavTextures[5][launchIndexDraw] = float4(1,1,1,1);
    //uavTextures[5][launchIndexDraw] = float4(shadow, 0, ambient_occlusion, 1);

 
}
