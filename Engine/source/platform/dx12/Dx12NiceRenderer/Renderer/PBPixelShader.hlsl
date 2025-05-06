SamplerState s1 : register(s0);

Texture2D texture1 : register(t0);

cbuffer RootConstants : register(b0) { float opacity; }

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 aBar : TEXCOORD1;
    float4 aBColor : COLOR0;
    float4 aFColor : COLOR1;
    float xPos : TEXCOORD2;

};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 base_color = texture1.Sample(s1, input.texCoord);
    //return float4(1, 1, 1, 1);
    clip(base_color.a < 0.1 ? -1 : 1);
  //  return float4(input.aFColor.r, input.aFColor.g, input.aFColor.b, 1.0f);
    
    float begin = input.aBar.x;
    float diff = input.aBar.y;
    float value = input.aBar.z;

    float fragment = input.xPos - begin;

   float4 colour=0;

    if (fragment > diff * (value / 100))
    {
       colour = base_color * input.aBColor;
      //  colour = input.aBColor;
    }
    else
    {
        colour = base_color * input.aFColor;
     //   colour =  input.aFColor;
    }

    return float4(colour.rgb, colour.a * opacity);





    //   return float4(input.texCoord.r, input.texCoord.g, 0, 1.0f);
}