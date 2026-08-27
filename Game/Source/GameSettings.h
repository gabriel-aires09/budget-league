#ifndef GAMESETTINGS_H
#define GAMESETTINGS_H

#include <string>

// Shared by the main menu and the pause menu; App owns the single instance.
struct GameSettings
{
    // Picked in CarSelectScene rather than in the settings panel, but it lives
    // here for the same reason everything else does: it is the one struct every
    // scene can see, so the choice survives from the picker to the match and
    // back to the menu.
    std::string playerCarModel = "SportsCar";

    // Graphics
    // The game opens fullscreen. App::Initialize is what acts on this at launch;
    // SettingsMenu::ApplyGraphics is what acts on it afterwards.
    bool fullscreen = true;
    int resolutionIndex = 0; // 0 = custom, meaning whatever size the window has
    bool postProcessing = true;

    // Gameplay
    float cameraSensitivity = 1.0f;
    int matchDurationMinutes = 5;
    bool botEnabled = true;

    // Controls
    bool gamepadEnabled = true;
    // Radial, on both sticks. 0.15 is the usual resting slop of an Xbox pad.
    float stickDeadZone = 0.15f;
    bool vibrationEnabled = true;

    // Audio
    int masterVolume = 80;
    int sfxVolume = 80;
    int musicVolume = 60; // the soundtrack sits under the effects by default
};

#endif
