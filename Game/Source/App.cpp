#include "App.h"

#include "AudioSystem.h"
#include "CarSelectScene.h"
#include "HowToPlayScene.h"
#include "Lighting.h"
#include "ImGuiRaylib.h"
#include "MainMenuScene.h"
#include "PostProcess.h"
#include "MatchScene.h"
#include "TitleScene.h"
#include "UserInterface.h"

#include <raylib.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <imgui.h>

#include <cstdlib>
#include <cstring>

bool App::Initialize(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--smoke-test") == 0)
        {
            smokeTest = true;
            if (i + 1 < argc && argv[i + 1][0] != '-')
                smokeTestFrames = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc)
        {
            screenshotPath = argv[++i];
        }
    }

    InitWindow(windowWidth, windowHeight, "Budget League");
    if (!IsWindowReady())
        return false;

    // Open fullscreen, at whatever the monitor actually is. The window has to be
    // resized to the monitor first: ToggleFullscreen keeps the current size as
    // the fullscreen resolution, so without this the game would fill the screen
    // with a 1280x720 image. IsWindowFullscreen is what the settings panel reads,
    // so this is the same ToggleFullscreen it uses rather than the borderless
    // variant, and the Fullscreen row is in step from the first frame.
    //
    // The smoke test deliberately stays windowed: it exists to render a fixed
    // 1280x720 frame and write it out, and its screenshots would otherwise change
    // size with whatever machine ran it.
    if (settings.fullscreen && !smokeTest)
    {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    }

    SetTargetFPS(targetFps);
    SetExitKey(KEY_NULL); // Esc pauses the match instead of closing the game

    // Jolt global setup. Each scene creates its own PhysicsSystem.
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    lighting::Load();
    postprocess::Load();
    audio::Load();

#ifdef GAME_DEV_TOOLS
    imgui::Initialize();
#endif

    // The smoke test goes straight to the match, which is what needs validating.
    // Everything else boots on the title screen: TitleScene -> MainMenuScene.
    if (smokeTest)
        SetScene(new MatchScene());
    else
        SetScene(new TitleScene());

    return true;
}

void App::SetScene(Scene *scene)
{
    if (activeScene != nullptr)
    {
        activeScene->Shutdown();
        delete activeScene;
    }

    activeScene = scene;
    activeScene->settings = &settings;
    activeScene->Initialize();
}

void App::Run()
{
    int frame = 0;
    while (running && !WindowShouldClose())
    {
#ifdef GAME_DEV_TOOLS
        // Before the scene, so its panels can call ImGui while they draw.
        imgui::BeginFrame(GetFrameTime());
#endif
        // Read every frame rather than at Initialize, so both volume sliders are
        // heard as they move, exactly like the camera sensitivity.
        audio::SetVolumes(settings.masterVolume, settings.sfxVolume, settings.musicVolume);
        audio::UpdateMusic(); // refills the stream and moves the playlist on
        activeScene->Update(GetFrameTime());

        BeginDrawing();
        ClearBackground(uistyle::Background);
        activeScene->Draw();
#ifdef GAME_DEV_TOOLS
        imgui::EndFrame(); // on top of the game, still inside BeginDrawing
#endif
        EndDrawing();

        if (smokeTest && ++frame >= smokeTestFrames)
        {
            TakeScreenshot(screenshotPath.c_str());
            break;
        }

        // Scene changes happen after drawing, so no frame draws a dead scene.
        MenuAction action = activeScene->pendingAction;
        activeScene->pendingAction = MenuAction::None;
        switch (action)
        {
        case MenuAction::HowToPlay:  SetScene(new HowToPlayScene()); break;
        case MenuAction::SelectCar:  SetScene(new CarSelectScene()); break;
        case MenuAction::StartMatch: SetScene(new MatchScene()); break;
        case MenuAction::MainMenu:   SetScene(new MainMenuScene()); break;
        case MenuAction::ExitGame:   running = false; break;
        case MenuAction::None:       break;
        }
    }
}

void App::Shutdown()
{
    if (activeScene != nullptr)
    {
        activeScene->Shutdown();
        delete activeScene;
        activeScene = nullptr;
    }

#ifdef GAME_DEV_TOOLS
    imgui::Shutdown();
#endif

    // After the scenes, so every model has already detached from the shader.
    audio::Unload();
    postprocess::Unload();
    lighting::Unload();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    CloseWindow();
}
