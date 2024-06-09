struct IN
{
	float3 Position : POSITION;
}; 
struct TESS_FACTOR
{
	float Edge[4] : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};
struct OUT
{
	float4 Position : POSITION;
	float2 Texcoord : TEXCOORD0;
};

Texture2D DisplacementMap : register(t1, space0);
SamplerState Sampler : register(s1, space0);

struct WORLD_BUFFER
{
	float4x4 World[1];
};
ConstantBuffer<WORLD_BUFFER> WB : register(b1, space0);

[domain("quad")]
OUT main(const TESS_FACTOR tess, const float2 uv : SV_DomainLocation, const OutputPatch<IN, 4> quad)
{
	OUT Out;

	Out.Texcoord = float2(uv.x, 1.0f - uv.y);
	Out.Position = mul(WB.World[0], float4(2.0f * uv - 1.0f, DisplacementMap.SampleLevel(Sampler, Out.Texcoord, 0).r * 2.0f - 1.0f, 1.0f));

	return Out;
}
