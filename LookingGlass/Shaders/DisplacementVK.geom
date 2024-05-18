#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 InTexcoord[];

layout (set = 0, binding = 0) uniform VIEW_PROJECTION_BUFFER
{
	mat4 ViewProjection[64];
} VPB;

layout (location = 0) out vec2 OutTexcoord;

layout (triangles, invocations = 16) in;
layout (triangle_strip, max_vertices = 3) out;
void main()
{
	const mat4 W = mat4(3.5f, 0.0f, 0.0f, 0.0f,
						0.0f, 3.5f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f);

	for(int i=0;i<gl_in.length();++i) {
		gl_Position = VPB.ViewProjection[gl_InvocationID] * W * gl_in[i].gl_Position;	
		OutTexcoord = InTexcoord[i];
		gl_ViewportIndex = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();	
}
