struct IN
{
	float3 Position : POSITION;
	float2 Texcoord : TEXCOORD0;
};

struct VIEW_PROJECTION_BUFFER
{
	float4x4 ViewProjection[64];
};
ConstantBuffer<VIEW_PROJECTION_BUFFER> VPB : register(b0, space0);

struct OUT
{
	float4 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD0;

	uint Viewport : SV_ViewportArrayIndex;
};

[instance(16)]
[maxvertexcount(3)]
void main(const triangle IN In[3], inout TriangleStream<OUT> stream, uint instanceID : SV_GSInstanceID)
{
	OUT Out;

	const float4x4 W = float4x4(4, 0, 0, 0,
								0, 4, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1);

	[unroll]
	for (int i = 0; i < 3; ++i) {
		//Out.Position = mul(VPB.ViewProjection[instanceID], float4(In[i].Position, 1.0f));
		Out.Position = mul(mul(VPB.ViewProjection[instanceID], W), float4(In[i].Position, 1.0f));

		Out.Texcoord = In[i].Texcoord;
		Out.Viewport = instanceID;

		stream.Append(Out);
	}
	stream.RestartStrip();
}
