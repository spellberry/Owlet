#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D<float4> uavTextures[] : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

cbuffer CameraParams : register(b0)
{
    float4x4 viewI;
    float4x4 projectionI;
    
    float moved;
    //float4x4 view;
   // float4x4 projection;
}

uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

[shader("raygeneration")] 
void RayGen() 
{
  // Initialize the ray payload
  HitInfo payload;
  payload.colorAndDistance = float4(1, 1, 1, 0);


 uint2 launchIndex = DispatchRaysIndex().xy;
  float2 dims = float2(DispatchRaysDimensions().xy);
  float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

//  RayDesc ray;
 // ray.Origin = float3(d.x, -d.y, 1);
  //ray.Direction = float3(0, 0, -1);
 

   float aspectRatio = dims.x / dims.y;
  
  RayDesc ray;
  ray.Origin = mul(viewI, float4(0, 0, 0, 1));
  float4 target = mul(projectionI, float4(d.x, -d.y, 1, 1));
  ray.Direction = mul(viewI, float4(target.xyz, 0));

  ray.TMin = 0;
  ray.TMax = 100000;

  TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);



  
  if (moved<0.5 )
  {
      float blendFactor = 0.98;
      uavTextures[0][launchIndex] = lerp(float4(payload.colorAndDistance.rgb, 1.f), uavTextures[0][launchIndex], blendFactor);
  }
  else
      uavTextures[0][launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);


  if (launchIndex.y > 500) 
      uavTextures[0][launchIndex] = uavTextures[2][launchIndex];

 /* uavTextures[launchIndex] = 
      float4(float(pcg_hash(launchIndex.x * 587 + launchIndex.y * 2311)) / 0xFFFFFFFF,
                                float(pcg_hash(launchIndex.x * 4273 + launchIndex.y * 4591)) / 0xFFFFFFFF,
                                float(pcg_hash(launchIndex.x * 7919 + launchIndex.y * 5801)) / 0xFFFFFFFF, 1.f);*/

  
 // int sample_count = 1800;

 // uavTextures[launchIndex] = (float(launchIndex.x) / float(1920)) / float(sample_count);


  //uavTextures[launchIndex] = uavTextures[launchIndex];

    //  if (launchIndex.x > 1600 ) uavTextures[launchIndex] = float4(1,1,1, 1.f);


  // uavTextures[launchIndex] = float4(moved, moved, moved, 1);

  //uavTextures[launchIndex] = ( uavTextures[launchIndex] + float4(payload.colorAndDistance.rgb, 1.f)) / 2.0f;

   //uavTextures[launchIndex] = (uavTextures[launchIndex] + uavTextures[launchIndex + uint2(0, 1)] + uavTextures[launchIndex + uint2(0, -1)] +
   //                       uavTextures[launchIndex + uint2(1, 0)] + uavTextures[launchIndex + uint2(-1, 0)] + uavTextures[launchIndex + uint2(-1, -1)] +
   //    uavTextures[launchIndex + uint2(-1, 1)] + uavTextures[launchIndex + uint2(1, 1)] + uavTextures[launchIndex + uint2(1, -1)]) /
   //   9;



// uavTextures[launchIndex] = float4(payload.colorAndDistance.a, payload.colorAndDistance.a, payload.colorAndDistance.a, 1.0f);

 /* if (launchIndex.y>500)
   uavTextures[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
  else*/

 //  uavTextures[launchIndex] = float4(0, 1, 0, 0);

}
