cbuffer constants : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    row_major float4x4 rotation;
    row_major float4x4 scale;
    float4 color;
};

struct VS_Input
{
	float3 position: POSITION;
};

struct VS_Output
{
	float4 position: SV_POSITION;
	float4 color: COLOR;
};

VS_Output vs_main(VS_Input input)
{
	VS_Output output;
	output.position = mul(float4(input.position, 1.0f), mul(mul(mul(mul(scale, rotation), model), view), projection));
	output.color = color;
	return output;
};

float4 ps_main(VS_Output input): SV_TARGET
{
	return input.color;
};