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

	int TileX, TileY;

	float QuiltAspect;
} LB;

//!< CoordZ : �L���g�O���b�h��̃p�^�[���ԍ�(Z)�A�p�^�[����ł̃e�N�X�`�����W(XY)������
//!< ��΃e�N�X�`�����W�֕ϊ�����
vec2 ToTexcoord(vec3 CoordZ)
{
	return (vec2(mod(CoordZ.z, LB.TileX), -sign(LB.Tilt) * floor(CoordZ.z / LB.TileX)) + CoordZ.xy) / vec2(LB.TileX, LB.TileY);
}

void main()
{
#if 0
	OutColor = texture(Sampler2D, InTexcoord);
#else
	vec2 UV = InTexcoord;

	//!< �L���g�ƃf�B�X�v���C�̃A�X�y�N�g�䂪�قȂ�ꍇ�ɕK�v
	{
		//!< �u�L���g�ƃf�o�C�X�̃A�X�y�N�g�䂪�قȂ�ꍇ�v�̃X�P�[�����O�ݒ�
		//!<	0 (�f�t�H���g)	: �]���͍��Ŗ��߂ăX�P�[�����O
		//!<	1				: �]���𖄂߂�悤�ɃX�P�[�����O
		const int OverScan = 0;
		UV -= 0.5f;
		const float modx = clamp(step(LB.QuiltAspect, LB.DisplayAspect) * step(float(OverScan), 0.5f) + step(LB.DisplayAspect, LB.QuiltAspect) * step(0.5f, float(OverScan)), 0.0f, 1.0f);
		UV *= vec2(LB.DisplayAspect / LB.QuiltAspect * modx + (1.0f - modx), LB.QuiltAspect / LB.DisplayAspect * (1.0f - modx) + modx);
		UV += 0.5f;
	}

	if(any(lessThan(UV, vec2(0.0f)))) discard;
	if(any(lessThan(1.0f - UV, vec2(0.0f)))) discard;

	//!< RGB �T�u�s�N�Z���͐����g�p�^�[���ɕ���ł��āA�S�̓I�Ɏ΂߂ɂȂ��Ă���
	vec3 RGB[3];
	const float XY = LB.TileX * LB.TileY;
	for(int i = 0;i < 3;++i) {
		//!< �L���g�O���b�h��̂ǂ̃p�^�[����
		float Z = (InTexcoord.x + i * LB.Subp + InTexcoord.y * LB.Tilt) * LB.Pitch - LB.Center;
		Z = mod(Z + ceil(abs(Z)), 1.0f) * LB.InvView * XY;

		//!< �O��̃p�^�[������⊮����
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