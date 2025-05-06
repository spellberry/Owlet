#include "Common.hlsl"
Texture2D textures[1] : register(t4);
SamplerState s1 : register(s0);

struct GBuffer
{
    float4 Pos : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Color : SV_Target2;
    float4 Material : SV_Target3;
};

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
//struct Light
//{
//    float4 light_position_or_direction;
//    float4 light_color;
//    float intensity;
//    int type;
//};

struct PerInstanceData
{
    float4x4 WorldMatrix;
    float4x4 wMat;
   // float4 camera_pos;
    float4 color;
    
    int materialI;
    int paddin1;
    int paddin2;
    int paddin3;
    
    
    //float dummy1;
    //float dummy2;
    //float dummy3;
    //float opacity;
   
};
StructuredBuffer<PerInstanceData> InstanceData : register(t0);

//cbuffer ConstantBufferMaterial : register(b2)
//{
//    float4 camera_pos;
//}
cbuffer ConstantBufferPerMesh : register(b0)
{
    int instance_num;

    int instance_joints_num;
    int joints_num;

}

struct ConstantBufferMaterial
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
    bool is_unlit;
    int padding1; // 4
    int padding2; // 4
    
};
StructuredBuffer<ConstantBufferMaterial> MaterialData : register(t1);


GBuffer main(VS_OUTPUT input) : SV_TARGET
{
   
    const PerInstanceData instance = InstanceData[instance_num + input.instanceID];
    const int index = instance.materialI;
    const ConstantBufferMaterial materialData= MaterialData[index];

    
    float4 base_color = materialData.BaseTexture > -1 ? textures[materialData.BaseTexture].Sample(s1, input.texCoord) : float4(1, 1, 1, 1);
    clip(base_color.a < 0.1 ? -1 : 1);
    float4 normalMapValue = materialData.NormalTexture > -1 ? textures[materialData.NormalTexture].Sample(s1, input.texCoord) : float4(0.5, 0.5, 1.0, 1.0);
    float4 metallicRoughness = materialData.MetallicRoughnessTexture > -1 ? textures[materialData.MetallicRoughnessTexture].Sample(s1, input.texCoord) : float4(0, 1, 1, 1);
    float4 emissive = materialData.EmissiveTexture > -1 ? textures[materialData.EmissiveTexture].Sample(s1, input.texCoord) : float4(1, 1, 1, 1);
    

 
    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;

  
    
    

    const float3 normalFromMap = 2.0f * normalMapValue.xyz - 1.0f;
    float3 normal = input.normal.xyz;

    
    base_color *= materialData.base_color_factor;
    emissive *= materialData.emissive_color_factor;

    
    metallic *= materialData.metallic_factor;
    roughness *= materialData.roughness_factor;
    
    const float3 defaultTangent = float3(1, 0, 0); // Choose a sensible default
    const float3 defaultBitangent = float3(0, 1, 0); // Choose a sensible default
    
    const float3 safeTangent = isnan(input.tangent.x) ? defaultTangent : input.tangent;
    const float3 safeBitangent = isnan(input.bitangent.x) ? defaultBitangent : input.bitangent;


    const float3x3 TBN = float3x3(safeTangent, safeBitangent, input.normal.xyz);
    
    normal = normalize(mul(normalFromMap, TBN));
    
    
    
    base_color *= instance.color;

  
    base_color = pow(base_color, invGamma);
    emissive = pow(emissive, invGamma);
    metallic = pow(metallic, invGamma);
    roughness = pow(roughness, invGamma);
   
    GBuffer gbuf;

     
    gbuf.Pos = float4(input.worldpos.xyz, emissive.r);

    gbuf.Normal = float4(normal.x, normal.y, normal.z, emissive.g);

    gbuf.Color = float4(base_color.rgb, instance.color.a);
    
  
    gbuf.Material = float4(0, roughness, metallic, emissive.b);
    
    return gbuf;
}
