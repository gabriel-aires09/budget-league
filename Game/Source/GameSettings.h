#ifndef GAMESETTINGS_H
#define GAMESETTINGS_H

// Shared by the main menu and the pause menu; App owns the single instance.
struct GameSettings
{
    // Graphics
    bool fullscreen = false;
    int resolutionIndex = 0; // 0 = custom, meaning whatever size the window has
    bool postProcessing = true;

    // Gameplay
    float cameraSensitivity = 1.0f;
    int matchDurationMinutes = 5;
    bool botEnabled = true;

    // Audio
    int masterVolume = 80;
    int sfxVolume = 80;
};

#endif
