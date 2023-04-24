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
};
cbuffer CB1 : register(b0)
{
    ConstParams Params;
}

ConsumeStructuredBuffer<Particle> particlesBufSrc : register(u0);
AppendStructuredBuffer<Particle>  particlesBufDst : register(u1);

// GBUFFER
Texture2D NormalMap   : register(t0);
Texture2D WorldPosMap : register(t1);
struct GBufferData
{
    float3 Normal;
    float3 WorldPos;
};
GBufferData ReadGBuffer(float2 screenPos)
{
    GBufferData buf = (GBufferData) 0;
    buf.WorldPos    = WorldPosMap.Load(float3(screenPos, 0)).xyz;
    buf.Normal      = NormalMap.Load(float3(screenPos, 0)).xyz;    
    return buf;
}
// DEPTH MAP
Texture2D DepthMap : register(t2);

// PARTICLE MASS FOR GS
StructuredBuffer<Particle> renderBufSrc : register(t3); // буфер частиц

struct VSOutput
{
    int vertexID : TEXCOORD0;
};
// т.к. вертексов у нас нет, мы можем получить текущий ID вертекса при рисовании без использования Vertex Buffer
VSOutput VSMain(uint vertexID : SV_VertexID) // вершинный шейдер на вход получает VertexID
{
    VSOutput output;
    output.vertexID = vertexID; // отправяем на выход
    return output;
}

struct GSOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 Tex      : TEXCOORD0;
};
[maxvertexcount(4)] // максимальное кол-во вертексов, которое мы можем добавить
void GSMain(point VSOutput inputPoint[1], inout TriangleStream<GSOutput> outputStream)
{
    GSOutput p0, p1, p2, p3; // 4 точки
    
    Particle prt = renderBufSrc[inputPoint[0].vertexID]; // берем нашу частицу из хранилища частиц
    
    float sz     = prt.Size0Size1.x; // высчитываем значение Size'а
    float4 color = prt.Color0;       // берем цвет
    
    float4 wvPos = prt.Position;
    wvPos = mul(float4(wvPos.xyz, 1), Params.World);
    wvPos = mul(float4(wvPos.xyz, 1), Params.View);
    wvPos = float4(wvPos.xyz, 1.0f); // получаем WorldViewPosition в пространстве камеры
    
    // немного растягиваем 4 точки по пространству камеры
    p0.Position = mul(wvPos + float4(sz, sz, 0, 0), Params.Projection);
    p0.Tex      = float2(1, 1);
    p0.Color    = color;
    
    p1.Position = mul(wvPos + float4(-sz, sz, 0, 0), Params.Projection);
    p1.Tex      = float2(0, 1);
    p1.Color    = color;
    
    p2.Position = mul(wvPos + float4(-sz, -sz, 0, 0), Params.Projection);
    p2.Tex      = float2(0, 0);
    p2.Color    = color;
    
    p3.Position = mul(wvPos + float4(sz, -sz, 0, 0), Params.Projection);
    p3.Tex      = float2(1, 0);
    p3.Color    = color;
    
    // Добавление вершины (вертексы)
    outputStream.Append(p0);
    outputStream.Append(p1);
    outputStream.Append(p3);
    outputStream.Append(p2);
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
    uint id = groupID.x * THREAD_IN_GROUP_TOTAL + groupID.y * Params.DeltaTimeMaxParticlesGroupdim.z * THREAD_IN_GROUP_TOTAL + groupIndex;
    
    [flatten] // если текущий индекс больше максимального количества живых частиц, то ничего не делаем
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
        p.Velocity += float4(0, -10.0f * Params.DeltaTimeMaxParticlesGroupdim.x, 0, 0);  
    #endif
        p.Position.xyz += p.Velocity * Params.DeltaTimeMaxParticlesGroupdim.x;     
        
        float4 partPos  = mul(mul(mul(float4(p.Position.xyz, 1.0f), Params.World), Params.View), Params.Projection);
        partPos         = partPos / partPos.w;
        float partDepth = partPos.z;   
        
        float2 particleUVCoord = (partPos.xy + float2(1.0f, 1.0f)) / 2;
        particleUVCoord.y = 1.0f - particleUVCoord.y;
        particleUVCoord.xy *= float2(1200, 1200);
    
        float3 Normal   = NormalMap.Load(float3(particleUVCoord.xy, 0));   // это для того, чтобы проверить передается ли GBuffer
        float3 WorldPos = WorldPosMap.Load(float3(particleUVCoord.xy, 0)); // это для того, чтобы проверить передается ли GBuffer
        float  depthVal = DepthMap.Load(float3(particleUVCoord.xy, 0));
                
        float3 temp = WorldPos.xyz - Normal.xyz; // это для того, чтобы проверить передается ли GBuffer
        if (partDepth < depthVal)
        {
            p.Velocity *= -1;
            p.Velocity += float4(temp.xyz, 1.0f); // это для того, чтобы проверить передается ли GBuffer
        }
    
        particlesBufDst.Append(p); // переливание из буфера в буфер
    }
#endif
}