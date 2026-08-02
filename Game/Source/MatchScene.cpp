#include "MatchScene.h"

#include <cmath>

void MatchScene::Initialize()
{
    InitializePhysics();

    arena.Initialize(*this);
    ball.Initialize(*this);
    playerCar.controller = &playerController;
    playerCar.Initialize(*this);

    objects.push_back(&arena);
    objects.push_back(&ball);
    objects.push_back(&playerCar);

    physicsSystem.OptimizeBroadPhase();
    chaseCamera.Initialize(camera, playerCar);
}

void MatchScene::Update(float deltaTime)
{
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
    {
        if (settingsOpen)
            settingsOpen = false;
        else
            paused = !paused;
    }

    if (paused)
        return;

    // R re-centres the ball as well as the car. Until the arena is closed in
    // Milestone 06 the ball can be knocked off the floor, and without this
    // there would be no way to get it back.
    if (IsKeyPressed(KEY_R))
        ball.ResetTo(ball.spawnPosition);

    StepPhysics(deltaTime);
    chaseCamera.Update(camera, playerCar, deltaTime);
}

void MatchScene::Draw()
{
    BeginMode3D(camera);
    for (GameObject *object : objects)
        object->Draw();
    EndMode3D();

    uistyle::DrawTextAt(TextFormat("SPEED %3.0f km/h", fabsf(playerCar.GetForwardSpeed()) * 3.6f),
                        20.0f, 20.0f, 24.0f, uistyle::Text);
    uistyle::DrawTextAt(playerCar.grounded ? "GROUNDED" : "AIRBORNE", 20.0f, 52.0f, 18.0f, uistyle::TextDim);
    uistyle::DrawTextAt("WASD / arrows drive - R resets car and ball - Esc pauses",
                        20.0f, GetScreenHeight() - 32.0f, 18.0f, uistyle::TextDim);

    if (!paused)
        return;

    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;
    uistyle::DrawDimmer(0.65f);

    if (settingsOpen)
    {
        Rectangle area = { centerX - SettingsMenu::PreferredWidth() * 0.5f, 105.0f * scale,
                           SettingsMenu::PreferredWidth(), SettingsMenu::PreferredHeight() };
        settingsOpen = settingsMenu.Draw(*settings, area);
        return;
    }

    uistyle::DrawTitle("PAUSED", centerX, 130.0f * scale, 44.0f);

    const float rowWidth = 340.0f * scale;
    const float rowHeight = 46.0f * scale;
    const float rowGap = 12.0f * scale;
    Rectangle row = { centerX - rowWidth * 0.5f, 250.0f * scale, rowWidth, rowHeight };

    pauseMenu.itemCount = 4;
    pauseMenu.Update();

    if (pauseMenu.Item(row, "Resume", 0))
        paused = false;
    row.y += rowHeight + rowGap;

    if (pauseMenu.Item(row, "Settings", 1))
        settingsOpen = true;
    row.y += rowHeight + rowGap;

    if (pauseMenu.Item(row, "Return to main menu", 2))
        pendingAction = MenuAction::MainMenu;
    row.y += rowHeight + rowGap;

    if (pauseMenu.Item(row, "Exit game", 3))
        pendingAction = MenuAction::ExitGame;
}

void MatchScene::Shutdown()
{
    JPH::BodyInterface &bodies = physicsSystem.GetBodyInterface();
    for (GameObject *object : objects)
    {
        if (!object->bodyID.IsInvalid())
        {
            bodies.RemoveBody(object->bodyID);
            bodies.DestroyBody(object->bodyID);
        }
    }
    objects.clear();
}
