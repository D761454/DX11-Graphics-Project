Texture2D diffuseTex : register(t0);
Texture2D specularTex : register(t1);
Texture2D normalTex : register(t2);
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

struct VS_Out
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
    float3 EyePos : LIGHTPOS;
    float3 LightPosPoint : LIGHTPOS1;
    float3 Tangent : TANGENT;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float3 Tangent : TANGENT)
{   
    VS_Out output = (VS_Out)0;
    
    output.texcoord = TexCoord;
    
    float4 Pos4 = float4(Position, 1.0f);
    output.position = mul(Pos4, World);
    output.WorldPos = output.position;  //
    
    output.normal = Normal;
    
    output.Tangent = Tangent;
    
    output.EyePos = EyePosW;
    output.LightPosPoint = PtLight.m_position;
    
    output.position = mul(output.position, View);
    
    output.position = mul(output.position, Projection);
    
    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{
    float4 texColor = diffuseTex.Sample(bilinerSampler, input.texcoord); // replaces ambient and diffuse mat
    float4 texSpec = specularTex.Sample(bilinerSampler, input.texcoord);
    float3 texNorm = normalTex.Sample(bilinerSampler, input.texcoord);
    
    clip(texColor.a - 0.1f);
    
    texNorm = (texNorm * 2.0f) - 1.0f;
    
    float3 WorldNorm = normalize(mul(float4(normalize(input.normal), 0), World));
    
    // use TBN to move from tangent space (normal map) into world space
    float3 Tangent = input.Tangent - dot(input.Tangent, WorldNorm) * WorldNorm;
    float3 biTangent = cross(WorldNorm, Tangent);
    
    // row major
    float3x3 TBN = float3x3(normalize(Tangent), normalize(biTangent), normalize(input.normal));
    
    float3 TexWorldNorm = normalize(mul(texNorm, TBN)); // tangent space -> model space
    TexWorldNorm = normalize(mul(float4(TexWorldNorm, 0), World)); // model space -> world space
    
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float DiffuseAmount;
    float3 Reflect;
    
    // normalize both vectors before dot
    // a dot b = cos angle between a and b
    
    if (NormMap == 1)
    {
        DiffuseAmount = saturate(dot(normalize(DirLight.m_direction), TexWorldNorm));

        Reflect = reflect(normalize(-DirLight.m_direction), TexWorldNorm);
    }
    else
    {
        DiffuseAmount = saturate(dot(normalize(DirLight.m_direction), WorldNorm));

        Reflect = reflect(normalize(-DirLight.m_direction), WorldNorm);
    }
    
    
    float3 ViewV = input.EyePos - input.WorldPos; // object to camera, 
    
    // Cache the distance to the eye from the surface point.
    float distToEye = length(ViewV);
    
    ViewV = normalize(ViewV);
    
    float dotViewReflect = dot(Reflect, ViewV);
    
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
    
    
    if (SpecMap == 1)
    {
        specular = texSpec * SpecularMaterial * pow(saturate(dotViewReflect), SpecularPower);
    }
    else
    {
        specular = DirLight.m_specularColor * SpecularMaterial * pow(saturate(dotViewReflect), SpecularPower);
    }
    
    // do point light in range check
    float3 lightVector = PtLight.m_position - input.WorldPos;
    float dist = length(lightVector);
    
    if (dist < PtLight.m_maxDistance)
    {
        lightVector = normalize(lightVector);
        
        float4 pDiffuse;
        float4 pSpecular;
        
        float DiffuseAmountPt;
        float3 Ref;
        float SpecFactor;
        
        if (NormMap == 1)
        {
            DiffuseAmountPt = saturate(dot(lightVector, TexWorldNorm));
            if (DiffuseAmountPt > 0.0f)
            {
                Ref = reflect(-lightVector, TexWorldNorm);
                SpecFactor = pow(saturate(dot(Ref, ViewV)), SpecularPower);
            }
        }
        else
        {
            DiffuseAmountPt = saturate(dot(lightVector, WorldNorm));
            if (DiffuseAmountPt > 0.0f)
            {
                Ref = reflect(-lightVector, WorldNorm);
                SpecFactor = pow(saturate(dot(Ref, ViewV)), SpecularPower);
            }
        }
        
        if (HasTexture == 1)
        {
            pDiffuse = DiffuseAmountPt * (PtLight.m_diffuseColor * texColor);
        }
        else
        {
            pDiffuse = DiffuseAmountPt * (PtLight.m_diffuseColor * DiffuseMaterial);
        }

        if (SpecMap == 1)
        {
            pSpecular = texSpec * SpecularMaterial * SpecFactor;
        }
        else
        {
            pSpecular = PtLight.m_specularColor * SpecularMaterial * SpecFactor;
        }
        
        float attenuation = 1.0f / dot(PtLight.m_attenuation, float3(1.0f, dist, dist * dist));
        
        pDiffuse *= attenuation;
        pSpecular *= attenuation;
        
        diffuse += pDiffuse;
        specular += pSpecular;
    }
    
    input.color = diffuse + ambient + specular;
    
    if (FogEnabled)
    {
        float fogLerp = saturate((distToEye - FogW.m_start) / FogW.m_range);
    
        input.color = lerp(input.color, FogW.m_color, fogLerp);
    }
    
    
    return input.color;
}