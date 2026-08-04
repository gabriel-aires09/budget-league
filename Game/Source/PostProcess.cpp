#include "PostProcess.h"

#include "StaticModelAsset.h" // assets::Path

#include <raylib.h>
#include <rlgl.h>

namespace
{
    Shader brightShader = {};
    Shader blurShader = {};
    bool loaded = false;

    int thresholdLocation = -1;
    int directionLocation = -1;

    RenderTexture2D sceneTarget = {};
    RenderTexture2D bloomA = {};
    RenderTexture2D bloomB = {};
    int targetWidth = 0;
    int targetHeight = 0;
    bool capturing = false;

    // The bloom chain runs at a quarter of each axis. It is blurred anyway, so
    // the resolution buys nothing, and it makes the two blur passes cheap enough
    // to leave on.
    const int BLOOM_DIVISOR = 4;
    const float BLOOM_THRESHOLD = 0.55f;
    // Additive, so it lifts the bright edges without washing out the field.
    const unsigned char BLOOM_STRENGTH = 190;

    void ReleaseTargets()
    {
        if (targetWidth == 0)
            return;

        UnloadRenderTexture(sceneTarget);
        UnloadRenderTexture(bloomA);
        UnloadRenderTexture(bloomB);
        targetWidth = 0;
        targetHeight = 0;
    }

    // Rebuilt whenever the window changes size, which is why nothing is created
    // in Load: the resolution setting can change at any time.
    bool EnsureTargets()
    {
        int width = GetScreenWidth();
        int height = GetScreenHeight();
        if (width <= 0 || height <= 0)
            return false;
        if (width == targetWidth && height == targetHeight)
            return true;

        ReleaseTargets();
        sceneTarget = LoadRenderTexture(width, height);
        bloomA = LoadRenderTexture(width / BLOOM_DIVISOR, height / BLOOM_DIVISOR);
        bloomB = LoadRenderTexture(width / BLOOM_DIVISOR, height / BLOOM_DIVISOR);
        targetWidth = width;
        targetHeight = height;

        SetTextureFilter(sceneTarget.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(bloomA.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(bloomB.texture, TEXTURE_FILTER_BILINEAR);
        return true;
    }

    // A render texture is stored bottom up, so every draw of one flips the
    // source rectangle. Getting this wrong is silent - the image is simply
    // upside down - so it lives in one place.
    void DrawTarget(const RenderTexture2D &source, int width, int height, Color tint)
    {
        Rectangle from = { 0.0f, 0.0f, (float)source.texture.width, -(float)source.texture.height };
        Rectangle to = { 0.0f, 0.0f, (float)width, (float)height };
        DrawTexturePro(source.texture, from, to, Vector2{ 0.0f, 0.0f }, 0.0f, tint);
    }
}

void postprocess::Load()
{
    if (loaded)
        return;

    // Both share raylib's default vertex shader: these are full screen passes,
    // so there is nothing for a vertex shader of our own to do.
    brightShader = LoadShader(nullptr, assets::Path("Shaders/Bright.fs").c_str());
    blurShader = LoadShader(nullptr, assets::Path("Shaders/Blur.fs").c_str());

    // Same trap as the lit shader: raylib answers a missing file with its own
    // default shader, whose id is perfectly valid, so comparing against the
    // default id is the only reliable test.
    unsigned int defaultId = rlGetShaderIdDefault();
    if (brightShader.id == defaultId || blurShader.id == defaultId ||
        !IsShaderValid(brightShader) || !IsShaderValid(blurShader))
    {
        TraceLog(LOG_WARNING, "POSTFX: bloom shaders unavailable, drawing without post-processing");
        UnloadShader(brightShader);
        UnloadShader(blurShader);
        brightShader = {};
        blurShader = {};
        return;
    }

    thresholdLocation = GetShaderLocation(brightShader, "threshold");
    directionLocation = GetShaderLocation(blurShader, "direction");
    float threshold = BLOOM_THRESHOLD;
    SetShaderValue(brightShader, thresholdLocation, &threshold, SHADER_UNIFORM_FLOAT);

    loaded = true;
    TraceLog(LOG_INFO, "POSTFX: bloom ready");
}

void postprocess::Unload()
{
    ReleaseTargets();
    if (!loaded)
        return;

    UnloadShader(brightShader);
    UnloadShader(blurShader);
    loaded = false;
}

bool postprocess::Available()
{
    return loaded;
}

bool postprocess::Begin(bool enabled)
{
    capturing = enabled && loaded && EnsureTargets();
    if (!capturing)
        return false;

    BeginTextureMode(sceneTarget);
    return true;
}

void postprocess::End()
{
    if (!capturing)
        return;
    capturing = false;

    EndTextureMode();

    const int bloomWidth = bloomA.texture.width;
    const int bloomHeight = bloomA.texture.height;

    // Bright pass, straight into the quarter sized target: the downsample and
    // the threshold happen in the same draw.
    BeginTextureMode(bloomA);
    BeginShaderMode(brightShader);
    DrawTarget(sceneTarget, bloomWidth, bloomHeight, WHITE);
    EndShaderMode();
    EndTextureMode();

    // Then one axis at a time, ping-ponging between the two small targets.
    Vector2 horizontal = { 1.0f / (float)bloomWidth, 0.0f };
    Vector2 vertical = { 0.0f, 1.0f / (float)bloomHeight };

    BeginTextureMode(bloomB);
    BeginShaderMode(blurShader);
    SetShaderValue(blurShader, directionLocation, &horizontal, SHADER_UNIFORM_VEC2);
    DrawTarget(bloomA, bloomWidth, bloomHeight, WHITE);
    EndShaderMode();
    EndTextureMode();

    BeginTextureMode(bloomA);
    BeginShaderMode(blurShader);
    SetShaderValue(blurShader, directionLocation, &vertical, SHADER_UNIFORM_VEC2);
    DrawTarget(bloomB, bloomWidth, bloomHeight, WHITE);
    EndShaderMode();
    EndTextureMode();

    // The scene as it was, then the glow added on top.
    DrawTarget(sceneTarget, targetWidth, targetHeight, WHITE);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTarget(bloomA, targetWidth, targetHeight,
               Color{ 255, 255, 255, BLOOM_STRENGTH });
    EndBlendMode();
}
