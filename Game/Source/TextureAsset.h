#ifndef TEXTUREASSET_H
#define TEXTUREASSET_H

#include <raylib.h>

#include <string>

// One cooked .evtex: the texture format from CLAUDE.md 3.2, an RGBA8 image in a
// QOI chunk stream. It is decoded straight into a buffer and handed to the GPU,
// so there is nothing to configure and nothing kept in RAM afterwards.
struct TextureAsset
{
    bool Load(const std::string &name); // "budget-league-logo" -> assets/Textures/<name>.evtex
    void Unload();

    Texture2D texture = {};
    bool loaded = false;
};

#endif
