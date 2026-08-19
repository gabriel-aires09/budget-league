#include "TextureAsset.h"

#include "StaticModelAsset.h" // assets::Path, so there is one scheme for both

#include <cstring>

namespace
{
    const char TEXTURE_MAGIC[8] = { 'E', 'V', 'T', 'X', 'Q', 'O', 'I', '1' };

    struct Pixel
    {
        unsigned char r, g, b, a;
    };

    // The QOI decoder, mirroring EncodeQoi in Tools/AssetCooker.py. The stream
    // carries no header and no end marker: the cooker's own header has the size,
    // and the loop stops at pixelCount.
    bool DecodeQoi(const unsigned char *stream, int streamBytes, int pixelCount, Pixel *out)
    {
        Pixel table[64] = {};
        Pixel pixel = { 0, 0, 0, 255 };
        int at = 0;

        for (int written = 0; written < pixelCount;)
        {
            if (at >= streamBytes)
                return false;
            unsigned char tag = stream[at++];

            if (tag == 0xfe) // RGB
            {
                if (at + 3 > streamBytes)
                    return false;
                pixel.r = stream[at++];
                pixel.g = stream[at++];
                pixel.b = stream[at++];
            }
            else if (tag == 0xff) // RGBA
            {
                if (at + 4 > streamBytes)
                    return false;
                pixel.r = stream[at++];
                pixel.g = stream[at++];
                pixel.b = stream[at++];
                pixel.a = stream[at++];
            }
            else if ((tag >> 6) == 0) // index
            {
                pixel = table[tag & 0x3f];
            }
            else if ((tag >> 6) == 1) // diff, each channel -2..1
            {
                pixel.r += (unsigned char)(((tag >> 4) & 3) - 2);
                pixel.g += (unsigned char)(((tag >> 2) & 3) - 2);
                pixel.b += (unsigned char)((tag & 3) - 2);
            }
            else if ((tag >> 6) == 2) // luma
            {
                if (at >= streamBytes)
                    return false;
                int dg = (tag & 0x3f) - 32;
                unsigned char extra = stream[at++];
                pixel.r += (unsigned char)(dg - 8 + (extra >> 4));
                pixel.g += (unsigned char)dg;
                pixel.b += (unsigned char)(dg - 8 + (extra & 0x0f));
            }
            else // run of the previous pixel, 1..62
            {
                int run = (tag & 0x3f) + 1;
                if (written + run > pixelCount)
                    return false;
                for (int i = 0; i < run; ++i)
                    out[written++] = pixel;
                continue;
            }

            table[(pixel.r * 3 + pixel.g * 5 + pixel.b * 7 + pixel.a * 11) % 64] = pixel;
            out[written++] = pixel;
        }

        return true;
    }
}

bool LoadCookedImage(const std::string &name, Image &image)
{
    std::string path = assets::Path("Textures/" + name + ".evtex");
    int size = 0;
    unsigned char *data = LoadFileData(path.c_str(), &size);
    if (data == nullptr)
    {
        TraceLog(LOG_WARNING, "TEXTURE: could not read %s", path.c_str());
        return false;
    }

    // Magic plus four uint32 fields: width, height, channels, payload bytes.
    const int HEADER_BYTES = 24;
    unsigned int header[4] = {};
    if (size < HEADER_BYTES || memcmp(data, TEXTURE_MAGIC, sizeof(TEXTURE_MAGIC)) != 0)
    {
        TraceLog(LOG_WARNING, "TEXTURE: %s is not an evtex file", path.c_str());
        UnloadFileData(data);
        return false;
    }
    memcpy(header, data + 8, sizeof(header));

    int width = (int)header[0];
    int height = (int)header[1];
    int payload = (int)header[3];
    if (width <= 0 || height <= 0 || header[2] != 4 || payload != size - HEADER_BYTES)
    {
        TraceLog(LOG_WARNING, "TEXTURE: %s has a bad header", path.c_str());
        UnloadFileData(data);
        return false;
    }

    // Allocated the way raylib allocates an Image, because UnloadImage is what
    // frees it: a std::vector here would be freed twice or not at all.
    Pixel *pixels = (Pixel *)RL_MALLOC((size_t)width * (size_t)height * sizeof(Pixel));
    bool decoded = pixels != nullptr && DecodeQoi(data + HEADER_BYTES, payload, width * height, pixels);
    UnloadFileData(data);
    if (!decoded)
    {
        RL_FREE(pixels);
        TraceLog(LOG_WARNING, "TEXTURE: %s is truncated or corrupt", path.c_str());
        return false;
    }

    image = Image{};
    image.data = pixels;
    image.width = width;
    image.height = height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    return true;
}

bool TextureAsset::Load(const std::string &name)
{
    Unload();

    Image image = {};
    if (!LoadCookedImage(name, image))
        return false;

    texture = LoadTextureFromImage(image);
    UnloadImage(image); // nothing keeps a copy in RAM once it is on the GPU
    if (!IsTextureValid(texture))
    {
        TraceLog(LOG_WARNING, "TEXTURE: %s could not be uploaded", name.c_str());
        return false;
    }

    // The logo is drawn scaled to the window, so it is always resampled.
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    loaded = true;
    TraceLog(LOG_INFO, "TEXTURE: loaded %s (%ix%i)", name.c_str(), texture.width, texture.height);
    return true;
}

void TextureAsset::Unload()
{
    if (!loaded)
        return;

    UnloadTexture(texture);
    texture = {};
    loaded = false;
}
