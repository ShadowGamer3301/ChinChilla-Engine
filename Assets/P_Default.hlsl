struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_Target
{
    return float4(1.0f, 1.0f, 0.0f, 1.0f);
}