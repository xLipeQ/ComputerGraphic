cbuffer cbProj : register(b0) //Geometry Shader constant buffer slot 0
{
	matrix projMatrix;
};

struct GSInput
{
	float4 pos : POSITION;
	float age : TEXCOORD0;
	float angle : TEXCOORD1;
	float size : TEXCOORD2;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex1: TEXCOORD0;
	float2 tex2: TEXCOORD1;
};

static const float TimeToLive = 4.0f;

[maxvertexcount(4)]
void main(point GSInput inArray[1], inout TriangleStream<PSInput> ostream)
{
	GSInput i = inArray[0];
	float sina, cosa;
	sincos(i.angle, sina, cosa);
	float dx = (cosa - sina) * 0.5 * i.size;
	float dy = (cosa + sina) * 0.5 * i.size;
	PSInput o = (PSInput)0;

	// TODO : 1.30 Initialize 4 vertices to make a bilboard and append them to the ostream
    PSInput ps[4];
	
	// prawy górny
    ps[0].pos = (inArray[0].pos.r + dx, inArray[0].pos.g + dy, inArray[0].pos.b, 1.f);
    ps[0].tex1 = (1, 0);
	// lewy dolny
    ps[1].pos = (inArray[0].pos.r - dx, inArray[0].pos.g - dy, inArray[0].pos.b, 1.f);
    ps[1].tex1 = (0, 1);
	
	// lewy gorny
    ps[2].pos = (inArray[0].pos.r - dy, inArray[0].pos.g + dx, inArray[0].pos.b, 1.f);
    ps[2].tex1 = (0, 0);
    
	// prawy dolny
	ps[3].pos = (inArray[0].pos.r + dy, inArray[0].pos.g - dx, inArray[0].pos.b, 1.f);
    ps[3].tex1 = (1, 1);
	
    ps[0].tex2 = ps[1].tex2 = ps[2].tex2 = ps[3].tex2 = (inArray[0].age / 4.f, 0.5);
    ostream.Append(ps[1]);
    ostream.Append(ps[2]);
    ostream.Append(ps[3]);
    ostream.Append(ps[0]);
	
	ostream.RestartStrip();
}