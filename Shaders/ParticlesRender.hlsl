struct VS_IN
{
    float4 pos : POSITION0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output = (PS_IN) 0;
    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{  
    return float4(1.0f, 0.0f, 0.0f, 0.0f);
}