#include "Common.hlsl"
//
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

RaytracingAccelerationStructure SceneBVH : register(t2);

Texture2D<float4> texture1[] : register(t3);

SamplerState s1 : register(s0);

cbuffer CData : register(b0)
{
    int frame_index;
    int tex_index;
}

cbuffer CMatrix : register(b1) { float4x4 m_world_matrix; }

struct ShadowHitInfo
{
    bool isHit;
};

float nrand(float2 uv) { return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453); }

uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float3 RandomInUnitSphere(uint seed1, uint seed2, uint seed3)
{
    float3 randomVec;
    int diff = 0;

    do
    {
        diff++;

        randomVec = 2.0 * float3(float(pcg_hash(seed1 + pcg_hash(diff * 7919))) / 0xFFFFFFFF,
                                 float(pcg_hash(seed2 + pcg_hash(diff * 4273))) / 0xFFFFFFFF,
                                 float(pcg_hash(seed3 + pcg_hash(diff * 587))) / 0xFFFFFFFF) -
                    float3(1, 1, 1);

    } while (dot(randomVec, randomVec) >= 1.0);

    return randomVec;
}

float3 RandomInHemisphere(float3 normal, uint seed1, uint seed2)
{
    // Generate random values
    float r1 = float(pcg_hash(seed1)) / 0xFFFFFFFF;
    float r2 = float(pcg_hash(seed2)) / 0xFFFFFFFF;
    //// Convert to spherical coordinates
    float sinTheta = sqrt(1.0 - r1 * r1);
    float phi = 2 * 3.1415f * r2;
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    // Create basis
    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);
    // Combine to get random vector in hemisphere

    // return float3(0,0,0);
    return x * right + r1 * normal + y * up;
}

[shader("closesthit")] void ClosestHit(inout HitInfo payload, Attributes attrib)
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

    float3 lit = float3(0.5, 0.5, -0.5);

    float3 lite = float3(0, 3, 3);

    float3 hitLocation = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    float3 hitColor = float3(1, 1, 1);  //* dot(normalize(lit), norm);

    float3 litPos = float3(0, 3, 3);

    int hitcount = 0;

   
    int sample_count = 32;

    for (int i = 0; i < sample_count; i++)
    {
        float3 sphere = RandomInUnitSphere(
            float(pcg_hash(i * 7127 * nrand(tex * 3167) + frame_index * barycentrics.x * 2311 + frame_index * 3907)),
            float(pcg_hash(i * 4591 + frame_index * barycentrics.y * 3167 + frame_index * 7919) + nrand(tex * 5801)),
            float(pcg_hash(i * 6841 + frame_index * barycentrics.z * 4057 + frame_index * 5801) + nrand(tex * 7127)));


        float3 lightPos = litPos + sphere * 3;

        float3 lightDir = lightPos - hitLocation;

        RayDesc ray;
        ray.Origin = hitLocation;
        ray.Direction = normalize(lightDir);
        ray.TMin = 0.01;
        ray.TMax = length(lightDir) * 2;
        bool hit = true;

        ShadowHitInfo shadowPayload;
        shadowPayload.isHit = false;
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);

        if (!shadowPayload.isHit) hitcount++;
    }

    float4 base_color = texture1[tex_index].SampleGrad(s1, tex, 0, 0);

    float4 roughness = texture1[tex_index + 2].SampleGrad(s1, tex, 0, 0);

    //  if (base_color.r > 0.9 && base_color.g > 0.9 && base_color.b > 0.9)
    /*   if (metal_color.b > 0.9)
        {*/
    float3 color_r = float3(0, 0, 0);
    for (int i = 0; i < sample_count; i++)
    {
        float rand1 = float(pcg_hash(i * 7127 + frame_index * tex.x * 2311 + frame_index * 3907)) / 0xFFFFFFFF;

        float rand2 = float(pcg_hash(i * 4591 + frame_index * tex.x * 3167 + frame_index * 7919)) / 0xFFFFFFFF;

        float rand3 = float(pcg_hash(i * 6841 + frame_index * tex.y * 4057 + frame_index * 5801)) / 0xFFFFFFFF;

        /*float3 new_norm =normalize( norm + roughness.g * float3(rand1 - 1, rand2 - 1, rand3 - 1));

        if (dot(norm,new_norm) < 0)
        {
            i--;
            continue;
        }*/
        float3 new_norm = RandomInHemisphere(
            norm, float(pcg_hash(i * 7127 * nrand(tex * 3167) + frame_index * barycentrics.x * 2311 + frame_index * 3907)),
            float(pcg_hash(i * 6841 + frame_index * barycentrics.z * 4057 + frame_index * 5801) + nrand(tex * 7127)));

        // float3 new_norm = normalize(norm + roughness.g * float3(rand1 - 1, rand2 - 1, rand3 - 1));

        new_norm = lerp(norm, new_norm, roughness.g);

        float3 reflectedRay = reflect(WorldRayDirection(), new_norm);

        RayDesc ray;
        ray.Origin = hitLocation;
        ray.Direction = normalize(reflectedRay);
        ray.TMin = 0.01;
        ray.TMax = 100000;
        bool hit = true;

        HitInfo relfectionPayload;

        relfectionPayload.colorAndDistance = float4(0, 0, 0, 0);
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 2, 0, 2, ray, relfectionPayload);

        color_r = color_r + relfectionPayload.colorAndDistance.rgb;
    }

    float4 emissive = texture1[tex_index + 4].SampleGrad(s1, tex, 0, 0);

    hitColor = (color_r / float(sample_count)) * base_color.rgb + emissive;

    /*    }
        else
        {
            hitColor = base_color.rgb;

        }*/

    hitColor = hitColor * (float(hitcount) / float(sample_count));

    /*  hitColor =
          float3(float(pcg_hash(frame_index * 257 + barycentrics.x * 4591 + barycentrics.y * 3167 + barycentrics.z * 5801)) /
                     0xFFFFFFFF,
                        float(pcg_hash(frame_index * 3167 + 2 * (barycentrics.x * 7127 + barycentrics.y * 4057 + barycentrics.z
       * 2311))) / 0xFFFFFFFF, float(pcg_hash(frame_index * 2141 + 3 * (barycentrics.x * 3907 + barycentrics.y * 587 +
       barycentrics.z * 7127))) / 0xFFFFFFFF);*/

    /* hitColor = float3(float(pcg_hash(257 * (barycentrics.x * 4591 + barycentrics.y * 3167 + barycentrics.z * 5801))) /
             0xFFFFFFFF,
                float(pcg_hash(3167 * (barycentrics.x * 7127 + barycentrics.y * 4057 + barycentrics.z * 2311))) /
             0xFFFFFFFF,
                float(pcg_hash(2141 * (barycentrics.x * 3907 + barycentrics.y * 587 + barycentrics.z * 7127))) /
             0xFFFFFFFF);*/

    // hitColor = float3(barycentrics.x,barycentrics.y, barycentrics.z);

    /*float gamma = 2.2;


    hitColor = pow(hitColor, 1.0f / gamma);*/

    payload.colorAndDistance = float4(hitColor.rgb, RayTCurrent());
}
