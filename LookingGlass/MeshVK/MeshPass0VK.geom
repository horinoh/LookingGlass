#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 InNormal[];

layout (set = 0, binding = 0) uniform ViewProjectionBuffer
{
	mat4 ViewProjection[64];
} VPB;

layout (location = 0) out vec3 OutNormal;

layout (triangles, invocations = 16) in;
layout (line_strip, max_vertices = 3) out;
void main()
{	
	const float Scale = 5.0f;
	//const float Scale = gl_InvocationID + ViewportOffset;
	const mat4 World = mat4(Scale, 0, 0, 0, 0, Scale, 0, 0, 0, 0, Scale, 0, 0, 0, 0, 1);
	
	for(int i=0;i<gl_in.length();++i) {
		gl_Position = VPB.ViewProjection[gl_InvocationID + 0] * World * gl_in[i].gl_Position;

		OutNormal = normalize(InNormal[i]);
		//OutNormal = normalize(WIT * InNormal[i]);
	
		gl_ViewportIndex = gl_InvocationID; //!< GSインスタンシング(ビューポート毎)

		EmitVertex();
	}
	EndPrimitive();	
}
