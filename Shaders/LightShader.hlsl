struct CurrentCameraData
{
    row_major matrix view;
    row_major matrix projection;
    row_major matrix model;
    float3           position;
};
cbuffer CameraConstantBuffer : register(b0)
{
    CurrentCameraData curCamera;
};

struct MaterialData
{
    float3 ambient;
    float3 diffuse;
    float3 specular;
};
struct DirectionalLightData
{
    float3 lightColor;
    float3 direction;
};
struct PointLightData
{
    float3 lightColor;
    float4 valueConLinQuadCount;
    float3 position;
};
cbuffer LightConstantBuffer : register(b1)
{
    MaterialData         material;
    DirectionalLightData dirLight;
    PointLightData       poiLight[2];
};

cbuffer LightCameraConstantBuffer : register(b2)
{   
    row_major matrix ViewProj[4];
    float4           Distances;
};

struct VS_IN
{
    float3 pos : POSITION0;
    float2 tex : TEXCOORD0;
    float4 normal : NORMAL0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 normal : NORMAL;
    float2 tex : TEXCOORD;
    
    float4 modelPos : POSITION;
    float4 curCameraViewPos : POSITION1;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output = (PS_IN) 0;
 
    output.pos    = mul(mul(mul(float4(input.pos, 1.0f), curCamera.model), curCamera.view), curCamera.projection); // положение vertex'а относительно model мира и viewProjection основной камеры
    output.normal = mul(transpose(curCamera.model), input.normal);
    output.tex    = input.tex;
    
    float4 modelPos = mul(float4(input.pos, 1.0f), curCamera.model);
    output.modelPos = modelPos;
    
    output.curCameraViewPos = mul(modelPos, curCamera.view);
    
    return output;
}

Texture2D DiffuseMap                    : register(t0);
SamplerState Sampler                    : register(s0);

Texture2DArray ShadowMap                : register(t1);
SamplerComparisonState ShadowMapSampler : register(s1);

float3 CalcDirLight(float4 modelPos, MaterialData material, float3 normal, float3 viewDir, float2 tex, float4 posViewProj, float layer);

float4 PSMain(PS_IN input) : SV_Target
{
    float3 normal    = normalize(input.normal);
    float3 viewDir = normalize(curCamera.position - input.modelPos.xyz);
    
    float layer = 3.0f;
    float depthVal = abs(input.curCameraViewPos.z);
    for (int i = 0; i < 4; ++i)
    {
        if (depthVal < Distances[i])
        {
            layer = (float) i;
            break;
        }
    }
    
    float4 dirLightViewProj = mul(input.modelPos, ViewProj[layer]);
    
    float3 result = CalcDirLight(input.modelPos, material, normal, viewDir, input.tex, dirLightViewProj, layer);

    return float4(result, 1.0f);
}

float IsLighted(float3 lightDir, float3 normal, float4 dirLightViewProj, float layer);

float3 CalcDirLight(float4 modelPos, MaterialData material, float3 normal, float3 viewDir, float2 tex, float4 dirLightViewProj, float layer)
{
    float3 diffValue  = DiffuseMap.Sample(Sampler, tex).rgb; 
    
    // DIRECTIONAL LIGHT
    float3 lightDir   = normalize( - dirLight.direction);
    float  diff       = max(dot(normal, lightDir), 0.0);
    float3 reflectDir = reflect( - lightDir, normal);
    float  spec       = pow(max(dot(viewDir, reflectDir), 0.0), 128);
    
    float3 ambient  = material.ambient         * diffValue * dirLight.lightColor;
    float3 diffuse  = material.diffuse  * diff * diffValue;
    float3 specular = material.specular * spec * diffValue;
    
    // POINT LIGHTS
    for (int i = 0; i < poiLight[i].valueConLinQuadCount.w; i++)
    {
        float distance = length(poiLight[i].position - modelPos.xyz);
        float attenuation = 1.0f / (poiLight[i].valueConLinQuadCount.x + poiLight[i].valueConLinQuadCount.y * distance + poiLight[i].valueConLinQuadCount.z * (distance * distance));
        ambient  += ambient  * attenuation * poiLight[i].lightColor;
        diffuse  += diffuse  * attenuation * poiLight[i].lightColor;
        specular += specular * attenuation * poiLight[i].lightColor;
    }
    
    float1 isLighted = 1;
    isLighted = IsLighted(lightDir, normal, dirLightViewProj, layer);
    
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