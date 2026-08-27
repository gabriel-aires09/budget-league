#ifndef POSTPROCESS_H
#define POSTPROCESS_H

// Optional bloom over the 3D scene, toggled by GameSettings::postProcessing.
//
// It degrades gracefully in both directions: with the shaders missing it reports
// itself unavailable and the scene draws straight to the screen, and with the
// setting off nothing is allocated beyond the two shaders.
namespace postprocess
{
    // Needs a window. Safe to call when the shaders are missing.
    void Load();
    void Unload();
    bool Available();

    // Wrap the 3D pass. Begin returns true when the scene is being captured, in
    // which case End resolves it; when it returns false nothing was redirected
    // and End does nothing, so the caller needs no branch of its own.
    bool Begin(bool enabled);
    void End();
}

#endif
