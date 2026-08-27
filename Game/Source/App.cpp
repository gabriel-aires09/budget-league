#include "App.h"

#include "CarSelectScene.h"
#include "Lighting.h"
#include "MainMenuScene.h"
#include "MatchScene.h"
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

    InitWindow(windowWidth, windowHeight, "Arcade Car Soccer");
    if (!IsWindowReady())
        return false;

    SetTargetFPS(targetFps);
    SetExitKey(KEY_NULL); // Esc pauses the match instead of closing the game

    // Jolt global setup. Each scene creates its own PhysicsSystem.
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    lighting::Load();

#ifdef GAME_DEV_TOOLS
    ImGui::CreateContext();
#endif

    // The smoke test goes straight to the match, which is what needs validating.
    if (smokeTest)
        SetScene(new MatchScene());
    else
        SetScene(new MainMenuScene());

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
        activeScene->Update(GetFrameTime());

        BeginDrawing();
        ClearBackground(uistyle::Background);
        activeScene->Draw();
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
    ImGui::DestroyContext();
#endif

    // After the scenes, so every model has already detached from the shader.
    lighting::Unload();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    CloseWindow();
}
