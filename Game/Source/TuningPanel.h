#ifndef TUNINGPANEL_H
#define TUNINGPANEL_H

#include <vector>

class MatchScene;

// The live tuning panel, Debug and Development only (Milestone 13). F1 opens it
// and freezes the match while it is up.
//
// Every slider points straight at the field it tunes on the live object, so
// there is no second copy of the tuning to keep in step. One table of entries
// drives all three of drawing the panel, saving the config and loading it back,
// which is what stops those three from drifting apart.
struct TuningPanel
{
    struct Entry
    {
        const char *section;
        const char *name;
        float *value;
        // The same field on the bot's car, for the handling entries. Tuning has
        // to move both or the bot ends up driving a different car from the
        // player. Null for everything that is not per car.
        float *mirror;
        float minimum;
        float maximum;
    };

    // Collects the entries from the scene's objects and applies the saved config
    // on top of them. Call once the scene's cars, ball and pads exist.
    void Initialize(MatchScene &scene);
    void Draw(MatchScene &scene);
    bool Save() const;

    bool open = false;
    std::vector<Entry> entries;

    // These three are not a field of anything, so they are held here and pushed
    // into the scene when they change.
    float gravity = -9.81f;
    float smallPadRefill = 12.0f;
    float bigPadRefill = 100.0f;
};

#endif
