#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (early_fragment_tests) in;

layout (location = 0) in vec2 InTexcoord;

layout (location = 0) out vec4 OutColor;

layout (set = 0, binding = 0) uniform sampler2D Sampler2D;

#if 0
layout (set = 0, binding = 1) uniform LenticularBuffer
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
#else
const float Pitch=246.866f;
const float Tilt=-0.185377f;
const float Center=0.565845f;
const float Subp=0.000217014f;

const float DisplayAspect=0.75f;
const int InvView=1;
const int Ri=0;
const int Bi=2;

const float Column=8;
const float Row=6;
const float ColRow=48;
const float QuiltAspect=0.75f;
#endif

//!< uvz.xy	: パターン上での UV 値
//!< uvz.z	: キルトグリッド上のどのパターンか
//!< パターン番号とパターン上での UV から絶対 UV 値を求める
vec2 ToTexcoord(vec3 uvz)
{
	//const vec2 ViewPortion = vec2(0.999755859f, 0.999755859f);
	//return (vec2(mod(uvz.z, Column), floor(uvz.z / Column)) + uvz.xy) / vec2(Column, Row) * ViewPortion;
	return (vec2(mod(uvz.z, Column), floor(uvz.z / Column)) + uvz.xy) / vec2(Column, Row);
}

void main()
{
#if 0
	OutColor = texture(Sampler2D, InTexcoord);
	const int OverScan = 0;
	const float modx = clamp(step(QuiltAspect, DisplayAspect) * step(float(OverScan), 0.5f) + step(DisplayAspect, QuiltAspect) * step(0.5f, float(OverScan)), 0.0f, 1.0f);
	OutColor = vec4(modx, modx, modx, 1);
#else
	vec2 nuv = InTexcoord;

	//!< キルトとディスプレイのアスペクト比が異なる場合に必要
	{
		//!< 「キルトとデバイスのアスペクト比が異なる場合」のスケーリング設定
		//!<	0 (デフォルト)	: 余白は黒で埋めてスケーリング
		//!<	1				: 余白を埋めるようにスケーリング
		const int OverScan = 0;
		nuv -= 0.5;
		const float modx = clamp(step(QuiltAspect, DisplayAspect) * step(float(OverScan), 0.5f) + step(DisplayAspect, QuiltAspect) * step(0.5f, float(OverScan)), 0.0f, 1.0f);
		nuv *= vec2(DisplayAspect / QuiltAspect * modx + (1.0f - modx), QuiltAspect / DisplayAspect * (1.0f - modx) + modx);
		nuv += 0.5;
	}

	if(any(lessThan(nuv, vec2(0)))) discard;
	if(any(lessThan(1 - nuv, vec2(0)))) discard;

	//!< RGB サブピクセルは正弦波パターンに並んでいて、全体的に斜めになっている
	vec3 RGB[3];
	for(int i = 0;i < 3;++i) {
		//!< キルトグリッド上のどのパターンか
		float Z = (InTexcoord.x + i * Subp + InTexcoord.y * Tilt) * Pitch - Center;
		Z = mod(Z + ceil(abs(Z)), 1.0) * InvView * ColRow;

		//!< 前後のパターンから補完する
		const float Y = clamp(nuv.y, 0.005f, 0.995f);
		const vec3 Coords1 = vec3(nuv.x, Y, floor(Z));
		const vec3 Coords2 = vec3(nuv.x, Y, ceil(Z));
		const vec3 Color1 = texture(Sampler2D, ToTexcoord(Coords1)).rgb;
		const vec3 Color2 = texture(Sampler2D, ToTexcoord(Coords2)).rgb;
		RGB[i] = mix(Color1, Color2, Z - Coords1.z);
	}
	OutColor = vec4(RGB[Ri].r, RGB[1].g, RGB[Bi].b, 1.0);
#endif
}