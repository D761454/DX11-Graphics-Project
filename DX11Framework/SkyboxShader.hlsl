TextureCube skybox : register(t0);
SamplerState bilinerSampler : register(s0);

struct DirectionalLight
{
    float4 m_ambientColor;
    float4 m_specularColor;
    float4 m_diffuseColor;
    float3 m_direction;
    float padding;
};

struct PointLight
{
    float4 m_ambientColor;
    float4 m_specularColor;
    float4 m_diffuseColor;
    float m_maxDistance;
    float3 m_position;
    float3 m_attenuation; // control light intensity falloff
    float padding;
};

struct Fog
{
    float4 m_color;
    float m_start;
    float m_range;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
    float4 DiffuseMaterial;
    float4 AmbientMaterial;
    float4 SpecularMaterial;
    DirectionalLight DirLight;
    PointLight PtLight;
    uint HasTexture;
    float3 EyePosW;
    Fog FogW;
    uint SpecMap;
    uint NormMap;
    float SpecularPower;
    uint FogEnabled;
}

struct SkyboxVS_Out
{
    float4 position : SV_POSITION;
    float3 WorldPos : TEXCOORD1;
    float3 texcoord : TEXCOORD0;
};

SkyboxVS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float3 Tangent : TANGENT)
{
    SkyboxVS_Out output = (SkyboxVS_Out) 0;
    
    float4 Pos4 = float4(Position, 1.0f);
    output.position = mul(Pos4, World);
    output.WorldPos = output.position;
    
    output.position = mul(output.position, View);
    
    output.position = mul(output.position, Projection);
    
    output.position = output.position.xyww;
    
    output.texcoord = Position;
    
    return output;
}

float4 PS_main(SkyboxVS_Out input) : SV_TARGET
{
    float4 texColor = skybox.Sample(bilinerSampler, input.texcoord); // replaces ambient and diffuse mat
    
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    ambient = texColor;
    
    if (FogEnabled)
    {
        float fogLerp = saturate(input.WorldPos.y / FogW.m_range);
        
        ambient = lerp(FogW.m_color, ambient, fogLerp);
    }
    
    return ambient;
}