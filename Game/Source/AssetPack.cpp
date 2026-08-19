#include "AssetPack.h"

#include <raylib.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <unordered_map>

namespace
{
    const char PAK_MAGIC[8] = { 'E', 'V', 'P', 'A', 'K', '0', '0', '1' };
    const int PAK_HEADER_BYTES = 16;

    enum Compression
    {
        Stored = 0,
        Deflate = 1
    };

    struct Entry
    {
        unsigned int offset = 0;
        unsigned int storedBytes = 0;
        unsigned int originalBytes = 0;
        unsigned char compression = Stored;
    };

    // The archive is kept open rather than read into memory: the soundtrack
    // alone is 32 MB, and holding all of it resident to play one track at a time
    // would be worse than the loose folder it replaces.
    FILE *pak = nullptr;
    std::unordered_map<std::string, Entry> directory;

    unsigned int ReadU32(const unsigned char *at)
    {
        unsigned int value = 0;
        memcpy(&value, at, sizeof(value));
        return value;
    }
}

void assets::Mount()
{
    Unmount();

    std::string path = std::string(GetApplicationDirectory()) + "assets.pak";
    pak = fopen(path.c_str(), "rb");
    if (pak == nullptr)
    {
        TraceLog(LOG_INFO, "PAK: no assets.pak, reading the loose assets folder");
        return;
    }

    unsigned char header[PAK_HEADER_BYTES] = {};
    if (fread(header, 1, PAK_HEADER_BYTES, pak) != PAK_HEADER_BYTES ||
        memcmp(header, PAK_MAGIC, sizeof(PAK_MAGIC)) != 0)
    {
        TraceLog(LOG_WARNING, "PAK: %s is not an evpak file, falling back to the assets folder", path.c_str());
        Unmount();
        return;
    }

    unsigned int entryCount = ReadU32(header + 8);
    unsigned int directoryOffset = ReadU32(header + 12);

    // The whole directory in one read: it is a few kilobytes and seeking per
    // entry to build a map is the one part of this worth not doing lazily.
    if (fseek(pak, (long)directoryOffset, SEEK_SET) != 0)
    {
        TraceLog(LOG_WARNING, "PAK: %s has no directory at %u", path.c_str(), directoryOffset);
        Unmount();
        return;
    }

    for (unsigned int i = 0; i < entryCount; ++i)
    {
        unsigned short nameLength = 0;
        if (fread(&nameLength, sizeof(nameLength), 1, pak) != 1 || nameLength == 0)
            break;

        std::string name(nameLength, '\0');
        unsigned char tail[13] = {}; // offset, storedBytes, originalBytes, compression
        if (fread(&name[0], 1, nameLength, pak) != nameLength ||
            fread(tail, 1, sizeof(tail), pak) != sizeof(tail))
            break;

        Entry entry;
        entry.offset = ReadU32(tail);
        entry.storedBytes = ReadU32(tail + 4);
        entry.originalBytes = ReadU32(tail + 8);
        entry.compression = tail[12];
        directory[name] = entry;
    }

    if (directory.size() != entryCount)
    {
        TraceLog(LOG_WARNING, "PAK: %s is truncated (%i of %u entries), falling back to the assets folder",
                 path.c_str(), (int)directory.size(), entryCount);
        Unmount();
        return;
    }

    TraceLog(LOG_INFO, "PAK: mounted %s, %i asset(s)", path.c_str(), (int)directory.size());
}

void assets::Unmount()
{
    if (pak != nullptr)
        fclose(pak);
    pak = nullptr;
    directory.clear();
}

bool assets::Mounted()
{
    return pak != nullptr;
}

std::string assets::Path(const std::string &relative)
{
    return std::string(GetApplicationDirectory()) + "assets/" + relative;
}

unsigned char *assets::LoadData(const std::string &relative, int *size)
{
    if (size != nullptr)
        *size = 0;

    if (pak == nullptr)
        return LoadFileData(Path(relative).c_str(), size);

    auto found = directory.find(relative);
    if (found == directory.end())
    {
        TraceLog(LOG_WARNING, "PAK: %s is not in the archive", relative.c_str());
        return nullptr;
    }

    const Entry &entry = found->second;
    unsigned char *stored = (unsigned char *)RL_MALLOC(entry.storedBytes);
    if (stored == nullptr)
        return nullptr;

    if (fseek(pak, (long)entry.offset, SEEK_SET) != 0 ||
        fread(stored, 1, entry.storedBytes, pak) != entry.storedBytes)
    {
        RL_FREE(stored);
        TraceLog(LOG_WARNING, "PAK: could not read %s", relative.c_str());
        return nullptr;
    }

    if (entry.compression == Stored)
    {
        if (size != nullptr)
            *size = (int)entry.storedBytes;
        return stored;
    }

    // Raw DEFLATE, which is what Tools/PakWriter.py writes and what raylib's own
    // decompressor reads.
    int decompressed = 0;
    unsigned char *data = DecompressData(stored, (int)entry.storedBytes, &decompressed);
    RL_FREE(stored);
    if (data == nullptr || decompressed != (int)entry.originalBytes)
    {
        RL_FREE(data);
        TraceLog(LOG_WARNING, "PAK: %s did not decompress to its stated size", relative.c_str());
        return nullptr;
    }

    if (size != nullptr)
        *size = decompressed;
    return data;
}

void assets::UnloadData(unsigned char *data)
{
    RL_FREE(data);
}

std::string assets::LoadText(const std::string &relative)
{
    int size = 0;
    unsigned char *data = LoadData(relative, &size);
    if (data == nullptr)
        return std::string();

    // The cooked shaders are text but are stored as bytes, with no terminator of
    // their own, so the length is what says where they end.
    std::string text((const char *)data, (size_t)size);
    UnloadData(data);
    return text;
}

std::vector<std::string> assets::List(const std::string &folder, const std::string &extension)
{
    std::vector<std::string> found;
    std::string prefix = folder + "/";

    if (pak != nullptr)
    {
        for (const auto &entry : directory)
        {
            const std::string &name = entry.first;
            if (name.size() <= prefix.size() || name.compare(0, prefix.size(), prefix) != 0)
                continue;
            if (name.size() < extension.size() ||
                name.compare(name.size() - extension.size(), extension.size(), extension) != 0)
                continue;
            found.push_back(name);
        }
        // An unordered_map hands them back in whatever order it likes, and the
        // soundtrack picks by index: sorted keeps a build's playlist stable.
        std::sort(found.begin(), found.end());
        return found;
    }

    FilePathList files = LoadDirectoryFilesEx(Path(folder).c_str(), extension.c_str(), false);
    for (unsigned int i = 0; i < files.count; ++i)
        found.push_back(prefix + GetFileName(files.paths[i]));
    UnloadDirectoryFiles(files);
    std::sort(found.begin(), found.end());
    return found;
}
