struct IN
{
	float4 Position : SV_POSITION;
	float2 Texcoord : TEXCOORD0;
};

SamplerState Sampler : register(s0, space0);
Texture2D QuiltTexture : register(t0, space0);

struct BUFFER_PS
{
	float Pitch;
	float Tilt;
	float Center;
	int InvView;
	float Subp;
	float Aspect;
	int Ri
	int Bi;

	float3 QuiltTile;
};
ConstantBuffer<BUFFER_PS> BufferPS : register(b0, space0);

float2 texArr(const float2 uv, const float z)
{
	return float2(mod(z, BufferPS.QuiltTile.x) + uv.x, floor(z / BufferPS.QuiltTile.x) + uv.y) / BufferPS.QuiltTile.xy;
}

float4 main(IN In) : SV_TARGET
{
	clip (In.Texcoord);
	clip (1.0 - In.Texcoord);

	float4 RGB[3];
	//!< RGB のサブピクセルは正弦波の傾いたパターンで並べられている
	for (int i = 0; i < 3; ++i) {
		//!< [1, Column * Row] の値を取る、キルトインデックス
		float z = (In.Texcoord.x + i * BufferPS.Subp + In.Texcoord.y * BufferPS.Tilt) * BufferPS.Pitch - BufferPS.Center;
		z = fmod(z + ceil(abs(z)), 1.0);
		z *= HoloBuffer.QuiltTile.z;

		const float2 uv = float2(In.Texcoord.x, clamp(In.Texcoord.y, 0.005f, 0.995f));
		const float z1 = floor(z);
		const float4 Color1 = Texture.Sample(Sampler, TexArr(uv, z1));		//!< 1つ前にキルトインデックスパターン
		const float4 Color2 = Texture.Sample(Sampler, TexArr(uv, ceil(z))); //!< 1つ後のキルトインデックスパターン
		RGB[i] = lerp(Color1, Color2, z - z1); //!< 前後のキルトインデックスパターンを補完
	}
	return float4(RGB[BufferPS.Ri].r, RGB[1].g, RGB[BufferPS.Bi].b, 1.0f);
}