struct IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};

struct VIEW_PROJECTION_BUFFER
{
	float4x4 ViewProjection[64];
};
ConstantBuffer<VIEW_PROJECTION_BUFFER> VPB : register(b0, space0);

struct OUT
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	//float3 ViewDirection : TEXCOORD0;

	uint Viewport : SV_ViewportArrayIndex; 
};

[instance(16)]
[maxvertexcount(3)]
void main(const triangle IN In[3], inout TriangleStream<OUT> stream, uint instanceID : SV_GSInstanceID)
{
	OUT Out;

	//const float3 CamPos = -float3(View[0][3], View[1][3], View[2][3]);

	[unroll]
	for (int i = 0; i < 3; ++i) {
		Out.Position = mul(VPB.ViewProjection[instanceID], float4(In[i].Position, 1.0f));

		Out.Normal = normalize(In[i].Normal);

		//Out.ViewDirection = CamPos - In[i].Position;

		Out.Viewport = instanceID;

		stream.Append(Out);
	}
	stream.RestartStrip();
}
