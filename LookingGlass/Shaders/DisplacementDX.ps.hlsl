struct IN
{
	float4 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD0;

	uint Viewport : SV_ViewportArrayIndex;
};
struct OUT
{
	float4 Color : SV_TARGET;
};

Texture2D ColorMap : register(t0, space0);
SamplerState Sampler : register(s0, space0);

[earlydepthstencil]
OUT main(const IN In)
{
	OUT Out;

	Out.Color = ColorMap.Sample(Sampler, In.Texcoord);
	//Out.Color = float4(In.Texcoord, 0, 1);

	return Out;
}

