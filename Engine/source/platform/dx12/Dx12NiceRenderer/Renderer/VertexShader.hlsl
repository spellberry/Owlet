struct VS_INPUT
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD;
    float4 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 worldpos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float4 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;

   /* float3 WorldNormal : WORLDNORMAL;
    float3x3 TBN : TBN;*/

};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 wvpMat;
    float4x4 wMat;
    float4 camera_pos;
  
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    output.pos = mul(input.pos, wvpMat);
    output.worldpos = mul(input.pos, wMat);
    output.texCoord = float2(input.texCoord.x, input.texCoord.y);

    
    float3 norm = normalize(mul(input.normal.xyz, (float3x3)wMat));

    output.normal = float4(norm, 1.0f);
    output.tangent = normalize(mul(input.tangent, (float3x3)wMat));
    output.bitangent = normalize(mul(input.bitangent, (float3x3)wMat));

  

    return output;
}