struct IN
{
	float4 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD0;
};

Texture2D<float4> Texture : register(t0, space0);
SamplerState Sampler : register(s0, space0);

struct LENTICULAR_BUFFER
{
	float Pitch;
	float Tilt;
	float Center;
	float Subp;

	float DisplayAspect;
	int InvView;
	int Ri;
	int Bi;

	float Column;
	float Row;

	float QuiltAspect;
};
ConstantBuffer<LENTICULAR_BUFFER> LB : register(b0, space0);

float2 ToTexcoord(float3 CoordZ)
{
	return (float2(fmod(CoordZ.z, LB.Column), -sign(LB.Tilt) * floor(CoordZ.z / LB.Column)) + CoordZ.xy) / float2(LB.Column, LB.Row);
}

float4 main(IN In) : SV_TARGET
{
#if 0
	return Texture.Sample(Sampler, In.Texcoord);
#else
	float2 UV = In.Texcoord;

	//!< 自前で描画しているので、キルトとディスプレイのアスペクト比調整は不要

	clip(UV);
	clip(1 - UV);

	float3 RGB[3];
	const float ColRow = LB.Column * LB.Row;
	for (int i = 0; i < 3; ++i) {
		float Z = (In.Texcoord.x + i * LB.Subp + In.Texcoord.y * LB.Tilt) * LB.Pitch - LB.Center;
		Z = fmod(Z + ceil(abs(Z)), 1.0f) * LB.InvView * ColRow;

		const float Y = clamp(UV.y, 0.005f, 0.995f);
		const float3 CoordZ1 = float3(UV.x, Y, floor(Z));
		const float3 CoordZ2 = float3(UV.x, Y, ceil(Z));
		const float3 Color1 = Texture.Sample(Sampler, ToTexcoord(CoordZ1)).rgb;
		const float3 Color2 = Texture.Sample(Sampler, ToTexcoord(CoordZ2)).rgb;
		RGB[i] = lerp(Color1, Color2, Z - CoordZ1.z);
	}
	return float4(RGB[LB.Ri].r, RGB[1].g, RGB[LB.Bi].b, 1.0);
#endif
}
