struct IN
{
	float4 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD0;
};

Texture2D<float4> Texture : register(t0, space0);
SamplerState Sampler : register(s0, space0);

struct LENTICULAR_BUFFER
{
	float Pitch;
	float Tilt;
	float Center;
	float Subp;

	float DisplayAspect;
	int InvView;
	int Ri;
	int Bi;

	float Column;
	float Row;
	float ColRow;
};
ConstantBuffer<LENTICULAR_BUFFER> LenticularBuffer : register(b0, space0);

float4 main(IN In) : SV_TARGET
{
	return Texture.Sample(Sampler, In.Texcoord);
}
