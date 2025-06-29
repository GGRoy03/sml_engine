// ===================================
// Type Definitions
// ===================================

enum SmlShaderFeature_Type
{
    SmlShaderFeat_AlbedoMap = 1 << 0,
    SmlShaderFeat_Instanced = 1 << 1,

    SmlShaderFeat_Count,
};

enum SmlShaderCompile_Result
{
    SmlShaderCompile_Success,
    SmlShaderCompile_ErrorVertexShader,
    SmlShaderCompile_ErrorFragmentShader,
    SmlShaderCompile_ErrorLinking,
};

enum SmlShader_Flag
{
    SmlShader_CompileDebug,
};

enum SmlShader_Type
{
    SmlShader_Vertex,
    SmlShader_Pixel,

    SmlShaderType_Count,
};

// ===================================
// Global Variables
// ===================================

static const char *SmlUberVertexShader_HLSL = R"(

// =============================================================================
// UBER_VERTEX_SHADER.hlsl
// =============================================================================

cbuffer PerFrame : register(b0)
{
    matrix ViewProjection;
};

cbuffer PerObject : register(b1)
{
    #ifdef HAS_INSTANCING

    #else
    matrix WorldMatrix;
    #endif
};

#ifdef HAS_INSTANCING
StructuredBuffer<matrix> InstanceTransforms : register(t10);
#endif

struct VertexInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;

    #ifdef HAS_INSTANCING
    uint InstanceID : SV_InstanceID;
    #endif
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float3 WorldPos : WORLD_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    #ifdef HAS_INSTANCING
    matrix ModelTransform = InstanceTransforms[input.InstanceID];
    #else
    matrix ModelTransform = WorldMatrix;
    #endif

    float4 worldPos = mul(ModelTransform, float4(input.Position, 1.0));
    output.Position = mul(ViewProjection, worldPos);


    // World position for lighting
    output.WorldPos = worldPos.xyz;

    // Transform normal
    output.Normal = normalize(mul(input.Normal, (float3x3)ModelTransform));

    // Pass through texture coordinates
    output.TexCoord = input.TexCoord;

    return output;
}
)";

static const char *SmlUberPixelShader_HLSL = R"(
// =============================================================================
// UBER_PIXEL_SHADER.hlsl  
// =============================================================================

// Constant buffers
cbuffer PerFrame : register(b0)
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    float3 CameraPosition;
    float  Time;
};

cbuffer MaterialBuffer : register(b2)
{
    float3 AlbedoFactor;
    float  MetallicFactor;
    float  RoughnessFactor;
};

// Inputs

struct PSInput 
{
    float4 Position : SV_POSITION;
    float3 WorldPos : WORLD_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

Texture2D    AlbedoText : register(t0);
SamplerState MatSampler : register(s0);
float4 PSMain(PSInput In) : SV_TARGET
{
    #ifdef HAS_ALBEDO_MAP
    float3 Color = AlbedoText.Sample(MatSampler, In.TexCoord).rgb * AlbedoFactor;
    #else
    float3 Color = float3(1.0f, 1.0f, 1.0f);
    #endif
    return float4(Color, 1.0f);
}
)";
