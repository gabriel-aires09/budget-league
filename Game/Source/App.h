#ifndef APP_H
#define APP_H

#include <string>

#include "GameSettings.h"
#include "MenuAction.h"

class Scene;

// Owns the window, the settings, the active scene and the game loop, and
// coordinates the smoke test (run N frames, write a screenshot, exit).
class App
{
public:
    bool Initialize(int argc, char **argv);
    void Run();
    void Shutdown();

    // Tears down the current scene and takes ownership of the new one.
    void SetScene(Scene *scene);

    int windowWidth = 1280;
    int windowHeight = 720;
    int targetFps = 60;

    GameSettings settings;
    Scene *activeScene = nullptr;
    bool running = true;

    bool smokeTest = false;
    int smokeTestFrames = 90;
    std::string screenshotPath = "SmokeTest.png";
};

#endif
