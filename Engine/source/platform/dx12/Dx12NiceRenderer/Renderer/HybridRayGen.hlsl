#include "Common.hlsl"

RWTexture2D<float4> uavTextures[] : register(u0);

RaytracingAccelerationStructure SceneBVH : register(t0);

struct PointLight
{
    float3 light_position;
    float3 light_color;
    float size;
    float intensity;
};

//cbuffer CameraParams : register(b0)
//{
//    float4x4 viewI;
//    float4x4 projectionI;
//    float4x4 moved;
//    int frame_index;

//    int shadow_SC;
//    int AO_SC;
//    int reflection_SC;
//    int reflected_shadow_SC;

//    int show_Shadow;
//    int show_AO;
//    int show_Reflection;
//    int show_DirectI;
//    float firefly_reduction;

//    float light_posx;
//    float light_posy;
//    float light_posz;

//    int frame_accumulated;

//}

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
//struct AOHitInfo
//{
//    float distance;
//};

//float3 RandomInUnitSphere(uint seed1, uint seed2, uint seed3)
//{
//    float3 randomVec;

//    randomVec =
//        float3(float(pcg_hash(seed1)) / 0xFFFFFFFF, float(pcg_hash(seed2)) / 0xFFFFFFFF, float(pcg_hash(seed3)) / 0xFFFFFFFF);

//    float z = lerp(-1.0, 1.0, randomVec.x);
//    float a = lerp(0.0, 2.0 * 3.14159265, randomVec.y);
//    float r = sqrt(1.0 - z * z);
//    float x = r * cos(a);
//    float y = r * sin(a);

//    float randomRadius = 1 * pow(randomVec.z, 1.0 / 3.0);

//    return float3(x, y, z) * randomRadius;
//}

//float3 RandomInHemisphere(float3 normal, uint seed1, uint seed2)  // Chat GPT magic
//{
//    float r1 = float(pcg_hash(seed1)) / 0xFFFFFFFF;
//    float r2 = float(pcg_hash(seed2)) / 0xFFFFFFFF;

//    float sinTheta = sqrt(1.0 - r1 * r1);
//    float phi = 2 * 3.1415f * r2;
//    float x = sinTheta * cos(phi);
//    float y = sinTheta * sin(phi);

//    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
//    float3 right = normalize(cross(up, normal));
//    up = cross(normal, right);

//    return x * right + r1 * normal + y * up;
//}

//float3 GGXMicrofacet(uint randSeed, uint randSeed2, float roughness, float3 normal)
//{
//    float2 randVal;
//    randVal.x = float(pcg_hash(randSeed)) / 0xFFFFFFFF;
//    randVal.y = float(pcg_hash(randSeed2)) / 0xFFFFFFFF;

//    //calculate tangent/bitangent based on the normal
//    float3 up = abs(normal.y) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
//    float3 tangent = normalize(cross(up, normal));
//    float3 bitangent = cross(normal, tangent);

//    //sample the normal distribution
//    float a = roughness * roughness;
//    float a2 = a * a;
//    float cosThetaH = sqrt(max(0.0f, (1.0 - randVal.x) / ((a2 - 1.0) * randVal.x + 1)));
//    float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
//    float phiH = randVal.y * 3.14159f * 2.0f;

//    return tangent * (sinThetaH * cos(phiH)) + bitangent * (sinThetaH * sin(phiH)) + normal * cosThetaH;
//}

