#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 2) uniform sampler2D DisplacementMap;

layout (location = 0) out vec2 OutTexcoord;

layout (quads, equal_spacing, cw) in;
void main()
{
	const float HeightScale = 1.0f;
	OutTexcoord = vec2(gl_TessCoord.x, 1.0f - gl_TessCoord.y);
	gl_Position = vec4(2.0f * gl_TessCoord.xy - 1.0f, HeightScale * (textureLod(DisplacementMap, OutTexcoord, 0.0f).r * 2.0f - 1.0f), 1.0f);
}

