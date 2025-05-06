//Texture2D texture1 : register(t0);
//Texture2D texture2 : register(t1);
//Texture2D texture3 : register(t2);
//Texture2D texture4 : register(t3);
//Texture2D texture5 : register(t4);

Texture2D textures[1] : register(t0);

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
StructuredBuffer<PerInstanceData> InstanceData : register(t3);

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


GBuffer main(VS_OUTPUT input) : SV_TARGET
{
    float4 base_color = float4(1, 1, 1, 1);
    float4 normalMapValue = float4(0.5, 0.5, 1.0, 1.0);
    float4 metallicRoughness = float4(0, 1, 1, 1);
    float4 occlusion = float4(1, 1, 1, 1);
    float4 emissive = float4(0, 0, 0, 1);

    if (BaseTexture > -1)
        base_color = textures[BaseTexture].Sample(s1, input.texCoord);

    // return base_color;

    if (NormalTexture > -1)
        normalMapValue = textures[NormalTexture].Sample(s1, input.texCoord);

    if (MetallicRoughnessTexture > -1)
        metallicRoughness = textures[MetallicRoughnessTexture].Sample(s1, input.texCoord);

    if (OcclusionTexture > -1)
        occlusion = textures[OcclusionTexture].Sample(s1, input.texCoord);

    if (EmissiveTexture > -1)
        emissive = textures[EmissiveTexture].Sample(s1, input.texCoord);
    
    

    float gamma = 2.2;

    //base_color.rgb = pow(base_color.rgb, gamma);
    //emissive.rgb = pow(emissive.rgb, gamma);

    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;

  

    float3 normalFromMap = 2.0f * normalMapValue.xyz - 1.0f;
    float3 normal = input.normal.xyz;


    base_color *= base_color_factor;
    emissive *= emissive_color_factor;
    normalFromMap *= normal_texture_scale;
    occlusion *= occlusion_texture_strength;
    metallic *= metallic_factor;
    roughness *= roughness_factor;


     if (!isnan(input.tangent.x) && !isnan(input.tangent.y) && !isnan(input.tangent.z) && !isnan(input.bitangent.x) &&
        !isnan(input.bitangent.y) && !isnan(input.bitangent.z))
    {
        float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal.xyz);
        normal = normalize(mul(normalFromMap, TBN));
    }



  

    emissive = pow(emissive, 1.0f / gamma);

    GBuffer gbuf;

    
    gbuf.Pos = float4(input.worldpos.xyz,1);

    gbuf.Normal = float4(normal.x, normal.y, normal.z, 1);

    gbuf.Color = pow(base_color, 1.0f);

    gbuf.Material = float4(is_unlit, roughness, metallic, 1);
    
    //gbuf.Pos = float4(input.worldpos.xyz, emissive.r);

    //gbuf.Normal =  float4(normal.x, normal.y, normal.z, emissive.g);

    //gbuf.Color = pow(base_color, 1.0f / gamma);

    //gbuf.Material = float4(is_unlit, roughness, metallic, emissive.b);

    return gbuf;
}
