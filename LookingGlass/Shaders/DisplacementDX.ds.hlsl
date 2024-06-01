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

[domain("quad")]
OUT main(const TESS_FACTOR tess, const float2 uv : SV_DomainLocation, const OutputPatch<IN, 4> quad)
{
	OUT Out;

#if 1
	//!< Portrait
	const float X = 6.0f * 0.65f, Y = 8.0f * 0.65f;
#else
	//!< Standard
	const float X = 9.0f * 0.65f, Y = 5.0f * 0.65f;
#endif
	const float4x4 World = float4x4(X, 0.0f, 0.0f, 0.0f,
		0.0f, Y, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	
	const float Displacement = 1.0f;

	Out.Texcoord = float2(uv.x, 1.0f - uv.y);
	Out.Position = mul(World, float4(2.0f * uv - 1.0f, Displacement * (DisplacementMap.SampleLevel(Sampler, Out.Texcoord, 0).r * 2.0f - 1.0f), 1.0f));

	return Out;
}
