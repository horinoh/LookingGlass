struct IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	uint InstanceID : SV_InstanceID;
};

//struct WORLD_BUFFER
//{
//	float4x4 World[64];
//};
//ConstantBuffer<WORLD_BUFFER> WB : register(b0, space0);

struct OUT
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};

OUT main(IN In)
{
	OUT Out;

	Out.Position = In.Position;
	//Out.Position = mul(WB.World[In.InstanceID], In.Position);
	
	Out.Normal = In.Normal;
	//Out.Normal = mul(transpose(float3x3(WB.World[In.InstanceID])), In.Normal);

	return Out;
}
