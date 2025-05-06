Texture2D textures[100] : register(t4);

SamplerState s1 : register(s0);

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 worldpos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float4 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    uint instanceID : TEXCOORD2;
};
struct Light
{
    float4 light_position_or_direction;
    float4 light_color;
    float intensity;
    int type;
};
struct PerInstanceData
{
    float4x4 WorldMatrix;
    float4x4 wMat;
    float4 camera_pos;
    float opacity;
    float dummy1;
    float dummy2;
    float dummy3;
};

StructuredBuffer<PerInstanceData> InstanceData : register(t0);

cbuffer ConstantBufferMaterial : register(b1)
{
    float4 base_color_factor;
    float4 emissive_color_factor;
    float normal_texture_scale;
    float occlusion_texture_strength;
    float metallic_factor;
    float roughness_factor;

    int BaseTexture;
    int EmissiveTexture;
    int NormalTexture;
    int OcclusionTexture;
    int MetallicRoughnessTexture;

    int instance_num;

    int instance_joints_num;
    int joints_num;

    bool is_unlit = false;
}
// cbuffer ConstantBufferDebug : register(b2)
//{
//     bool show_base_color;
//     bool show_normal_map;
//     bool show_metal_map;
//     bool show_roughness_map;
//     bool show_ambient_occlusion;
//     bool show_emissive;
//
//     bool show_normals;
//     bool only_normals;
//     bool only_metallic;
//     bool only_roughness;
//
//     bool show_Specular;
//     bool show_Diffuse;
//
//     bool showFresnel;
//
//     bool showTangents;
//
//     bool showBitangents;
// }
//
StructuredBuffer<Light> lights : register(t1);

float3 fresnelSchlick(float cosTheta, float3 F0) { return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); }

