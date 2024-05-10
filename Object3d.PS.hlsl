

struct Material
{
    float32_t4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);

//Textureの宣言
//Textureは基本的にSamplerを介して読む=Samplingという
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;

};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;


    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    //サンプリングしたtextureの色とマテリアルん色を乗算して合成する
    output.color = gMaterial.color * textureColor;
  
    return output;
}
