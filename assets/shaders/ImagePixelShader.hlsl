SamplerState s1 : register(s0);

Texture2D texture1 : register(t0);

cbuffer RootConstants : register(b0)
{
    float opacity; 
}
struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD1;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 base_color = texture1.Sample(s1, input.texCoord);
   // return float4(1, 1, 1, 1);
    clip(base_color.a < 0.1 ? -1 : 1);

    return float4(base_color.r, base_color.g, base_color.b, base_color.a) * opacity;

    //   return float4(input.texCoord.r, input.texCoord.g, 0, 1.0f);
}