#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 2) uniform sampler2D DisplacementMap;

layout (location = 0) out vec2 OutTexcoord;

layout (quads, equal_spacing, cw) in;
void main()
{
#if 1
	//!< Portrait
	const float X = 6.0f * 0.5f, Y = 8.0f * 0.5f;
#else
	//!< Standard
	const float X = 9.0f * 0.5f, Y = 5.0f * 0.5f;
#endif
	const mat4 World = mat4(X, 0.0f, 0.0f, 0.0f,
						0.0f, Y, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f);
	const float Displacement = 1.0f;

	OutTexcoord = vec2(gl_TessCoord.x, 1.0f - gl_TessCoord.y);
	gl_Position = World * vec4(2.0f * gl_TessCoord.xy - 1.0f, Displacement * (textureLod(DisplacementMap, OutTexcoord, 0.0f).r * 2.0f - 1.0f), 1.0f);
}

