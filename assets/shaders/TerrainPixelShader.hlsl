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
    int2 materialID : TEXCOORD2;
    float2 materialWeights : TEXCOORD3;
 //   uint materialID : TEXCOORD2;
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
    int padding1 ; // 4
    int padding2 ; // 4
    
};
StructuredBuffer<ConstantBufferMaterial> MaterialData : register(t1);


GBuffer main(VS_OUTPUT input) : SV_TARGET
{


    const ConstantBufferMaterial mat = MaterialData[input.materialID.x];
    const ConstantBufferMaterial mat2 = MaterialData[input.materialID.y];
    float4 base_color ;
   
    float4 normalMapValue;
    float4 metallicRoughness ;
   
    float4 base_color1 = mat.BaseTexture > -1 ? textures[mat.BaseTexture].Sample(s1, input.texCoord) : float4(1, 1, 1, 1);
    float4 normalMapValue1 = mat.NormalTexture > -1 ? textures[mat.NormalTexture].Sample(s1, input.texCoord) : float4(0.5, 0.5, 1.0, 1.0);
    float4 metallicRoughness1 = mat.MetallicRoughnessTexture > -1 ? textures[mat.MetallicRoughnessTexture].Sample(s1, input.texCoord) : float4(0, 1, 1, 1);
    
    
    float4 base_color2 = mat2.BaseTexture > -1 ? textures[mat2.BaseTexture].Sample(s1, input.texCoord) : float4(1, 1, 1, 1);
    float4 normalMapValue2 = mat2.NormalTexture > -1 ? textures[mat2.NormalTexture].Sample(s1, input.texCoord) : float4(0.5, 0.5, 1.0, 1.0);
    float4 metallicRoughness2 = mat2.MetallicRoughnessTexture > -1 ? textures[mat2.MetallicRoughnessTexture].Sample(s1, input.texCoord) : float4(0, 1, 1, 1);
   

    
    base_color = base_color1 * input.materialWeights.x + base_color2 * input.materialWeights.y;
   
    normalMapValue = normalMapValue1 * input.materialWeights.x + normalMapValue2 * input.materialWeights.y;
    metallicRoughness = metallicRoughness1 * input.materialWeights.x + metallicRoughness2 * input.materialWeights.y;

    
    
    
    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;

  
    const float3 normalFromMap = 2.0f * normalMapValue.xyz - 1.0f;
    float3 normal = input.normal.xyz;

 //   base_color *= occlusion;
    //base_color *= mat.base_color_factor;
   

    
    //metallic *= mat.metallic_factor;
    //roughness *= mat.roughness_factor;


   
    const float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal.xyz);
    normal = normalize(mul(normalFromMap, TBN));
    
    
  

  //  base_color *= InstanceData[instance_num + input.instanceID].color;

  
    base_color = pow(base_color, invGamma);
  
    metallic = pow(metallic, invGamma);
    roughness = pow(roughness, invGamma);
   
    GBuffer gbuf;

     
    gbuf.Pos = float4(input.worldpos.xyz,0);

    gbuf.Normal = float4(normal.x, normal.y, normal.z,0);

    gbuf.Color = float4(base_color.rgb, 1);
    
    gbuf.Material = float4(0, roughness, metallic, 0);
    
    return gbuf;
}
