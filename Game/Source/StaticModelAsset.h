#ifndef STATICMODELASSET_H
#define STATICMODELASSET_H

#include <raylib.h>

#include <string>
#include <vector>

// One cooked .evmodel: a raylib Model plus the bookkeeping needed to repaint
// the car panels in a team colour (CLAUDE.md 4.1). The cooker writes one mesh
// per material, so a repaint is just a diffuse colour per material.
struct StaticModelAsset
{
    struct MaterialInfo
    {
        Color baked;  // colour as authored
        bool paint;   // true when the team colour replaces it
        float shade;  // brightness relative to the brightest paint material
    };

    bool Load(const std::string &name); // "SportsCar" -> assets/Models/SportsCar.evmodel
    void Unload();
    // Repaints every paint material, keeping their relative shading.
    void SetPaintColor(Color color);

    Model model = {};
    BoundingBox bounds = {}; // model space, metres
    bool loaded = false;
    std::vector<MaterialInfo> materials;
};

#endif