float DistributionGGX(float NoH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NoH2 = NoH * NoH;

    float nom = a2;
    float denom = (NoH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NoV, float roughness)
{
    float r = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float denom = NoV * (1.0 - r) + r;

    return NoV / denom;
}

float GeometrySmith(float NoV, float NoL, float roughness)
{
    float ggx2 = GeometrySchlickGGX(NoV, roughness);
    float ggx1 = GeometrySchlickGGX(NoL, roughness);

    return ggx1 * ggx2;
}

//// Written with help from https://www.youtube.com/@gsn-composer and Chat GPT
//
float3 BRDF(float3 L, float3 V, float3 N, float metallic, float roughness, float3 baseColor)
{
    // specular
    float3 H = normalize(V + L);

    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);

    float3 f0 = float3(0.04, 0.04, 0.04);
    f0 = lerp(f0, baseColor, metallic);

    float3 F = fresnelSchlick(VoH, f0);
    float D = DistributionGGX(NoH, roughness);
    float G = GeometrySmith(NoV, NoL, roughness);

    float3 spec = (F * D * G) / max(0.00001, (4.0 * max(NoV, 0.00001) * max(NoL, 0.00001)));

    // diffuse
    float3 rhoD = baseColor;

    rhoD *= (float3(1.0, 1.0, 1.0) - F);

    rhoD *= (1.0 - metallic);

    float3 diff = rhoD / 3.14159;

    return diff + spec;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 base_color = float4(1, 1, 1, 1);
    float4 normalMapValue = float4(0.5, 0.5, 1.0, 1.0);
    float4 metallicRoughness = float4(0, 1, 1, 1);
    float4 occlusion = float4(1, 1, 1, 1);
    float4 emissive = float4(0, 0, 0, 1);

    if (BaseTexture > -1)
    {
        base_color = textures[BaseTexture].Sample(s1, input.texCoord);
        clip(base_color.a < 0.1 ? -1 : 1);
    }
    // return base_color;

        if (NormalTexture > -1)
            normalMapValue = textures[NormalTexture].Sample(s1, input.texCoord);

        if (MetallicRoughnessTexture > -1)
            metallicRoughness = textures[MetallicRoughnessTexture].Sample(s1, input.texCoord);

        if (OcclusionTexture > -1)
            occlusion = textures[OcclusionTexture].Sample(s1, input.texCoord);

        if (EmissiveTexture > -1)
            emissive = textures[EmissiveTexture].Sample(s1, input.texCoord);

    //  return base_color;

        float gamma = 2.2;

        base_color.rgb = pow(base_color.rgb, gamma);
        emissive.rgb = pow(emissive.rgb, gamma);

        float metallic = metallicRoughness.b;
        float roughness = metallicRoughness.g;

    //    //return normalMapValue;

    //   if (only_normals)
    //   {
    //       return normalMapValue;
    //   }

    //   if (only_metallic)
    //   {
    //       if (only_roughness)
    //       {
    //           return float4(0, roughness, metallic, 1);
    //       }

    //       return float4(0, 0, metallic, 1);
    //   }

    //   if (only_roughness)
    //   {
    //       return float4(0, roughness, 0, 1);
    //   }

    // /*  if (!show_base_color) base_color = float4(1.0, 1.0, 1.0, 1.0);
    //   if (!show_normal_map) normalMapValue = float4(0.5, 0.5, 1.0, 1.0);
    //   if (!show_metal_map) metallic = 0;
    //   if (!show_roughness_map) roughness = 0.5;
    //   if (!show_ambient_occlusion) occlusion = float4(1.0, 1.0, 1.0, 1.0);
    //   if (!show_emissive) emissive = float4(0.0, 0.0, 0.0, 0.0);*/

        if (is_unlit)
        {
            return float4(base_color.rgb, base_color.a * InstanceData[instance_num + input.instanceID].opacity);
        }

    //   if (showTangents)
    //   {
    //       return float4((input.tangent.x + 1.0f) / 2.0f, (input.tangent.y + 1.0f) / 2.0f, (input.tangent.z + 1.0f) / 2.0f,
    //       1);
    //   }

    //   if (showBitangents)
    //   {
    //       return float4((input.bitangent.x + 1.0f) / 2.0f, (input.bitangent.y + 1.0f) / 2.0f, (input.bitangent.z + 1.0f)
    //       / 2.0f,
    //                     1);
    //   }
        float3 normalFromMap = 2.0f * normalMapValue.xyz - 1.0f;
        float3 normal = input.normal.xyz;

        if (!isnan(input.tangent.x) && !isnan(input.tangent.y) && !isnan(input.tangent.z) && !isnan(input.bitangent.x) &&
        !isnan(input.bitangent.y) && !isnan(input.bitangent.z))
        {
            float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal.xyz);
            normal = normalize(mul(normalFromMap, TBN));
        }

    //   if (show_normals)
    //   {
    //       return float4((normal.x + 1.0f) / 2.0f, (normal.y + 1.0f) / 2.0f, (normal.z + 1.0f) / 2.0f, 1);
    //   }

        base_color *= base_color_factor;
        emissive *= emissive_color_factor;
    //  normal *= normal_texture_scale;
    //  occlusion *= occlusion_texture_strength;
        metallic *= metallic_factor;
        roughness *= roughness_factor;

        float3 radiance = emissive.rgb;

    //   // Lights
        for (int i = 0; i < 5; i++)
        {
            float attenuation = 1.0f;
            float3 light_dir = float3(0, 0, 0);
            if (lights[i].type == 0)
            {
                light_dir = -normalize(lights[i].light_position_or_direction.xyz);
            }
            else if (lights[i].type == 1)
            {
                light_dir = normalize(lights[i].light_position_or_direction.xyz - input.worldpos.xyz);

                float distance = length(lights[i].light_position_or_direction.xyz - input.worldpos.xyz);

                attenuation = 1.0 / (distance * distance);
            }

            float3 view_dir = normalize(InstanceData[0].camera_pos.xyz - input.worldpos.xyz);

            float irradiance = max(dot(light_dir, normal), 0.0);

            if (irradiance > 0.0)
            {
                float3 brdf = BRDF(light_dir, view_dir, normal, metallic, roughness, base_color.rgb);

                radiance += brdf * irradiance * lights[i].light_color.rgb * lights[i].intensity * attenuation;
            }
        }

        radiance *= occlusion.r;

        float3 finalColor = pow(radiance, 1.0f / gamma);

    //  /*  float4 base_color;
    //   float4 normalMapValue;
    //   float4 metallicRoughness;
    //   float4 occlusion;
    //   float4 emissive;*/
    //
    //   //return base_color;
    ////   return float4(base_color.a, base_color.a, base_color.a, 1);* InstanceData[input.instanceID].opacity)

        return float4(finalColor, base_color.a * InstanceData[instance_num + input.instanceID].opacity);
    }
