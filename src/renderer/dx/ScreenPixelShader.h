#pragma once

const char screenPixelShaderString[] = R"(
Texture2D shaderTexture;
SamplerState samplerState;

#define SCANLINE_FACTOR 0.5
#define SCANLINE_PERIOD 1

static const float M_PI = 3.14159265f;

float Gaussian2D(float x, float y, float sigma)
{
    return 1/(sigma*sqrt(2*M_PI)) * exp(-0.5*(x*x + y*y)/sigma/sigma);
}

float4 Blur(Texture2D input, float2 tex_coord, float sigma)
{
    uint width, height;
    shaderTexture.GetDimensions(width, height);

    float texelWidth = 1.0f/width;
    float texelHeight = 1.0f/height;

    float4 color = { 0, 0, 0, 0 };
    float factor = 1;

    int sampleCount = 13;

    for (int x = 0; x < sampleCount; x++) 
    {
        float2 samplePos = { 0, 0 };

        samplePos.x = tex_coord.x + (x - sampleCount/2) * texelWidth;
        for (int y = 0; y < sampleCount; y++)
        {
            samplePos.y = tex_coord.y + (y - sampleCount/2) * texelHeight;
            if (samplePos.x <= 0 || samplePos.y <= 0 || samplePos.x >= width || samplePos.y >= height) continue;

            color += input.Sample(samplerState, samplePos) * Gaussian2D((x - sampleCount/2), (y - sampleCount/2), sigma);
        }
    }

    return color;
}

float SquareWave(float y)
{
    return 1 - (floor(y / SCANLINE_PERIOD) % 2) * SCANLINE_FACTOR;
}

float4 Scanline(float4 color, float4 pos)
{
    float wave = SquareWave(pos.y);

    // TODO make this configurable.
    // Remove the && false to draw scanlines everywhere.
    if (length(color.rgb) < 0.2 && false)
    {
        return color + wave*0.1;
    }
    else
    {
        return color * wave;
    }
}

float4 main(float4 pos : SV_POSITION, float2 tex : TEXCOORD) : SV_TARGET
{
    Texture2D input = shaderTexture;

    // TODO Make these configurable in some way.
    float4 color = input.Sample(samplerState, tex);
    color += Blur(input, tex, 2)*0.3;
    color = Scanline(color, pos);

    return color;
}
)";
