Texture2D diffuseTex : register(t0);
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

struct VS_Out // keep most as this will be affected by light, unlike skybox
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
    float3 EyePos : LIGHTPOS;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float3 Tangent : TANGENT)
{
    VS_Out output = (VS_Out) 0;
    
    output.texcoord = TexCoord;
    
    float4 Pos4 = float4(Position, 1.0f);
    output.position = mul(Pos4, World);
    output.WorldPos = output.position; //
    
    output.normal = Normal;
    
    output.EyePos = EyePosW;
    
    output.position = mul(output.position, View);
    
    output.position = mul(output.position, Projection);
    
    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{
    float4 texColor = diffuseTex.Sample(bilinerSampler, input.texcoord); // replaces ambient and diffuse mat
    
    clip(texColor.a - 0.1f);
    
    float3 WorldNorm = normalize(mul(float4(input.normal, 0), World));
    
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float DiffuseAmount;
    float3 Reflect;
    
    DiffuseAmount = saturate(dot(normalize(DirLight.m_direction), WorldNorm));
    
    float3 ViewV = input.EyePos - input.WorldPos; // object to camera, 
    
    // Cache the distance to the eye from the surface point.
    float distToEye = length(ViewV);
    
    ViewV = normalize(ViewV);
    
    if (HasTexture == 1)
    {
        diffuse = DiffuseAmount * (DirLight.m_diffuseColor * texColor);
        ambient = DirLight.m_ambientColor * texColor;
    }
    else
    {
        diffuse = DiffuseAmount * (DirLight.m_diffuseColor * DiffuseMaterial);
        ambient = DirLight.m_ambientColor * AmbientMaterial;
    }
    
    input.color = diffuse + ambient;
    
    if (FogEnabled)
    {
        float fogLerp = saturate((distToEye - FogW.m_start) / FogW.m_range);
    
        input.color = lerp(input.color, FogW.m_color, fogLerp);
    }
    
    
    return input.color;
}