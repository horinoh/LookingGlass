struct IN
{
	float3 Position : POSITION;
	float2 Texcoord : TEXCOORD0;
};

struct Transform { float4x4 Projection; float4x4 View; float4x4 World; };
ConstantBuffer<Transform> Tr : register(b0, space0);

struct OUT
{
	float4 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD0;
	float3 ViewDirection : TEXCOORD1;
};

[instance(1)]
[maxvertexcount(3)]
void main(const triangle IN In[3], inout TriangleStream<OUT> stream, uint instanceID : SV_GSInstanceID)
{
	OUT Out;

	const float3 CamPos = -float3(Tr.View[0][3], Tr.View[1][3], Tr.View[2][3]);
	const float4x4 PVW = mul(mul(Tr.Projection, Tr.View), Tr.World);

	[unroll]
	for (int i = 0; i < 3; ++i) {
		Out.Position = mul(PVW, float4(In[i].Position, 1.0f));
		Out.Texcoord = In[i].Texcoord;
		Out.ViewDirection = CamPos - mul(Tr.World, Out.Position).xyz;
		stream.Append(Out);
	}
	stream.RestartStrip();
}
