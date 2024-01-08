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

struct RootConstant
{
	uint ViewportOffset;
};
ConstantBuffer<RootConstant> RC : register(b1, space0);

struct OUT
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;

	uint Viewport : SV_ViewportArrayIndex; 
};

[instance(16)]
[maxvertexcount(3)]
void main(const triangle IN In[3], inout LineStream<OUT> stream, uint instanceID : SV_GSInstanceID)
{
	OUT Out;

	const float Scale = 5.0f;
	//const float Scale = instanceID + RC.ViewportOffset;
	const float4x4 World = float4x4(Scale, 0, 0, 0, 0, Scale, 0, 0, 0, 0, Scale, 0, 0, 0, 0, 1);
	
	[unroll]
	for (int i = 0; i<3; ++i) {
		Out.Position = mul(mul(VPB.ViewProjection[instanceID + RC.ViewportOffset], World), float4(In[i].Position, 1.0f));

		Out.Normal = normalize(In[i].Normal);
		//Out.Normal = normalize(mul(WIT, In[i].Normal));

		Out.Viewport = instanceID; //!< GSインスタンシング(ビューポート毎)

		stream.Append(Out);
	}
	stream.RestartStrip();
}