float TraceShadowRays(float3 hitLocation, int sample_count, uint2 launchIndex, int frame)
{
    float3 light_pos = float3(0, 1, 1);

    int hitcount = 0;

    PointLight light;
    light.light_position = light_pos;
    light.intensity = 600;
    light.size = 1;

    //float seed1 = launchIndex.x * 4057 + launchIndex.y * 17929 + frame * 7919;
    //float seed2 = launchIndex.x * 7919 + launchIndex.y * 5801 + frame * 4273;
    //float seed3 = launchIndex.x * 5801 + launchIndex.y * 7127 + frame * 13591;

    float ray_length = length(light.light_position - hitLocation);

 //   for (int i = 0; i < sample_count; i++)
    {
       // float3 sphere = RandomInUnitSphere(i * 7127 + seed1, i * 20749 + seed2, i * 6841 + seed3);
        float3 lightPos;
      
        lightPos = light.light_position; //+ sphere * light.size;
        
     //   else  // if (show_Shadow)
         //    lightPos = light.light_position;


        float3 lightDir = light_pos - hitLocation;

        RayDesc ray;
        ray.Origin = hitLocation;
        ray.Direction = normalize(lightDir);
        ray.TMin = 0.01f;
        ray.TMax = 1.0f;

        ShadowHitInfo shadowPayload;
        shadowPayload.isHit = true;

        uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, shadowPayload);

        hitcount += !shadowPayload.isHit;
    }

    return hitcount;
    //return (float(hitcount) / float(sample_count));
}
//float TraceAORays(float3 hitLocation,float3 normal, int sample_count, uint2 launchIndex, int frame) 
//{
//    float randAo1 = launchIndex.x * 32707 + launchIndex.y * 17929 + frame * 7919 + hitLocation.x * 32099 +
//                    hitLocation.y * 46183 + hitLocation.z * 67607;
//    float randAo2 = launchIndex.x * 7919 + launchIndex.y * 5801 + frame * 22147 + hitLocation.x * 61339 +
//                    hitLocation.y * 33353 + hitLocation.z * 47057;

//    float g_RayLength = 5;
//    float g_OcclusionPower = 1;
//    float occlusion = 0;
//    for (int i = 0; i < sample_count; i++)
//    {
//        float3 dir = RandomInHemisphere(normal, float(pcg_hash(i * 7127 + randAo1)), float(pcg_hash(i * 20749 + randAo2)));

//        RayDesc ray;
//        ray.Origin = hitLocation;
//        ray.Direction = normalize(dir);
//        ray.TMin = 0.01f;
//        ray.TMax = g_RayLength;

//        AOHitInfo aoPayload;
//        aoPayload.distance = 0;

//        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 2, ray, aoPayload);

//        if (aoPayload.distance > 0)
//        {
//            float occlusionFactor = (aoPayload.distance / g_RayLength);
//            occlusion += pow(occlusionFactor, g_OcclusionPower);
//          //  float occlusionFactor = dot(normal, normalize(dir)) *(aoPayload.distance / g_RayLength)/3.14159;
//          occlusion += occlusionFactor;
//        }
//        else
//            occlusion += 1;
//    }

//   return (occlusion / float(sample_count));

//}

//    float3 CalculateReflection(float3 incidentRay, float3 normal, float3 albedo, float roughness, float metallic, uint2 launchIndex,
//                           float3 hitLocation)
//{
//    float3 F0 = float3(0.04, 0.04, 0.04);
//    F0 = lerp(F0, albedo, metallic);

//   // float cosTheta = dot(normalize(-incidentRay), normal);
//    //float3 fresnel = fresnelSchlick(cosTheta, F0);

//    float3 color_r = float3(0, 0, 0);

//    float rand1 = launchIndex.x * 32707 + launchIndex.y * 17929 + frame_index * 7919 + hitLocation.x * 32099 +
//                  hitLocation.y * 46183 + hitLocation.z * 67607;
//    float rand2 = launchIndex.x * 7919 + launchIndex.y * 5801 + frame_index * 22147 + hitLocation.x * 61339 +
//                  hitLocation.y * 33353 + hitLocation.z * 47057;

//    for (int i = 0; i < reflection_SC; i++)
//    {
//        float3 H = GGXMicrofacet(pcg_hash(i * 7127 + rand1), pcg_hash(i * 20749 + rand2),
//                                    roughness, normal);

//        float3 L = normalize(2.f * dot(normalize(-incidentRay), H) * H - normalize(-incidentRay));

