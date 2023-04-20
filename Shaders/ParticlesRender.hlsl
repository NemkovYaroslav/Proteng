struct Particle
{
    float4 Position;
    float4 Velocity;
    float4 Color0;
    float2 Size0Size1;
    float  LifeTime;
};

struct VSOutput
{
    int vertexID : TEXCOORD0;
};

struct GSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 Tex      : TEXCOORD0;
};

struct ConstParams
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
    float4   DeltaTimeMaxParticlesGroupdim;
};

cbuffer CB1 : register(b0)
{
    ConstParams Params;
}

StructuredBuffer<Particle>        renderBufSrc    : register(t0);
ConsumeStructuredBuffer<Particle> particlesBufSrc : register(u0);
AppendStructuredBuffer<Particle>  particlesBufDst : register(u1);

VSOutput VSMain(uint vertexID : SV_VectexID)
{
    VSOutput output;
    output.vertexID = vertexID;
    return output;
}

[maxvertexcount(4)]
void GSMain(point VSOutput inputPoint[1], inout TriangleStream<GSOutput> outputStream)
{
    GSOutput p0, p1, p2, p3;
    
    Particle prt = renderBufSrc[inputPoint[0].vertexID];
    
    float sz = prt.Size0Size1.x;
    float4 color = prt.Color0; 
    
    float4 mvPos = prt.Position;
    mvPos = mul(float4(mvPos.xyz, 1), Params.World);
    mvPos = mul(float4(mvPos.xyz, 1), Params.View);
    mvPos = float4(mvPos.xyz, 1.0f);
    
    p0.Position = mul(mvPos + float4(sz, sz, 0, 0), Params.Projection);
    p0.Tex = float2(1, 1);
    p0.Color = color;
    
    p1.Position = mul(mvPos + float4(-sz, sz, 0, 0), Params.Projection);
    p1.Tex = float2(0, 1);
    p1.Color = color;
    
    p2.Position = mul(mvPos + float4(-sz, -sz, 0, 0), Params.Projection);
    p2.Tex = float2(0, 0);
    p2.Color = color;
    
    p3.Position = mul(mvPos + float4(sz, -sz, 0, 0), Params.Projection);
    p3.Tex = float2(1, 0);
    p3.Color = color;
    
    outputStream.Append(p1);
    outputStream.Append(p0);
    outputStream.Append(p2);
    outputStream.Append(p3);
}


float4 PSMain(GSOutput input) : SV_Target0
{
    float amount = length(input.Tex - float2(0.5f, 0.5f)) * 2.0f;
    
    amount = smoothstep(0.0f, 1.0f, 1.0f - amount);

    return float4(input.Color.rgb, amount);
}

#define BLOCK_SIZE 256
#define THREAD_IN_GROUP_TOTAL 256

[numthreads(BLOCK_SIZE, 1, 1)]
void CSMain
(
    uint3 groupID          : SV_GroupID,
    uint3 groupThreadID    : SV_GroupThreadID,
    uint3 dispatchThreadID : SV_DispatchThreadID,
    uint  groupIndex       : SV_GroupIndex
)
{
    uint id = groupID.x * THREAD_IN_GROUP_TOTAL * groupID.y * Params.DeltaTimeMaxParticlesGroupdim.z * THREAD_IN_GROUP_TOTAL;
    
    [flatten]
    if (id >= (uint) Params.DeltaTimeMaxParticlesGroupdim.y)
    {
        return;
    }

#ifdef INJECTION
    Particle p  = particlesBufSrc.Consume();
    if (p.LifeTime > 0)
    {
        particlesBufDst.Append(p);
    }   
#endif
    
#ifdef SIMULATION
    Particle p = particlesBufSrc.Consume();   
    p.LifeTime -= Params.DeltaTimeMaxParticlesGroupdim.x;    
    if (p.LifeTime > 0)
    {
    #ifdef ADD_GRAVITY  
        p.Velocity += float4(0, -9.8f * Params.DeltaTimeMaxParticlesGroupdim.x, 0, 0);   
    #endif           
    }  
    p.Position.xyz += p.Velocity * Params.DeltaTimeMaxParticlesGroupdim.x;  
    particlesBufDst.Append(p);
#endif
}