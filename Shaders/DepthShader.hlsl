struct CascadeData
{
    row_major matrix model;
    row_major matrix ViewProj[4]; //
};
cbuffer CascBuf : register(b0)
{
    CascadeData CascData;
};

struct VS_IN
{
    float3 pos : POSITION0;
};

struct GS_IN
{
    float4 pos : POSITION;
};

GS_IN VSMain(VS_IN input)
{
    GS_IN output = (GS_IN) 0;
    output.pos = mul(float4(input.pos.xyz, 1.0f), CascData.model);
    return output;
}

struct GS_OUT
{
    float4 pos : SV_POSITION;
    uint arrInd : SV_RenderTargetArrayIndex;
};

[instance(4)]
[maxvertexcount(3)]
void GSMain(triangle GS_IN p[3], in uint id : SV_GSInstanceID, inout TriangleStream<GS_OUT> stream)
{
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        GS_OUT gs = (GS_OUT) 0;
        gs.pos = mul(float4(p[i].pos.xyz, 1.0f), CascData.ViewProj[id]);
        gs.arrInd = id;
        stream.Append(gs);
    }
}