//        RayDesc ray;
//        ray.Origin = hitLocation;
//        ray.Direction = L;
//        ray.TMin = 0.01f;
//        ray.TMax = 10000;

//        HitInfo relfectionPayload;

//        relfectionPayload.colorAndDistance = float4(0, 0, 0, 0);
//        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 1, ray, relfectionPayload);

//        float3 r_hitLocation;

//        r_hitLocation = hitLocation + L * relfectionPayload.colorAndDistance.w;

//        float shadow = TraceShadowRays(r_hitLocation, reflected_shadow_SC, launchIndex, frame_index);

//        float NoV = clamp(dot(normal, normalize(-incidentRay)), 0.0, 1.0);

//        float NoL = clamp(dot(normal, L), 0.0, 1.0);
//        float NoH = clamp(dot(normal, H), 0.0, 1.0);
//        float VoH = clamp(dot(normalize(-incidentRay), H), 0.0, 1.0);

//        float LdotH = clamp(dot(L, H), 0.0, 1.0);

//        float3 F = fresnelSchlick(VoH, F0);
//        float D = DistributionGGX(NoH, max(roughness, 0.00001));
//        float G = GeometrySmith(NoV, NoL, roughness);

//        float3 BRDF = D * F * G / max(0.00001, (4.0 * max(NoV, 0.00001) * max(NoL, 0.00001)));

//        float GGXProb = D * NoH / (4 * LdotH);

//       /* float3 rhoD = albedo;

//        rhoD *= (float3(1.0, 1.0, 1.0) - F);

//        rhoD *= (1.0 - metallic);

//        float3 diff = rhoD / 3.14159;*/

//        color_r = color_r + relfectionPayload.colorAndDistance.rgb * (NoL * BRDF / (GGXProb)) * shadow;
//    }

// /*   float3 rhoD = albedo;

//    rhoD *= (float3(1.0, 1.0, 1.0) - fresnel);

//    rhoD *= (1.0 - metallic);

//    float3 diff = rhoD / 3.14159;*/

//    return (color_r) / float(reflection_SC);
//}

