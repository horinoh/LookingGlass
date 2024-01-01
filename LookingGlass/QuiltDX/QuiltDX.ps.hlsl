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

//!< CoordZ : キルトグリッド上のパターン番号(Z)、パターン上でのテクスチャ座標(XY)を持つ
//!< 絶対テクスチャ座標へ変換する
float2 ToTexcoord(float3 CoordZ)
{
	//const float2 ViewPortion = float2(0.999755859f, 0.999755859f);
	//return (float2(fmod(CoordZ.z, LB.Column), -sign(LB.Tilt) * floor(CoordZ.z / LB.Column)) + CoordZ.xy) / float2(LB.Column, LB.Row) * ViewPortion;
	return (float2(fmod(CoordZ.z, LB.Column), -sign(LB.Tilt) * floor(CoordZ.z / LB.Column)) + CoordZ.xy) / float2(LB.Column, LB.Row);
}

float4 main(IN In) : SV_TARGET
{
#if 0
	return Texture.Sample(Sampler, In.Texcoord);
#else
	float2 UV = In.Texcoord;

	//!< キルトとディスプレイのアスペクト比が異なる場合に必要
	{
		//!< 「キルトとデバイスのアスペクト比が異なる場合」のスケーリング設定
		//!<	0 (デフォルト)	: 余白は黒で埋めてスケーリング
		//!<	1				: 余白を埋めるようにスケーリング
		const int OverScan = 0;
		UV -= 0.5;
		const float modx = clamp(step(LB.QuiltAspect, LB.DisplayAspect) * step(float(OverScan), 0.5f) + step(LB.DisplayAspect, LB.QuiltAspect) * step(0.5f, float(OverScan)), 0.0f, 1.0f);
		UV *= float2(LB.DisplayAspect / LB.QuiltAspect * modx + (1.0f - modx), LB.QuiltAspect / LB.DisplayAspect * (1.0f - modx) + modx);
		UV += 0.5;
	}

	clip(UV);
	clip(1 - UV);

	//!< RGB サブピクセルは正弦波パターンに並んでいて、全体的に斜めになっている
	float3 RGB[3];
	const float ColRow = LB.Column * LB.Row;
	for (int i = 0; i < 3; ++i) {
		//!< キルトグリッド上のどのパターンか
		float Z = (In.Texcoord.x + i * LB.Subp + In.Texcoord.y * LB.Tilt) * LB.Pitch - LB.Center;
		Z = fmod(Z + ceil(abs(Z)), 1.0f) * LB.InvView * ColRow;

		//!< 前後のパターンから補完する
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
