#define TILE_DIMENSION_MAX (16 * 5)

struct IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};

struct VIEW_PROJECTION 
{
	float4x4 View;
	float4x4 Projection;
};
struct VIEW_PROJECTION_BUFFER
{
	VIEW_PROJECTION ViewProjection[TILE_DIMENSION_MAX];
};
ConstantBuffer<VIEW_PROJECTION_BUFFER> VPB : register(b0, space0);

struct OUT
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float3 ViewDirection : TEXCOORD0;

	uint Viewport : SV_ViewportArrayIndex; 
};

[instance(16)]
[maxvertexcount(3)]
void main(const triangle IN In[3], inout TriangleStream<OUT> stream, uint instanceID : SV_GSInstanceID)
{
	OUT Out;

	const float3 CamPos = -float3(VPB.ViewProjection[instanceID].View[0][3], VPB.ViewProjection[instanceID].View[1][3], VPB.ViewProjection[instanceID].View[2][3]);

	[unroll]
	for (int i = 0; i < 3; ++i) {
		Out.Position = mul(mul(VPB.ViewProjection[instanceID].Projection, VPB.ViewProjection[instanceID].View), float4(In[i].Position, 1.0f));

		Out.Normal = normalize(In[i].Normal);

		Out.ViewDirection = CamPos - In[i].Position;

		Out.Viewport = instanceID;

		stream.Append(Out);
	}
	stream.RestartStrip();
}
