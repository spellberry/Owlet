struct AOHitInfo
{
    float distance;
};

struct Attributes
{
    float2 uv;
};

[shader("closesthit")]
void AOClosestHit(inout AOHitInfo hit, Attributes bary) 
{ 
    hit.distance = RayTCurrent(); 

}
 
 [shader("miss")] 
 void AOMiss(inout AOHitInfo hit: SV_RayPayload)
{
    hit.distance = -1;
}