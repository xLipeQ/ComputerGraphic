struct Light
{
	float4 position;
	float4 color;
};

struct Lighting
{
	float4 ambient;
	float4 surface;
	Light lights[3];
};

cbuffer cbSurfaceColor : register(b0) //Pixel Shader constant buffer slot 0 - matches slot in psBilboard.hlsl
{
	float4 surfaceColor;
}

cbuffer cbLighting : register(b1) //Pixel Shader constant buffer slot 1
{
	Lighting lighting;
}

//TODO : 0.8. Modify pixel shader input structure to match vertex shader output
struct PSInput
{
	float4 pos : SV_POSITION;
	float4 nor : NORMAL;
    float3 worldPos : POSITION0;
    float4 cam_pos : VIEW;
};

float4 main(PSInput input) : SV_TARGET
{
	//TODO : 0.9. Calculate output color using Phong Illumination Model
    float3 N = normalize(input.nor); // Normalized normal
    float3 V = normalize(input.cam_pos.xyz - input.worldPos); // View direction
    float4 ambient = lighting.ambient * lighting.surface.r * surfaceColor;

    float4 diffuse = float4(0, 0, 0, 0);
    float4 specular = float4(0, 0, 0, 0);

    for (int i = 0; i < 3; i++)
    {
        float3 L = normalize(lighting.lights[i].position.xyz - input.worldPos.xyz); // Light direction
        float3 R = reflect(-L, N); // Reflected light vector

        float NdotL = max(dot(N, L), 0.0);
        float RdotV = pow(max(dot(R, V), 0.0), lighting.surface.a); // Specular exponent in .a

        // Compute diffuse and specular components
        diffuse += lighting.lights[i].color * lighting.surface.g * surfaceColor * NdotL;
        specular += lighting.lights[i].color * lighting.surface.b * RdotV;
    }

    return ambient + diffuse + specular;
}