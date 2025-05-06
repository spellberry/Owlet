#include "Common.hlsl"


RWTexture2D<float4> uavTextures[] : register(u0);
cbuffer ConstantBufferImageEffects : register(b0)
{
    float4x4 view;
    float4x4 proj;
    float4 hsvc; // hue, saturation, value, contrast
    
    float2 resolution;
    int frameCount;
    float grainAmount;
    
    float4 vignetteColor;
    float2 vignetteSettings; // x = strength, y = radius
    float time;
    float fogOffset;
    float airDensity;
    float3 fogColorCB;
    float3 sunDirection;
};

//#define KERNEL_SIZE 32
//#define KERNEL_RADIUS (KERNEL_SIZE / 2)
//#define Epsilon  1e-10

float saturateFunction(float num)
{
    return clamp(num, 0.0, 1.0);
}



float hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);

}

float noise(in float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);
    f = f * f * (3. - 2. * f);
    
    float lb = hash(i + float2(0., 0.));
    float rb = hash(i + float2(1., 0.));
    float lt = hash(i + float2(0., 1.));
    float rt = hash(i + float2(1., 1.));
    
    return lerp(lerp(lb, rb, f.x),
               lerp(lt, rt, f.x), f.y);
}

#define OCTAVES 8
float fbm(in float2 uv)
{
    float v = 0.;
    float amplitude = .5;
    
    for (int i = 0; i < OCTAVES; i++)
    {
        v += noise(uv) * amplitude;
        
        amplitude *= .5;
        
        uv *= 2.;
    }
    
    return v;
}
        


float3 Sky(in float3 ro, in float3 rd)
{
    const float SC = 1e5;

    // Calculate sky plane
    float dist = (SC - ro.z) / rd.z;
    float2 p = (ro + dist * rd).xy;
    p *= 2.4 / SC;
    
    // from iq's shader, https://www.shadertoy.com/view/MdX3Rr
    float3 lightDir = normalize(sunDirection); //sunDirection
    float sundot = clamp(dot(rd, lightDir), 0.0, 1.0);
    
    float3 cloudCol = float3(0.9, 0.9,0.9);
    float3 skyCol = float3(.6, .71, .85) - rd.z * .2 * float3(1., .5, 1.) + .15 * .5;
    //float3 skyCol = float3(0.3, 0.5, 0.85) - rd.z * rd.z * 0.5;
    skyCol = lerp(skyCol, 0.85 * float3(0.7, 0.75, 0.85), pow(1.0 - max(rd.z, 0.0), 4.0));
    
    // sun
    float3 sun = 0.2 * float3(1.0, 0.7, 0.4) * pow(sundot, 8.0);
    sun += 0.45 * float3(1.0, 0.8, 0.4) * pow(sundot, 2048.0);
    sun += 0.2 * float3(1.0, 0.8, 0.4) * pow(sundot, 128.0);
sun = clamp(sun,0.0,1.0);
    skyCol += sun;
    
    // clouds
    float t = time * 0.1;
    float den = fbm(float2(p.x - t, p.y - t));
    skyCol = lerp(skyCol, cloudCol, smoothstep(.4, .8, den));
    
    // horizon
    skyCol = lerp(skyCol, fogColorCB, pow(1.0 - max(rd.z, 0.0), 16.0));
    
    return skyCol;
}

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const uint2 launchIndex = DTid.xy;
    const float3 hitLocation = uavTextures[1][launchIndex].rgb;
    const float4 originalColor = uavTextures[0][launchIndex];
    
    const float fogIntensity = clamp(exp((-hitLocation.z - fogOffset) * airDensity), 0, 1); // Calculate fog intensity based on height
    const float3 fogColor = fogColorCB; // Light blueish fog color
    float3 colorWithFog = lerp(originalColor.rgb, fogColor, fogIntensity);

    
    const float4 albedo = uavTextures[3][launchIndex];
    float isZero = dot(albedo, albedo);
    if (isZero== 0)
    {
        const float aspectRatio = resolution.x / resolution.y;
        float2 uv = launchIndex / resolution;
        uv -= 0.5;
        uv.x *= aspectRatio;
        uv.y -= 0.4 * 1.0/aspectRatio;
        uv.y = -uv.y;
		const float smoothCurve = lerp(0.0, 0.45, smoothstep(-0.5, 0.5, uv.y));
        const float curve = -(1.0-dot(uv,uv)* smoothCurve);
        const float3 rayDir = normalize(mul( float3(uv, curve),(float3x3) view));
        const float3 ro = float3(0, 0.0, 0);
        colorWithFog = Sky(ro, rayDir);
    }
    
    
    
    float3 originalHSV = RGBtoHSV(colorWithFog.rgb);

//adujst the hue
    originalHSV.x *= hsvc.x;
    originalHSV.y *= hsvc.y;
    originalHSV.z *= hsvc.z;

//adjust contrast
    originalHSV.z = 0.5 + hsvc.w * (originalHSV.z - 0.5);
    const float3 newColor = HSVtoRGB(originalHSV);
    
    //vignette
    const float vignetteStrength = vignetteSettings.x;
    const float vignetteRadius = vignetteSettings.y;
    float2 coords = launchIndex / resolution; // Normalize coordinates
    coords *= 1.0 - coords.yx;
    float vig = coords.x * coords.y * vignetteStrength;
    vig = smoothstep(0.0, 1.0, vig); // Smooth step for a smoother transition
    vig = pow(vig, vignetteRadius);

    // Film grain effect
    const float noise = frac(sin(dot(launchIndex / resolution, float2(12.9898, 78.233))) * 43758.5453 + (float(frameCount) * 0.1));

    const float3 blendedColor = lerp(newColor.rgb, vignetteColor * newColor.rgb, 1 - vig);
    
    float3 color = blendedColor - noise * grainAmount;
    //result
    uavTextures[0][launchIndex] = float4(color, 1);

    
    
    //   uavTextures[0][launchIndex] = float4(uavTextures[0][launchIndex].rgb + color, 1);

}