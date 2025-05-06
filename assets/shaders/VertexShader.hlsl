#pragma target 6.0

struct VS_INPUT
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float4 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    int4 jointIDs : JOINT_IDS;
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
    uint instanceID : TEXCOORD2;
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
    
    float4 animatedPosition = float4(0,0,0,0);
    float3 animatedNormal = float3(0, 0, 0);
    float3 animatedTangent = float3(0, 0, 0);
    float3 animatedBitangent = float3(0, 0, 0);
    
    float4x4 jointTransform;
    
    
    int c = 0;
    for (int i = 0; i < 4;i++)
    {
        if (input.weights[i] == 0)
        {
            continue;
        }
        c++;
       
        float4x4 jointMat = JointData[ instance_joints_num +input.jointIDs[i] + instanceID * joints_num].jointMat;
        
    
        float4 localPos = mul(float4(input.pos.xyz,1), jointMat);
        animatedPosition += localPos * input.weights[i];
  
        float3x3 normalMatrix = (float3x3) jointMat;
        animatedNormal += mul(input.normal.xyz, normalMatrix) * input.weights[i];
        
        animatedTangent += mul(input.tangent, normalMatrix) * input.weights[i];
        animatedBitangent += mul(input.bitangent, normalMatrix) * input.weights[i];
        
        
    }
    
    if (c == 0 )
    {
       animatedPosition = input.pos;
    animatedNormal = input.normal;
        animatedTangent = input.tangent;
        animatedBitangent = input.bitangent;
       
    } 
    
    //    output.pos = input.pos;
    animatedNormal = normalize(animatedNormal);
    animatedTangent = normalize(animatedTangent);
    animatedBitangent = normalize(animatedBitangent);
    
    output.pos = mul(animatedPosition, InstanceData[instance_num + instanceID].wvpMat);
    output.worldpos = mul(animatedPosition, InstanceData[instance_num + instanceID].wMat);
    output.texCoord = float2(input.texCoord.x, input.texCoord.y);

   // float3 norm = normalize(mul(input.normal.xyz, (float3x3)InstanceData[instance_num + instanceID].wMat));

    output.normal = float4(normalize(mul(animatedNormal, (float3x3) InstanceData[instance_num + instanceID].wMat)), 1.0f);
    
    output.tangent = normalize(mul(animatedTangent, (float3x3) InstanceData[instance_num + instanceID].wMat));
    output.bitangent = normalize(mul(animatedBitangent, (float3x3) InstanceData[instance_num + instanceID].wMat));
    
    //output.tangent = normalize(mul(input.tangent, (float3x3)InstanceData[instance_num + instanceID].wMat));
    //output.bitangent = normalize(mul(input.bitangent, (float3x3)InstanceData[instance_num + instanceID].wMat));
    output.instanceID = instanceID;

    return output;
}