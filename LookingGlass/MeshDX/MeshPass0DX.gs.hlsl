struct IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};
struct OUT
{
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	uint Viewport : SV_ViewportArrayIndex; 
	//uint RenderTarget : SV_RenderTargetArrayIndex; 
};

[instance(4)]
[maxvertexcount(3)]
void main(const triangle IN In[3], inout LineStream<OUT> stream, uint instanceID : SV_GSInstanceID)
{
	OUT Out;
	
	[unroll]
	for (int i = 0; i<3; ++i) {
		Out.Position = float4(In[i].Position, 1.0f);
		Out.Normal = In[i].Normal;
		Out.Viewport = instanceID; //!< GSインスタンシング(ビューポート毎)
		//Out.RenderTarget = instanceID; //!< インスタンシング(レンダーターゲット毎)
		stream.Append(Out);
	}
	stream.RestartStrip();
}
