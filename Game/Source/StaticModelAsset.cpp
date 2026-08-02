#include "StaticModelAsset.h"

#include <raymath.h>

#include <cstring>

std::string assets::Path(const std::string &relative)
{
    return std::string(GetApplicationDirectory()) + "assets/" + relative;
}

namespace
{
    const char MODEL_MAGIC[8] = { 'E', 'V', 'M', 'D', 'M', 'S', 'H', '1' };

    // Cursor over the file bytes. Every read is bounds checked and the first
    // failure latches, so the parse below can run straight through and only
    // check ok once at the end.
    struct Reader
    {
        const unsigned char *data;
        int size;
        int offset = 0;
        bool ok = true;

        bool Take(void *out, int bytes)
        {
            if (!ok || bytes < 0 || offset + bytes > size)
            {
                ok = false;
                return false;
            }
            memcpy(out, data + offset, (size_t)bytes);
            offset += bytes;
            return true;
        }

        unsigned int U32()
        {
            unsigned int value = 0;
            Take(&value, 4);
            return value;
        }
    };
}

bool StaticModelAsset::Load(const std::string &name)
{
    Unload();

    std::string path = assets::Path("Models/" + name + ".evmodel");
    int size = 0;
    unsigned char *data = LoadFileData(path.c_str(), &size);
    if (data == nullptr)
    {
        TraceLog(LOG_WARNING, "MODEL: could not read %s", path.c_str());
        return false;
    }

    Reader reader = { data, size };

    char magic[8] = {};
    reader.Take(magic, 8);
    if (!reader.ok || memcmp(magic, MODEL_MAGIC, sizeof(magic)) != 0)
    {
        TraceLog(LOG_WARNING, "MODEL: %s is not an evmodel file", path.c_str());
        UnloadFileData(data);
        return false;
    }

    unsigned int materialCount = reader.U32();
    unsigned int meshCount = reader.U32();
    reader.Take(&bounds.min, 12);
    reader.Take(&bounds.max, 12);

    std::vector<MaterialInfo> readMaterials;
    for (unsigned int i = 0; i < materialCount && reader.ok; ++i)
    {
        unsigned int nameLength = reader.U32();
        reader.offset += (int)nameLength; // the material name is cooker-side documentation
        if (reader.offset > reader.size)
            reader.ok = false;

        unsigned char record[8] = {};
        reader.Take(record, 8);

        MaterialInfo material;
        material.baked = Color{ record[0], record[1], record[2], record[3] };
        material.paint = record[4] != 0;
        material.shade = record[5] / 255.0f;
        readMaterials.push_back(material);
    }

    // Meshes are triangle soups, so vertexCount is also the triangle count x 3.
    std::vector<Mesh> readMeshes;
    std::vector<int> meshMaterial;
    for (unsigned int i = 0; i < meshCount && reader.ok; ++i)
    {
        unsigned int materialIndex = reader.U32();
        unsigned int vertexCount = reader.U32();
        if (!reader.ok || materialIndex >= materialCount || vertexCount == 0 || vertexCount % 3 != 0)
        {
            reader.ok = false;
            break;
        }

        int floatBytes = (int)vertexCount * 3 * (int)sizeof(float);
        Mesh mesh = {};
        mesh.vertexCount = (int)vertexCount;
        mesh.triangleCount = (int)vertexCount / 3;
        mesh.vertices = (float *)MemAlloc((unsigned int)floatBytes);
        mesh.normals = (float *)MemAlloc((unsigned int)floatBytes);
        reader.Take(mesh.vertices, floatBytes);
        reader.Take(mesh.normals, floatBytes);

        readMeshes.push_back(mesh);
        meshMaterial.push_back((int)materialIndex);
    }

    if (!reader.ok || readMeshes.empty() || readMaterials.size() != materialCount)
    {
        TraceLog(LOG_WARNING, "MODEL: %s is truncated or malformed", path.c_str());
        for (Mesh &mesh : readMeshes)
        {
            MemFree(mesh.vertices);
            MemFree(mesh.normals);
        }
        UnloadFileData(data);
        return false;
    }

    UnloadFileData(data);

    model = {};
    model.transform = MatrixIdentity();
    model.meshCount = (int)readMeshes.size();
    model.meshes = (Mesh *)MemAlloc((unsigned int)(readMeshes.size() * sizeof(Mesh)));
    model.meshMaterial = (int *)MemAlloc((unsigned int)(readMeshes.size() * sizeof(int)));
    model.materialCount = (int)materialCount;
    model.materials = (Material *)MemAlloc((unsigned int)(materialCount * sizeof(Material)));

    for (size_t i = 0; i < readMeshes.size(); ++i)
    {
        UploadMesh(&readMeshes[i], false);
        model.meshes[i] = readMeshes[i];
        model.meshMaterial[i] = meshMaterial[i];
    }

    for (unsigned int i = 0; i < materialCount; ++i)
    {
        model.materials[i] = LoadMaterialDefault();
        model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = readMaterials[i].baked;
    }

    materials = readMaterials;
    loaded = true;
    TraceLog(LOG_INFO, "MODEL: loaded %s (%d meshes, %d materials)",
             path.c_str(), model.meshCount, model.materialCount);
    return true;
}

void StaticModelAsset::Unload()
{
    if (loaded)
        UnloadModel(model);

    model = {};
    bounds = {};
    materials.clear();
    loaded = false;
}

void StaticModelAsset::SetPaintColor(Color color)
{
    for (size_t i = 0; i < materials.size(); ++i)
    {
        if (!materials[i].paint)
            continue;

        // The shade keeps a two-tone paint job two-tone once it is repainted.
        model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = Color{
            (unsigned char)(color.r * materials[i].shade),
            (unsigned char)(color.g * materials[i].shade),
            (unsigned char)(color.b * materials[i].shade),
            color.a
        };
    }
}
