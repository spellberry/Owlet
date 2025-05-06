#include "Common.hlsl"

Texture2D textures[100] : register(t4);
SamplerState s1 : register(s0);


struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 worldpos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    uint instanceID : TEXCOORD2;
};
struct PerInstanceData
{
    float4x4 WorldMatrix;
    float4x4 wMat;  
   
    float4 color;
    
    int materialI;
    int paddin1;
    int paddin2;
    int paddin3;
  
  
};
StructuredBuffer<PerInstanceData> InstanceData : register(t0);


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


float4 main(VS_OUTPUT input) : SV_TARGET
{
    const PerInstanceData instance = InstanceData[instance_num + input.instanceID];
    const int index = instance.materialI;
    const ConstantBufferMaterial mat = MaterialData[index];
   
    float4 base_color = mat.BaseTexture > -1 ? textures[mat.BaseTexture].Sample(s1, input.texCoord) : float4(1, 1, 1, 1);
    clip(base_color.a < 0.1 ? -1 : 1);
    float4 emissive = mat.EmissiveTexture > -1 ? textures[mat.EmissiveTexture].Sample(s1, input.texCoord) : float4(1, 1, 1, 1);
    
    
    
    base_color *= mat.base_color_factor * instance.color;
    emissive *= mat.emissive_color_factor;
    emissive = pow(emissive, invGamma);


    return float4(pow(base_color.rgb, 1) + emissive.rgb, instance.color.a * base_color.a);
     


}
