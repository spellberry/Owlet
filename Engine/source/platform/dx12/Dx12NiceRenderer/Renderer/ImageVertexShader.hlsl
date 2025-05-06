struct VS_INPUT
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD;
};
struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD1;
};


cbuffer ConstantBuffer : register(b1)
{
    float4x4 wpMat;
};


VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos =  input.pos;
    output.pos = mul(output.pos, wpMat);  
    output.texCoord =  input.texCoord;



    return output;
}