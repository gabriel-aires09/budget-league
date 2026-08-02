#ifndef LIGHTING_H
#define LIGHTING_H

#include <raylib.h>

// One directional key light plus a hemisphere fill, shared by every 3D model.
// The shading is flat and computed in the shader, never baked into the meshes.
namespace lighting
{
    // Needs a window, so call it after InitWindow. If the shader fails to load
    // the game keeps running with raylib's unlit default.
    void Load();
    void Unload();

    // Points every material of a model at the lit shader.
    void Apply(Model &model);
    // MUST be called before UnloadModel on any model that went through Apply.
    // UnloadModel unloads each material's shader, which would destroy the one
    // shared lit shader and leave every other model drawing with a dead program.
    void Detach(Model &model);
}

#endif
