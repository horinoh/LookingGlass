#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (early_fragment_tests) in;

layout (location = 0) in vec2 InTexcoord;

layout (location = 0) out vec4 OutColor;

layout (set = 0, binding = 0) uniform sampler2D Sampler2D;

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

	float QuiltAspect;
} LB;

vec2 ToTexcoord(vec3 CoordZ)
{
	return (vec2(mod(CoordZ.z, LB.Column), -sign(LB.Tilt) * floor(CoordZ.z / LB.Column)) + CoordZ.xy) / vec2(LB.Column, LB.Row);
}

void main()
{
#if 1
	OutColor = texture(Sampler2D, InTexcoord);
#else
	vec2 UV = InTexcoord;

	//!< 自前で描画しているので、キルトとディスプレイのアスペクト比調整は不要

	if(any(lessThan(UV, vec2(0)))) discard;
	if(any(lessThan(1 - UV, vec2(0)))) discard;

	vec3 RGB[3];
	const float ColRow = LB.Column * LB.Row;
	for(int i = 0;i < 3;++i) {
		float Z = (InTexcoord.x + i * LB.Subp + InTexcoord.y * LB.Tilt) * LB.Pitch - LB.Center;
		Z = mod(Z + ceil(abs(Z)), 1.0f) * LB.InvView * ColRow;

		const float Y = clamp(UV.y, 0.005f, 0.995f);
		const vec3 CoordZ1 = vec3(UV.x, Y, floor(Z));
		const vec3 CoordZ2 = vec3(UV.x, Y, ceil(Z));
		const vec3 Color1 = texture(Sampler2D, ToTexcoord(CoordZ1)).rgb;
		const vec3 Color2 = texture(Sampler2D, ToTexcoord(CoordZ2)).rgb;
		RGB[i] = mix(Color1, Color2, Z - CoordZ1.z);
	}
	OutColor = vec4(RGB[LB.Ri].r, RGB[1].g, RGB[LB.Bi].b, 1.0);
#endif
}