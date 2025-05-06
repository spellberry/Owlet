#include "Common.hlsl"

struct STriVertex
{
    float3 pos : SV_POSITION;
    float2 texCoord : TEXCOORD1;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);

StructuredBuffer<int> indices : register(t1);

//RaytracingAccelerationStructure SceneBVH : register(t2);

Texture2D<float4> texture1[] : register(t2);

SamplerState s1 : register(s0);

cbuffer CData : register(b0)
{
    int frame;
    int tex_index;
}
cbuffer CMatrix : register(b1)
{
    float4x4 m_world_matrix;
}

cbuffer CameraParams : register(b3)
{
    float4x4 viewI;
    float4x4 projectionI;
    float4x4 moved;
    int frame_index;

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
}

[shader("closesthit")] void ReflectionClosestHit(inout HitInfo hit, Attributes attrib)
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint vertId = 3 * PrimitiveIndex();

    float2 tex = BTriVertex[indices[vertId + 0]].texCoord * barycentrics.x +
                 BTriVertex[indices[vertId + 1]].texCoord * barycentrics.y +
                 BTriVertex[indices[vertId + 2]].texCoord * barycentrics.z;

    float3 norm = normalize(normalize(BTriVertex[indices[vertId + 0]].normal) * barycentrics.x +
                            normalize(BTriVertex[indices[vertId + 1]].normal) * barycentrics.y +
                            normalize(BTriVertex[indices[vertId + 2]].normal) * barycentrics.z);

    norm = normalize(mul(norm, (float3x3)m_world_matrix));

    float3 tangent = normalize(normalize(BTriVertex[indices[vertId + 0]].tangent) * barycentrics.x +
                               normalize(BTriVertex[indices[vertId + 1]].tangent) * barycentrics.y +
                               normalize(BTriVertex[indices[vertId + 2]].tangent) * barycentrics.z);

    tangent = normalize(mul(tangent, (float3x3)m_world_matrix));

    float3 bitangent = normalize(normalize(BTriVertex[indices[vertId + 0]].bitangent) * barycentrics.x +
                                 normalize(BTriVertex[indices[vertId + 1]].bitangent) * barycentrics.y +
                                 normalize(BTriVertex[indices[vertId + 2]].bitangent) * barycentrics.z);

    bitangent = normalize(mul(bitangent, (float3x3)m_world_matrix));

    float4 normalMapValue = texture1[tex_index + 1].SampleGrad(s1, tex, 0, 0);

    float3 normalFromMap = 2.0f * normalMapValue.xyz - 1.0f;

    float3x3 TBN = float3x3(tangent, bitangent, norm);

    norm = normalize(mul(normalFromMap, TBN));

    float4 base_color = texture1[tex_index].SampleGrad(s1, tex, 0, 0);
    float4 material = texture1[tex_index+2].SampleGrad(s1, tex, 0, 0);
    float4 emissive = texture1[tex_index + 4].SampleGrad(s1, tex, 0, 0);
   
    float roughness = material.g;
    float metallic = material.b;

    base_color = pow(base_color, gamma);
    emissive = pow(emissive, gamma);

    float dist;
  
    dist = RayTCurrent();
   
    float3 litPos = float3(light_posx, light_posy, light_posz);  

    float3 hitLocation = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    float3 lit = litPos - hitLocation;
 
    float irradiance = max(dot(normalize(lit), norm), 0.0f);
    float attenuation = 1.0f / (length(lit) * length(lit));

 
    float3 color = BRDF(normalize(lit), normalize(-WorldRayDirection()), norm, metallic, roughness, base_color) 
        //base_color.rgb
        * irradiance * attenuation * 600.0f 
        +
        emissive;
   
    hit.colorAndDistance = float4(color.rgb, dist);

  
}

    [shader("miss")] void ReflectionMiss(inout HitInfo hit
                                         : SV_RayPayload)
{
    hit.colorAndDistance = float4(0.8f, 0.8f, 0.75f, 0);
}