[shader("raygeneration")] void RayGen()
{
    //float3 light_pos = float3(light_posx, light_posy, light_posz);
    uint2 launchIndex = DispatchRaysIndex().xy;
    //float2 dims = float2(DispatchRaysDimensions().xy);
    //float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

    float3 hitLocation = uavTextures[1][launchIndex].rgb;
    float3 albedo = uavTextures[3][launchIndex].rgb;
    float3 norm = uavTextures[2][launchIndex].rgb;
    float roughness = uavTextures[4][launchIndex].g;
    float metallic = uavTextures[4][launchIndex].b;
    float3 emissive = float3(uavTextures[1][launchIndex].a, uavTextures[2][launchIndex].a, uavTextures[4][launchIndex].a);

    //PointLight light;
    //light.light_position = light_pos;  
    //light.intensity = 600;
    //light.size = 1;

   // float shadow = 0;
   // if (show_Shadow) 
    //    shadow = TraceShadowRays(hitLocation, 1, launchIndex, 0);

    //float unlit = uavTextures[4][launchIndex].r;

    //float4 target = mul(projectionI, float4(d.x, -d.y, 1, 1));

    //float3 dir = mul(viewI, float4(target.xyz, 0));

    //float gamma = 2.2;

    //albedo = pow(albedo, gamma);
    //emissive = pow(emissive, gamma);

    //float3 reflection = float3(0, 0, 0);
    //if (show_Reflection)
    //    reflection = CalculateReflection(normalize(dir), norm, albedo, roughness, metallic, launchIndex, hitLocation);

    //float ambient_occlusion = 1;

    //if (show_AO)
    //    ambient_occlusion = TraceAORays(hitLocation, norm, AO_SC, launchIndex, frame_index);


    //float3 lit = light.light_position - hitLocation;

    //float attenuation = 1.0f / (length(lit) * length(lit));

    //float3 final_color;

    //float irradiance = max(dot(normalize(lit), norm), 0.0f);

    //uint2 pixel = uint2(1920 - launchIndex.x, launchIndex.y);

    //if (unlit != 1)
    //{
    //    float3 radiance = float3(1, 1, 1);
    //    if (show_DirectI)
    //        radiance = irradiance * BRDF(normalize(lit), normalize(-dir), norm, metallic, roughness, albedo) * attenuation *
    //                   light.intensity;
    //    else if (show_Reflection)
    //        radiance = float3(0, 0, 0);

    //  if (moved[0][0] > 0.5)
    //    if (dot(reflection, reflection) > firefly_reduction)
    //        {
    //            float3 med = (uavTextures[0][launchIndex + uint2(0, 1)] + uavTextures[0][launchIndex + uint2(0, -1)] +
    //                          uavTextures[0][launchIndex + uint2(1, 0)] + uavTextures[0][launchIndex + uint2(-1, 0)] +
    //                          uavTextures[0][launchIndex + uint2(-1, -1)] + uavTextures[0][launchIndex + uint2(-1, 1)] +
    //                          uavTextures[0][launchIndex + uint2(1, 1)] + uavTextures[0][launchIndex + uint2(1, -1)]) /
    //                         9;

    //            if (dot(reflection, reflection) > dot(med, med)) reflection = reflection * dot(med, med);
    //        }

    //    final_color = radiance * shadow * ambient_occlusion + reflection + emissive;
    //}

    //else
    //{
    //    final_color = emissive;
    //}

    //final_color = pow(final_color, 1.0f / gamma);

    //if (moved[0][0] < 0.5)
    //{
    //    float blendFactor = 0.98;
    //    uavTextures[0][launchIndex] = lerp(float4(final_color, 1), uavTextures[0][launchIndex], blendFactor);
        
       
    ////    uavTextures[0][launchIndex] = float4((uavTextures[0][launchIndex].rgb * (frame_accumulated - 2) + final_color*2) / float(frame_accumulated), 1.0f);

    //}
   
    //else
    //{
    //    uavTextures[0][launchIndex] = float4(final_color, 1);
    //}
    
    
 //   uavTextures[0][launchIndex] = float4(hitLocation.rgb * shadow, 1);
    
    //if (albedo.r == 0 && albedo.g == 0 && albedo.b == 0)
    //    uavTextures[0][launchIndex] = float4(1,1,1,0);
    
    
    
    
    float3 light_pos = float3(0, 2, 4);

    int hitcount = 0;

  
    float3 lightDir = float3(0.3,0.4, 1);

    RayDesc ray;
   
    ray.Origin = hitLocation;
    
   //float3 lightDir = hitLocation -light_pos ;

    //RayDesc ray;
    
    //ray.Origin = light_pos;
    
    
        ray.Direction = normalize(lightDir);
        ray.TMin = 0.3f;
        ray.TMax =100.0f;

        ShadowHitInfo shadowPayload;
        shadowPayload.isHit = true;

        uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

    TraceRay(SceneBVH, rayFlags, 0xFF, 0, 0, 0, ray, shadowPayload);

        hitcount += !shadowPayload.isHit;
    
    //float color = 1;
    
    //if (hitcount == 0)
    //    color = 0.2f;
    
   
    
   // float3 light_pos = float3(0, 0,2);
    
        float3 lit = light_pos - hitLocation;

    float attenuation = 1.0f / (length(lit) * length(lit));
    
    int2 div = int2(-5,10);
    float3 shadowLocation = uavTextures[1][launchIndex + div].rgb;
      float color = 1;
    
    if (shadowLocation.z < hitLocation.z)
        color = 0;
    
      
    
    
    
    uavTextures[0][launchIndex] = float4(hitLocation.rgb, 1);
     //   uavTextures[0][launchIndex] = float4(shadow,shadow,shadow, 1);
    //   uavTextures[0][launchIndex] = float4(1,1,1, 1);
}
