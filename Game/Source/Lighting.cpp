#include "Lighting.h"

#include "StaticModelAsset.h" // assets::Path

#include <raymath.h>
#include <rlgl.h>

namespace
{
    Shader litShader = {};
    bool shaderReady = false;

    // Indoor stadium key light, high and slightly to one side so the flat faces
    // of the cars read as different tones instead of one silhouette.
    // Mostly overhead on purpose. The chase camera swings all the way around the
    // car, so a low sun would leave whichever side faces the player in shadow.
    const Vector3 SUN_DIRECTION = { 0.30f, 0.90f, 0.32f };
    const Vector3 SUN_COLOR = { 0.56f, 0.54f, 0.49f };
    const Vector3 SKY_COLOR = { 0.50f, 0.54f, 0.62f };
    const Vector3 GROUND_COLOR = { 0.18f, 0.19f, 0.24f };

    void SetVec3(const char *name, Vector3 value)
    {
        SetShaderValue(litShader, GetShaderLocation(litShader, name), &value, SHADER_UNIFORM_VEC3);
    }
}

void lighting::Load()
{
    Shader shader = LoadShader(assets::Path("Shaders/Lit.vs").c_str(),
                               assets::Path("Shaders/Lit.fs").c_str());
    // Two separate failures to catch: a shader that would not compile comes back
    // with id 0, but missing files come back as raylib's *default* shader, whose
    // id is perfectly valid. Only the second check notices an uncooked build.
    if (!IsShaderValid(shader) || shader.id == rlGetShaderIdDefault())
    {
        TraceLog(LOG_WARNING, "LIGHTING: lit shader unavailable, drawing unlit");
        UnloadShader(shader);
        return;
    }

    litShader = shader;
    shaderReady = true;

    // The light never moves, so these are set once and stay in the program.
    SetVec3("sunDirection", Vector3Normalize(SUN_DIRECTION));
    SetVec3("sunColor", SUN_COLOR);
    SetVec3("skyColor", SKY_COLOR);
    SetVec3("groundColor", GROUND_COLOR);
}

void lighting::Unload()
{
    if (shaderReady)
        UnloadShader(litShader);

    litShader = {};
    shaderReady = false;
}

void lighting::Apply(Model &model)
{
    if (!shaderReady)
        return;

    for (int i = 0; i < model.materialCount; ++i)
        model.materials[i].shader = litShader;
}

void lighting::Detach(Model &model)
{
    for (int i = 0; i < model.materialCount; ++i)
    {
        model.materials[i].shader.id = rlGetShaderIdDefault();
        model.materials[i].shader.locs = rlGetShaderLocsDefault();
    }
}
