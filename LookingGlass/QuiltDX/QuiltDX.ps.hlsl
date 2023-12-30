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
	float ColRow;
	float QuiltAspect;
};
#if 0
ConstantBuffer<LENTICULAR_BUFFER> LB : register(b0, space0);
#else
static const LENTICULAR_BUFFER LB = {
	246.866f,
	-0.185377f,
	0.565845f,
	0.000217014f,

	0.75f,
	1,
	0,
	2,

	8,
	6,
	48,
	0.75f
};
#endif

//!< uvz.xy	: パターン上での UV 値
//!< uvz.z	: キルトグリッド上のどのパターンか
//!< パターン番号とパターン上での UV から絶対 UV 値を求める
float2 ToTexcoord(float3 uvz)
{
	//const float2 ViewPortion = float2(0.999755859f, 0.999755859f);
	//return (float2(fmod(uvz.z, LB.Column), floor(uvz.z / LB.Column)) + uvz.xy) / float2(LB.Column, LB.Row) * ViewPortion;
	return (float2(fmod(uvz.z, LB.Column), floor(uvz.z / LB.Column)) + uvz.xy) / float2(LB.Column, LB.Row);
}

float4 main(IN In) : SV_TARGET
{
#if 0
	//return Texture.Sample(Sampler, In.Texcoord);
	const int OverScan = 0;
	const float modx = clamp(step(LB.QuiltAspect, LB.DisplayAspect) * step(float(OverScan), 0.5f) + step(LB.DisplayAspect, LB.QuiltAspect) * step(0.5f, float(OverScan)), 0.0f, 1.0f);
	return float4(modx, modx, modx, 1);
#else
	float2 nuv = In.Texcoord;

	//!< キルトとディスプレイのアスペクト比が異なる場合に必要
	{
		//!< 「キルトとデバイスのアスペクト比が異なる場合」のスケーリング設定
		//!<	0 (デフォルト)	: 余白は黒で埋めてスケーリング
		//!<	1				: 余白を埋めるようにスケーリング
		const int OverScan = 0;
		nuv -= 0.5;
		const float modx = clamp(step(LB.QuiltAspect, LB.DisplayAspect) * step(float(OverScan), 0.5f) + step(LB.DisplayAspect, LB.QuiltAspect) * step(0.5f, float(OverScan)), 0.0f, 1.0f);
		nuv *= float2(LB.DisplayAspect / LB.QuiltAspect * modx + (1.0f - modx), LB.QuiltAspect / LB.DisplayAspect * (1.0f - modx) + modx);
		nuv += 0.5;
	}

	clip(nuv);
	clip(1 - nuv);

	//!< RGB サブピクセルは正弦波パターンに並んでいて、全体的に斜めになっている
	float3 RGB[3];
	for (int i = 0; i < 3; ++i) {
		//!< キルトグリッド上のどのパターンか
		float Z = (In.Texcoord.x + i * LB.Subp + In.Texcoord.y * LB.Tilt) * LB.Pitch - LB.Center;
		Z = fmod(Z + ceil(abs(Z)), 1.0) * LB.InvView * LB.ColRow;

		//!< 前後のパターンから補完する
		const float Y = clamp(nuv.y, 0.005f, 0.995f);
		const float3 Coords1 = float3(nuv.x, Y, floor(Z));
		const float3 Coords2 = float3(nuv.x, Y, ceil(Z));
		const float3 Color1 = Texture.Sample(Sampler, ToTexcoord(Coords1)).rgb;
		const float3 Color2 = Texture.Sample(Sampler, ToTexcoord(Coords2)).rgb;
		RGB[i] = lerp(Color1, Color2, Z - Coords1.z);
	}
	return float4(RGB[LB.Ri].r, RGB[1].g, RGB[LB.Bi].b, 1.0);
#endif
}
