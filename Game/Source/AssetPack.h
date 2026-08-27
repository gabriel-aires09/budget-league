#ifndef ASSETPACK_H
#define ASSETPACK_H

#include <string>
#include <vector>

// Where every cooked asset is read from, and the only place that knows there are
// two ways to read one.
//
// A build produces both: the loose `assets/` folder the cooker writes, and
// `assets.pak`, one file holding the same bytes (Tools/PakWriter.py has the
// layout). The archive wins whenever it is there, so the shipped path is the one
// exercised in development too, and a build that has never been packed — or a
// folder the archive was deleted from — still runs from the loose files.
//
// Nothing here is fatal. A missing asset comes back empty and its caller draws
// the stand-in it already had.
namespace assets
{
    // Opens assets.pak beside the executable if there is one. Called once, before
    // anything loads.
    void Mount();
    void Unmount();
    bool Mounted();

    // The loose path of a cooked asset, for the fallback and for logging.
    std::string Path(const std::string &relative);

    // Bytes of one asset, decompressed if the archive holds it compressed.
    // Returns null when it is not there. Free it with UnloadData.
    unsigned char *LoadData(const std::string &relative, int *size);
    void UnloadData(unsigned char *data);

    // The same, as a string, for the shader sources. Empty when it is missing.
    std::string LoadText(const std::string &relative);

    // Every asset in a folder with that extension, as relative paths — the
    // soundtrack is found by looking rather than by a list in the code, and it
    // has to keep working when the folder is inside the archive.
    std::vector<std::string> List(const std::string &folder, const std::string &extension);
}

#endif
