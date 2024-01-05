struct IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};

struct OUT
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};

OUT main(IN In)
{
	OUT Out;

	Out.Position = In.Position;
	Out.Normal = In.Normal;

	return Out;
}
