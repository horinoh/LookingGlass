#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 InNormal[];

layout (set = 0, binding = 1) uniform ViewProjectionBuffer
{
	mat4 ViewProjection[64];
} VPB;

layout (location = 0) out vec3 OutNormal;
//layout (location = 1) out vec3 OutViewDirection;

layout (triangles, invocations = 16) in;
layout (triangle_strip, max_vertices = 3) out;
void main()
{	
	//const vec3 CamPos = vec3(View[3][0], View[3][1], View[3][2]);

	for(int i=0;i<gl_in.length();++i) {
		gl_Position = VPB.ViewProjection[gl_InvocationID] * gl_in[i].gl_Position;

		OutNormal = normalize(InNormal[i]);
		//OutViewDirection = CamPos - gl_in[i].gl_Position.xyz;

		gl_ViewportIndex = gl_InvocationID;

		EmitVertex();
	}
	EndPrimitive();	
}
