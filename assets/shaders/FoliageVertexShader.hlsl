#pragma target 6.0

struct VS_INPUT
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float4 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
   // int4 jointIDs : JOINT_IDS;
   // float4 weights : WEIGHTS;
    
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
    float4x4 wvpMat;
    float4x4 wMat;
 
    float4 color;
    
    int materialI;
    float paddin1;
    int paddin2;
    int paddin3;
  
  
};
cbuffer ConstantBufferPerMesh : register(b0)
{
    int instance_num;

    int instance_joints_num;
    int joints_num;
}

cbuffer Constants : register(b1)
{
    float time;
};

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



uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}
float3 hash31(float p)
{
    float3 p3 = frac(float3(p,p,p) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return frac((p3.xxy+p3.yzz)*p3.zyx); 
}



float hash12(float2 x)
{
    return frac(sin(dot(x, float2(21.57851, 20.47856))) * 1347.8967423);
}

float noise12(float2 x)
{
    float2 rootUV = floor(x);
    float2 fracResult  = frac(x);
    
    float n00 = hash12(rootUV + float2(0.0, 0.0));
    float n01 = hash12(rootUV + float2(0.0, 1.0));
    float n10 = hash12(rootUV + float2(1.0, 0.0));
    float n11 = hash12(rootUV + float2(1.0, 1.0));
    
    float n0 = lerp(n00, n01, fracResult.y);
    float n1 = lerp(n10, n11, fracResult.y);
    
    return lerp(n0, n1, fracResult.x);
}
float invLerp(float from, float to, float value){
    return (value - from) / (to - from);
}
float remap(float origFrom, float origTo, float targetFrom, float targetTo, float value){
    float rel = invLerp(origFrom, origTo, value);
    return lerp(targetFrom, targetTo, rel);
}
float easeIn(float x)
{
    return pow(2.0, 10.0 * x - 10.0);
    //return 1.0 - cos((x * 3.1415) / 2.0);
}

#define swayAmplitude 0.35

VS_OUTPUT main(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    const PerInstanceData instance = InstanceData[instance_num + instanceID];
    const float3x3 worldMat = (float3x3) instance.wMat;
    float4 pos = input.pos;
    const float windMultiplier = instance.paddin1;

    float3 worldPosition = instance.wMat._m30_m31_m32;
    float noiseSample = noise12(float2(time * 0.35, time * 0.35) + worldPosition.xy);
    
    pos.x += noiseSample * 0.75 * windMultiplier;
    pos.z += noiseSample * 0.75 * windMultiplier;

    float windDir = noise12(worldPosition.xy * 0.05 + 0.05 * time);

    //remap from 0 to 360 degrees
    windDir = remap(-1.0, 1.0, 0.0, 3.1415 * 2.0, windDir);

    float windNoiseSample = noise12(worldPosition.xy * 0.25 + time);
    
    float windLeanAngle = remap(-1.0, 1.0, 0.25, 1.0, windNoiseSample);
    windLeanAngle = easeIn(windLeanAngle) * 1.25 * windMultiplier;

    // Calculate displacement

    pos.x += sin(windDir) * windLeanAngle * swayAmplitude * pos.y;
    pos.z += cos(windDir) * windLeanAngle * swayAmplitude * pos.y;


    output.pos = mul(pos, instance.wvpMat);
    output.worldpos = mul(input.pos, instance.wMat);
    output.texCoord = input.texCoord;

   
    output.normal = float4(normalize(mul(normalize(input.normal), worldMat)), 1.0f);
    output.tangent = normalize(mul(normalize(input.tangent), worldMat));
    output.bitangent = normalize(mul(normalize(input.bitangent), worldMat));
    
    output.instanceID = instanceID;

    return output;
}