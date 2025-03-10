cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0 - matches slot in vsBilboard.hlsl
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1 - matches slot in vsBilboard.hlsl
{
	matrix viewMatrix;
	matrix invViewMatrix;
};

cbuffer cbProj : register(b2) //Vertex Shader constant buffer slot 2 - matches slot in vsBilboard.hlsl
{
	matrix projMatrix;
};

struct VSInput
{
	float3 pos : POSITION;
	float3 nor : NORMAL; 
};

struct PSInput
{
	float4 pos : SV_POSITION;
    float4 nor : NORMAL; 
    float3 worldPos : POSITION0; 
    float4 cam_pos : VIEW;
};
PSInput main(VSInput i)
{
	PSInput o;
	float4 pos = mul(worldMatrix, float4(i.pos, 1.0f));
    o.worldPos = pos.xyz; // Store world position
	
	pos = mul(viewMatrix, pos);
	o.pos = mul(projMatrix, pos);

    o.nor = mul(worldMatrix, float4(i.nor, 0.0f));
    o.cam_pos = mul(invViewMatrix, float4(0, 0, 0, 1));
	
	return o;
}