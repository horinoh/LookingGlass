struct IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	uint InstanceID : SV_InstanceID;
};

struct WORLD_BUFFER
{
	float4x4 World[16];
};
ConstantBuffer<WORLD_BUFFER> WB : register(b0, space0);

struct OUT
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};

OUT main(IN In)
{
	OUT Out;

#if 0
	Out.Position = In.Position;
	Out.Normal = In.Normal;
#else
	Out.Position = mul(WB.World[In.InstanceID], float4(In.Position, 1.0f)).xyz;
	Out.Normal = mul(transpose((float3x3)WB.World[In.InstanceID]), In.Normal);
#endif

	return Out;
}
