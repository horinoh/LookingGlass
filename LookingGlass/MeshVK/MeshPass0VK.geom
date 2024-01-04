#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 InNormal[];

layout (location = 0) out vec3 OutNormal;

layout (triangles, invocations = 4) in;
layout (line_strip, max_vertices = 3) out;
void main()
{
	for(int i=0;i<gl_in.length();++i) {
		gl_Position = gl_in[i].gl_Position;
		OutNormal = InNormal[i];
		gl_ViewportIndex = gl_InvocationID; //!< GSインスタンシング(ビューポート毎)
		//gl_Layer = gl_InvocationID; //!< GSインスタンシング(レンダーターゲット毎)
		EmitVertex();
	}
	EndPrimitive();	
}
