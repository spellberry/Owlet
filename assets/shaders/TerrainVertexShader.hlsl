#pragma target 6.0

struct VS_INPUT
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float4 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
  //  uint materialIndex : MATERIALINDEX;
    int4 materialIDs : MATERIAL_IDS;
    float4 weights : WEIGHTS;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 worldpos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float4 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
  //  uint materialID : TEXCOORD2;
    int2 materialID : TEXCOORD2;
    float2 materialWeights : TEXCOORD3;
    /* float3 WorldNormal : WORLDNORMAL;
     float3x3 TBN : TBN;*/
};
struct PerInstanceData
{
    float4x4 wvpMat;
    float4x4 wMat;
 //   float4 camera_pos;  
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
};
StructuredBuffer<ConstantBufferMaterial> MaterialData : register(t1);


StructuredBuffer<PerInstanceData> InstanceData : register(t0);

struct JointMatrix
{
    float4x4 jointMat;
};
StructuredBuffer<JointMatrix> JointData : register(t3);


// cbuffer ConstantBuffer : register(b0)
//{
//     float4x4 wvpMat;
//     float4x4 wMat;
//     float4 camera_pos;
//
// };

VS_OUTPUT main(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    const PerInstanceData instance = InstanceData[instance_num + instanceID];
    const float3x3 worldMat = (float3x3) instance.wMat;

    output.pos = mul(input.pos, instance.wvpMat);
    output.worldpos = mul(input.pos, instance.wMat);
    output.texCoord = input.texCoord;

    output.normal = float4(normalize(mul(normalize(input.normal), worldMat)), 1.0f);
    output.tangent = normalize(mul(normalize(input.tangent), worldMat));
    output.bitangent = normalize(mul(normalize(input.bitangent), worldMat));
     
    
 //   output.materialID = instanceID;
    output.materialID.x = input.materialIDs.x;
    output.materialID.y = input.materialIDs.y;
    //output.materialID.z = input.materialIDs.z;
    //output.materialID.w = input.materialIDs.w;
    
    output.materialWeights.x = input.weights.x;
    output.materialWeights.y = input.weights.y;
    //output.materialWeights.z = input.weights.z;
    //output.materialWeights.w = input.weights.w;
    //output.materialWeights.x = 1;
    //output.materialWeights.y = 0;
    //output.materialWeights.z = input.weights.z;
    //output.materialWeights.w = input.weights.w;
    
    
    
    

        return output;
}