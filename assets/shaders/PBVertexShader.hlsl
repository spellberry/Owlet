
struct VS_INPUT
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 aBar : TEXCOORD1;
    float4 aBColor : COLOR0;
    float4 aFColor : COLOR1;

};
struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD0;
 float4 aBar : TEXCOORD1;
    float4 aBColor : COLOR0;
       float4 aFColor : COLOR1;
    float xPos : TEXCOORD2;

};

cbuffer ConstantBuffer : register(b1) { float4x4 wpMat; };

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
   
     
    output.pos  = mul(input.pos, wpMat) ;  
    output.texCoord =  input.texCoord;
    output.aBar = input.aBar;
    output.aBColor = input.aBColor;
    output.aFColor = input.aFColor;
    output.xPos = input.pos.x;

  

    return output;
}