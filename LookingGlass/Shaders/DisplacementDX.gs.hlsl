#define TILE_DIMENSION_MAX (16 * 5)

struct IN
{
	float4 Position : POSITION;
	float2 Texcoord : TEXCOORD0;
};

struct VIEW_PROJECTION_BUFFER
{
	float4x4 ViewProjection[TILE_DIMENSION_MAX];
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
//void main(const triangle IN In[3], inout LineStream<OUT> stream, uint instanceID : SV_GSInstanceID)
//void main(const triangle IN In[3], inout PointStream<OUT> stream, uint instanceID : SV_GSInstanceID)
{
	OUT Out;

	[unroll]
	for (int i = 0; i < 3; ++i) {
		Out.Position = mul(VPB.ViewProjection[instanceID], In[i].Position);

		Out.Texcoord = In[i].Texcoord;
		Out.Viewport = instanceID;

		stream.Append(Out);
	}
	stream.RestartStrip();
}
