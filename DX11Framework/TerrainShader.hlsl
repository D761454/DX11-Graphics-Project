Texture2D texLDirt : register(t0);
Texture2D texDDirt : register(t1);
Texture2D texGrass : register(t2);
Texture2D texStone : register(t3);
Texture2D texSnow : register(t4);
Texture2D texBlend : register(t5);
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
    float3 Tangent : TANGENT;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float3 Tangent : TANGENT)
{
    VS_Out output = (VS_Out) 0;
    
    output.texcoord = TexCoord;
    
    float4 Pos4 = float4(Position, 1.0f);
    output.position = mul(Pos4, World);
    output.WorldPos = output.position; //
    
    output.normal = Normal;
    
    output.Tangent = Tangent;
    
    output.EyePos = EyePosW;
    
    output.position = mul(output.position, View);
    
    output.position = mul(output.position, Projection);
    
    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{
    float4 tex0 = texLDirt.Sample(bilinerSampler, input.texcoord); // replaces ambient and diffuse mat
    float4 tex1 = texDDirt.Sample(bilinerSampler, input.texcoord);
    float4 tex2 = texGrass.Sample(bilinerSampler, input.texcoord);
    float4 tex3 = texStone.Sample(bilinerSampler, input.texcoord);
    float4 tex4 = texSnow.Sample(bilinerSampler, input.texcoord);
    float4 tex5 = texBlend.Sample(bilinerSampler, input.texcoord);
    
    clip(tex0.a - 0.1f);
    
    float3 WorldNorm = normalize(mul(float4(input.normal, 0), World));
    
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float DiffuseAmount;
    float3 Reflect;
    
    DiffuseAmount = saturate(dot(normalize(DirLight.m_direction), WorldNorm));

    Reflect = reflect(normalize(-DirLight.m_direction), WorldNorm);
    
    
    float3 ViewV = input.EyePos - input.WorldPos; // object to camera, 
    
    // Cache the distance to the eye from the surface point.
    float distToEye = length(ViewV);
    
    ViewV = normalize(ViewV);
    
    float dotViewReflect = dot(Reflect, ViewV);
    
    if (HasTexture == 1)
    {
        // height checks - workd without any smoothness - use blend map to edit
        if (input.WorldPos.y > 0 && input.WorldPos.y <= 10)
        {
            diffuse = DiffuseAmount * (DirLight.m_diffuseColor * lerp(tex2, tex1, input.WorldPos.y / 10));
            ambient = DirLight.m_ambientColor * lerp(tex2, tex1, input.WorldPos.y / 10);
        }
        else if (input.WorldPos.y > 10 && input.WorldPos.y <= 20)
        {
            diffuse = DiffuseAmount * (DirLight.m_diffuseColor * lerp(tex1, tex0, (input.WorldPos.y - 10) / 10));
            ambient = DirLight.m_ambientColor * lerp(tex1, tex0, (input.WorldPos.y - 10) / 10);
        }
        else if (input.WorldPos.y > 20 && input.WorldPos.y <= 30)
        {
            diffuse = DiffuseAmount * (DirLight.m_diffuseColor * lerp(tex0, tex3, (input.WorldPos.y - 20) / 10));
            ambient = DirLight.m_ambientColor * lerp(tex0, tex3, (input.WorldPos.y - 20) / 10);
        }
        else if (input.WorldPos.y > 30 && input.WorldPos.y <= 40)
        {
            diffuse = DiffuseAmount * (DirLight.m_diffuseColor * lerp(tex3, tex4, (input.WorldPos.y - 30) / 10));
            ambient = DirLight.m_ambientColor * lerp(tex3, tex4, (input.WorldPos.y - 30) / 10);
        }
        else
        {
            diffuse = DiffuseAmount * (DirLight.m_diffuseColor * tex4);
            ambient = DirLight.m_ambientColor * tex4;
        }
    }
    else
    {
        diffuse = DiffuseAmount * (DirLight.m_diffuseColor * DiffuseMaterial);
        ambient = DirLight.m_ambientColor * AmbientMaterial;
    }
    
    specular = DirLight.m_specularColor * SpecularMaterial * pow(saturate(dotViewReflect), SpecularPower);
    
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
        
        DiffuseAmountPt = saturate(dot(lightVector, WorldNorm));
        if (DiffuseAmountPt > 0.0f)
        {
            Ref = reflect(-lightVector, WorldNorm);
            SpecFactor = pow(saturate(dot(Ref, ViewV)), SpecularPower);
        }
        
        if (HasTexture == 1)
        {
            // height checks - workd without any smoothness - use blend map to edit
            if (input.WorldPos.y > 0 && input.WorldPos.y <= 10)
            {
                pDiffuse = DiffuseAmountPt * (PtLight.m_diffuseColor * lerp(tex2, tex1, input.WorldPos.y / 10));
            }
            else if (input.WorldPos.y > 10 && input.WorldPos.y <= 20)
            {
                pDiffuse = DiffuseAmountPt * (PtLight.m_diffuseColor * lerp(tex1, tex0, (input.WorldPos.y - 10) / 10));
            }
            else if (input.WorldPos.y > 20 && input.WorldPos.y <= 30)
            {
                pDiffuse = DiffuseAmountPt * (PtLight.m_diffuseColor * lerp(tex0, tex3, (input.WorldPos.y - 20) / 10));
            }
            else if (input.WorldPos.y > 30 && input.WorldPos.y <= 40)
            {
                pDiffuse = DiffuseAmountPt * (PtLight.m_diffuseColor * lerp(tex3, tex4, (input.WorldPos.y - 30) / 10));
            }
            else
            {
                pDiffuse = DiffuseAmountPt * (PtLight.m_diffuseColor * tex4);
            }
        }
        else
        {
            pDiffuse = DiffuseAmountPt * (PtLight.m_diffuseColor * DiffuseMaterial);
        }

        pSpecular = PtLight.m_specularColor * SpecularMaterial * SpecFactor;
        
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