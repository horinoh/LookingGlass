#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 InPosition;
layout (location = 1) in vec3 InNormal;

layout (set = 0, binding = 0) uniform WorldBuffer
{
	mat4 World[16];
} WB;

layout (location = 0) out vec3 OutNormal;

void main()
{
	gl_Position = WB.World[gl_InstanceIndex] * vec4(InPosition, 1.0f);

	OutNormal = transpose(mat3(WB.World[gl_InstanceIndex])) * InNormal;
}
