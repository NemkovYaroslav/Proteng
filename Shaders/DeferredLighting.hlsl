struct CurrentCameraData
{
    row_major matrix view;
    float3           position;
};
cbuffer CameraConstantBuffer : register(b0)
{
    CurrentCameraData curCamera;
};

struct DirectionalLightData
{
    float3 lightColor;
    float3 direction;
};
cbuffer LightConstantBuffer : register(b1)
{
    DirectionalLightData dirLight;
};

struct ShadowData
{
    row_major matrix viewProj[4];
    float4           distances;
};
cbuffer LightCameraConstantBuffer : register(b2)
{
    ShadowData shadow;
};

struct VS_IN
{
    float4 pos : POSITION0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
};

PS_IN VSMain(uint id : SV_VertexID)
{
    PS_IN output = (PS_IN) 0;
    
    float2 inds = float2(id & 1, (id & 2) >> 1);
    output.pos  = float4(inds * float2(2, -2) + float2(-1, 1), 0, 1);
    
    return output;
}

Texture2D DiffuseMap  : register(t0);
Texture2D NormalMap   : register(t1);
Texture2D WorldPosMap : register(t2);

Texture2DArray ShadowMap                : register(t3);
SamplerComparisonState ShadowMapSampler : register(s0);

struct GBufferData
{
    float4 DiffuseSpec;
    float3 Normal;
    float3 WorldPos;
};

GBufferData ReadGBuffer(float2 screenPos)
{
    GBufferData buf = (GBufferData) 0;
    
    buf.DiffuseSpec = DiffuseMap.Load(float3(screenPos, 0));
    buf.WorldPos    = WorldPosMap.Load(float3(screenPos, 0)).xyz;
    buf.Normal      = NormalMap.Load(float3(screenPos, 0)).xyz;
    
    return buf;
}

float3 CalcDirLight(DirectionalLightData dirLight, float3 normal, float3 viewDir, GBufferData gBuffer, float4 posViewProj, float layer);

float4 PSMain(PS_IN input) : SV_Target
{
    GBufferData gBuffer = ReadGBuffer(input.pos.xy);

    float3 normal  = normalize(gBuffer.Normal);
    float3 viewDir = normalize(curCamera.position - gBuffer.WorldPos.xyz);
    
    float4 cameraViewPosition = mul(float4(gBuffer.WorldPos.xyz, 1.0f), curCamera.view);
    
    float layer    = 3.0f;
    float depthVal = abs(cameraViewPosition.z);
    for (int i = 0; i < 4; ++i)
    {
        if (depthVal < shadow.distances[i])
        {
            layer = (float) i;
            break;
        }
    }
    
    float4 dirLightViewProj = mul(float4(gBuffer.WorldPos.xyz, 1.0f), shadow.viewProj[layer]);
    
    float3 result = CalcDirLight(dirLight, normal, viewDir, gBuffer, dirLightViewProj, layer);
    
    return float4(result, 1.0f);
}

float IsLighted(float3 lightDir, float3 normal, float4 dirLightViewProj, float layer);

float3 CalcDirLight(DirectionalLightData dirLight, float3 normal, float3 viewDir, GBufferData gBuffer, float4 posViewProj, float layer)
{
    float3 diffValue = gBuffer.DiffuseSpec;
    
    //DIRECTIONAL LIGHT
    float3 lightDir = normalize(-dirLight.direction);
    float  diff = max(dot(normal, lightDir), 0.0);
    float3 reflectDir = reflect(-lightDir, normal);
    float  spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);
    
    float3 ambient  = float3(0.2f, 0.2f, 0.2f)        * diffValue;
    float3 diffuse  = float3(0.5f, 0.5f, 0.5f) * diff * diffValue;
    float3 specular = float3(0.5f, 0.5f, 0.5f) * spec * diffValue;
    
    float isLighted = 1;
    
    isLighted = IsLighted(lightDir, normal, posViewProj, layer);
    
    return (ambient + (diffuse + specular) * isLighted);
}

float IsLighted(float3 lightDir, float3 normal, float4 dirLightViewProj, float layer)
{
    float ndotl = dot(normal, lightDir);
    float bias = clamp(0.005f * (1.0f - ndotl), 0.0f, 0.0005f);
    
    float3 projectTexCoord;

    projectTexCoord.x = dirLightViewProj.x / dirLightViewProj.w;
    projectTexCoord.y = dirLightViewProj.y / dirLightViewProj.w;
    projectTexCoord.z = dirLightViewProj.z / dirLightViewProj.w;

    projectTexCoord.x = projectTexCoord.x * 0.5 + 0.5f;
    projectTexCoord.y = projectTexCoord.y * -0.5 + 0.5f;
    
    float max_depth = ShadowMap.SampleCmpLevelZero(ShadowMapSampler, float3(projectTexCoord.x, projectTexCoord.y, layer), projectTexCoord.z);

    float currentDepth = (dirLightViewProj.z / dirLightViewProj.w);

    currentDepth = currentDepth - bias;
    
    if (max_depth < currentDepth)
    {
        return 0;
    }
    return max_depth;
}