struct Particle
{
    float4 Position;
    float4 Velocity;
    float4 Color0;
    float2 Size0Size1;
    float  LifeTime;
};

struct ConstParams
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
    float4   DeltaTimeMaxParticlesGroupdim;
    float4   CameraPosX;
};

cbuffer CB1 : register(b0)
{
    ConstParams Params;
}

RWStructuredBuffer<Particle> particlesPool : register(u0);

#ifdef INJECTION
AppendStructuredBuffer<float2>    sortedParticles : register(u1);
ConsumeStructuredBuffer<uint>     deadParticles   : register(u2);
ConsumeStructuredBuffer<Particle> injectionBuf    : register(u3);
#endif

#ifdef SIMULATION
RWStructuredBuffer<float2>   sortedParticles : register(u1);
AppendStructuredBuffer<uint> deadParticles   : register(u2);
#endif

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
    if (id >= (uint)Params.DeltaTimeMaxParticlesGroupdim.y)
    {
        return;
    }

#ifdef INJECTION
    uint poolId = deadParticles.Consume();
    Particle p  = injectionBuf.Consume();
    
    float distance = length(Params.CameraPosX.xyz - mul(float4(p.Position.xyz, 1.0f), Params.World).xyz);
    
    sortedParticles.Append(float2(poolId, -distance));
    
    particlesPool[poolId] = p;  
#endif
    
#ifdef SIMULATION
    uint pId   = (uint) sortedParticles[id].x;
    Particle p = particlesPool[pId];
    
    p.LifeTime -= Params.DeltaTimeMaxParticlesGroupdim.x;
    
    [branch]
    if (p.LifeTime <= 0)
    {
        deadParticles.Append(pId);
        sortedParticles[id] = float2(pId, 1000);       
        p.Color0           = float4(1.0f, 0.0f, 0.0f, 1.0f);
        p.Size0Size1       = float2(20, 20);
        particlesPool[pId] = p;
        return;
    }  
#endif
    
#ifdef ADD_GRAVITY  
    p.Velocity += float4(0, -9.8f * Params.DeltaTimeMaxParticlesGroupdim.x, 0, 0);   
#endif
    
#ifdef MOVE
    p.Position.xyz += p.Velocity.xyz * Params.DeltaTimeMaxParticlesGroupdim.x;   
#endif

#ifdef SIMULATION
    float distance      = length(Params.CameraPosX.xyz - mul(float4(p.Position.xyz, 1.0f), Params.World).xyz);
    sortedParticles[id] = float2(pId, -distance);
    particlesPool[pId]  = p;
#endif
}