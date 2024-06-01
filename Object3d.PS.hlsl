

cbuffer Material : register(b0)
{
    float4 color;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
//Textureの宣言
//Textureは基本的にSamplerを介して読む=Samplingという
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);


struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;

};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;


    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    //サンプリングしたtextureの色とマテリアルん色を乗算して合成する
    output.color = color * textureColor;
  
    return output;
}
