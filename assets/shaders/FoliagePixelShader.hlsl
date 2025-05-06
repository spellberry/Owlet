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
    int padding1 ; // 4
    int padding2 ; // 4
    
};
StructuredBuffer<ConstantBufferMaterial> MaterialData : register(t1);

 
GBuffer main(VS_OUTPUT input) : SV_TARGET
{
    const PerInstanceData instance = InstanceData[instance_num + input.instanceID];
    const int index = instance.materialI;
    const ConstantBufferMaterial mat = MaterialData[index];

    // Utilize GPU texture sampling
    float4 base_color = mat.BaseTexture > -1 ? textures[mat.BaseTexture].Sample(s1, input.texCoord) : float4(1, 1, 1, 1);
    clip(base_color.a - 0.1); // Clip if alpha is less than 0.1

    float4 normalMapValue = mat.NormalTexture > -1 ? textures[mat.NormalTexture].Sample(s1, input.texCoord) : float4(0.5, 0.5, 1.0, 1.0);
    float4 metallicRoughness = mat.MetallicRoughnessTexture > -1 ? textures[mat.MetallicRoughnessTexture].Sample(s1, input.texCoord) : float4(0, 1, 1, 1);
    float4 emissive = mat.EmissiveTexture > -1 ? textures[mat.EmissiveTexture].Sample(s1, input.texCoord) : float4(1, 1, 1, 1);

    
    float3 normalFromMap = 2.0 * normalMapValue.xyz - 1.0;
    float3 normal = normalize(mul(normalFromMap, float3x3(input.tangent, input.bitangent, input.normal.xyz)));

    float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal.xyz);
    normal = normalize(mul(normalFromMap, TBN));
    
    
    // Combine material properties with instance color
    base_color *= mat.base_color_factor * instance.color;
    emissive *= mat.emissive_color_factor;


   emissive *= InstanceData[instance_num + input.instanceID].color;
    // Gamma correction

    
    base_color = pow(base_color, invGamma);
    emissive = pow(emissive, invGamma);

    // Packing GBuffer
    GBuffer gbuf;
    gbuf.Pos = float4(input.worldpos.xyz, 0);
    gbuf.Normal = float4(normal, 0);
    gbuf.Color = float4(emissive.rgb, instance.color.a);
    gbuf.Material = float4(1, mat.roughness_factor, mat.metallic_factor, 0);

    return gbuf;
}
