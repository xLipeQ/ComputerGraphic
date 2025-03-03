//float4 main() : SV_TARGET
//{
//    return float4(1.0f, 0.5f, 0.5f, 1.0f);
//}

struct VSOUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

float4 main(VSOUT i) : SV_TARGET
{
    return i.col;
}