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
    float4 color : COLOR01;
};

float screenPxRange(float2 PxRange, Texture2D tex, float2 texCoord)
{
    float2 textureSize;
    tex.GetDimensions(textureSize.x, textureSize.y);

    float2 unitRange = PxRange / textureSize;
    float2 ddxTexCoord = ddx(texCoord);
    float2 ddyTexCoord = ddy(texCoord);

    float2 screenTexSize = float2(1, 1) / float2(length(ddxTexCoord), length(ddyTexCoord));
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));

}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float pxRange = 6;

    float4 base_color = texture1.Sample(s1, input.texCoord);

    float sd = median(base_color.r, base_color.g, base_color.b);
    float screenPxDistance = screenPxRange(pxRange, texture1, input.texCoord) * (sd - 0.5);


    float fopacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    clip(fopacity == 0 ? -1 : 1);
      
 
    return float4(input.color.rgb, (input.color.a * fopacity) * opacity);

}