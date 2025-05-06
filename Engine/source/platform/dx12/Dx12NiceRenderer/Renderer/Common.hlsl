// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.
struct HitInfo
{
  float4 colorAndDistance;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
  float2 bary;
};
float3 fresnelSchlick(float cosTheta, float3 F0) 
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float DistributionGGX(float NoH, float roughness)
{
    //roughness = max(roughness, 0.001);

    float a = roughness * roughness;
    float a2 = a * a;

   // NoH = clamp(NoH, 0.0, 0.99);

    float NoH2 = NoH * NoH;

    float nom = a2;
    float denom = (NoH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;

    return nom / max(denom, 0.00001);

  

}

float GeometrySchlickGGX(float NoV, float roughness)
{
    float r = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float denom = NoV * (1.0 - r) + r;

    return NoV / denom;
}

float GeometrySmith(float NoV, float NoL, float roughness)
{
    float ggx2 = GeometrySchlickGGX(NoV, roughness);
    float ggx1 = GeometrySchlickGGX(NoL, roughness);

    return ggx1 * ggx2;
}
// Written with help from https://www.youtube.com/@gsn-composer and Chat GPT
float3 BRDF(float3 L, float3 V, float3 N, float metallic, float roughness, float3 albedo)
{
    // specular
    float3 H = normalize(V + L);

    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);

    float3 f0 = float3(0.04, 0.04, 0.04);
    f0 = lerp(f0, albedo, metallic);

    float3 F = fresnelSchlick(VoH, f0);
    float D = DistributionGGX(NoH, roughness);
    float G = GeometrySmith(NoV, NoL, roughness);

    float3 spec = (F * D * G) / max(0.00001, (4.0 * max(NoV, 0.00001) * max(NoL, 0.00001)));

    // diffuse
    float3 rhoD = albedo;

    rhoD *= (float3(1.0, 1.0, 1.0) - F);

    rhoD *= (1.0 - metallic);

    float3 diff = rhoD / 3.14159;

    return diff + spec;
}