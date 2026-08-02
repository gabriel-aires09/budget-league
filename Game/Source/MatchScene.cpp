#include "MatchScene.h"

#include <cmath>

void MatchScene::Initialize()
{
    InitializePhysics();

    arena.Initialize(*this);
    ball.Initialize(*this);

    // Both goals are built from the arena's own opening size, so the net can
    // never end up a different shape from the hole in the back wall.
    // Blue defends +Z, which is the end the player kicks off in front of.
    blueGoal.lineZ = arena.length * 0.5f;
    blueGoal.direction = 1.0f;
    blueGoal.defendingTeam = 0;
    blueGoal.teamColor = uistyle::TeamBlue;
    orangeGoal.lineZ = -arena.length * 0.5f;
    orangeGoal.direction = -1.0f;
    orangeGoal.defendingTeam = 1;
    orangeGoal.teamColor = uistyle::TeamOrange;
    for (GoalObject *goal : { &blueGoal, &orangeGoal })
    {
        goal->width = arena.goalWidth;
        goal->height = arena.goalHeight;
        goal->Initialize(*this);
    }

    playerCar.spawnPosition = Vector3{ 0.0f, 0.36f, arena.length * 0.3f };
    playerCar.controller = &playerController;
    playerCar.Initialize(*this);

    BuildBoostPads();

    objects.push_back(&arena);
    objects.push_back(&blueGoal);
    objects.push_back(&orangeGoal);
    objects.push_back(&ball);
    objects.push_back(&playerCar);
    for (BoostPadObject &pad : boostPads)
    {
        pad.cars.push_back(&playerCar);
        pad.Initialize(*this);
        objects.push_back(&pad);
    }

    physicsSystem.OptimizeBroadPhase();
    chaseCamera.Initialize(camera, playerCar);

    match.AddCar(playerCar);
    match.Begin(ball, blueGoal, orangeGoal, (float)settings->matchDurationMinutes);
}

// Four full pads out wide plus a scatter of small ones, so crossing the field
// the long way is always a choice between the quick route and the fed route.
void MatchScene::BuildBoostPads()
{
    const float smallRows[] = { -28.0f, -14.0f, 14.0f, 28.0f };
    const float smallColumns[] = { -16.0f, 0.0f, 16.0f };

    // Sized up front: see the note on boostPads in the header.
    boostPads.reserve(4 + 3 * 4 + 2);

    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
        {
            BoostPadObject pad;
            pad.position = Vector3{ sideX * 20.0f, 0.0f, sideZ * 32.0f };
            pad.radius = 3.0f;
            pad.refillAmount = 100.0f;
            pad.cooldownTime = 10.0f;
            boostPads.push_back(pad);
        }
    }

    for (float z : smallRows)
    {
        for (float x : smallColumns)
        {
            BoostPadObject pad;
            pad.position = Vector3{ x, 0.0f, z };
            boostPads.push_back(pad);
        }
    }

    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        BoostPadObject pad;
        pad.position = Vector3{ sideX * 23.0f, 0.0f, 0.0f };
        boostPads.push_back(pad);
    }
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

    match.Update(deltaTime);

    // The kickoff countdown freezes the field: everything was just re-centred
    // with zero velocity, so simply not stepping holds it there.
    if (!match.IsFrozen())
        StepPhysics(deltaTime);

    chaseCamera.Update(camera, playerCar, deltaTime);
}

// Score, clock and the state banners. This is deliberately minimal: the real HUD
// (boost meter, pad hints, celebration art) is Milestone 10.
void MatchScene::DrawMatchStatus()
{
    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;

    int seconds = (int)ceilf(match.timeRemaining);
    uistyle::DrawTextCentered(TextFormat("%d : %02d", seconds / 60, seconds % 60),
                              centerX, 16.0f * scale, 30.0f, uistyle::Text);
    uistyle::DrawTextCentered(TextFormat("%d", match.scoreBlue), centerX - 90.0f * scale,
                              14.0f * scale, 40.0f, uistyle::TeamBlue);
    uistyle::DrawTextCentered(TextFormat("%d", match.scoreOrange), centerX + 90.0f * scale,
                              14.0f * scale, 40.0f, uistyle::TeamOrange);

    if (match.state == MatchState::Kickoff)
    {
        // Counts 3, 2, 1 and then GO for the last moment before the field unfreezes.
        int count = (int)ceilf(match.stateTimer);
        uistyle::DrawTitle(count > 0 ? TextFormat("%d", count) : "GO", centerX, 200.0f * scale, 72.0f);
    }
    else if (match.state == MatchState::Celebration)
    {
        Color color = match.lastScoringTeam == 0 ? uistyle::TeamBlue : uistyle::TeamOrange;
        uistyle::DrawTextCentered("GOAL!", centerX, 190.0f * scale, 76.0f, color);
        uistyle::DrawTextCentered(match.lastScoringTeam == 0 ? "BLUE SCORES" : "ORANGE SCORES",
                                  centerX, 270.0f * scale, 26.0f, uistyle::Text);
    }
    else if (match.state == MatchState::Finished)
    {
        uistyle::DrawTitle("FULL TIME", centerX, 190.0f * scale, 60.0f);
        const char *result = match.scoreBlue == match.scoreOrange ? "DRAW"
                           : (match.scoreBlue > match.scoreOrange ? "BLUE WINS" : "ORANGE WINS");
        Color color = match.scoreBlue == match.scoreOrange ? uistyle::Text
                    : (match.scoreBlue > match.scoreOrange ? uistyle::TeamBlue : uistyle::TeamOrange);
        uistyle::DrawTextCentered(result, centerX, 260.0f * scale, 32.0f, color);
    }
}

// The 0-100 meter. Milestone 10 replaces this with the real HUD widget.
void MatchScene::DrawBoostMeter()
{
    const float scale = uistyle::Scale();
    const float barWidth = 210.0f * scale;
    const float barHeight = 20.0f * scale;
    Rectangle bar = { GetScreenWidth() - barWidth - 28.0f * scale,
                      GetScreenHeight() - barHeight - 46.0f * scale, barWidth, barHeight };

    float fraction = playerCar.boostCapacity > 0.0f ? playerCar.boostAmount / playerCar.boostCapacity : 0.0f;
    DrawRectangleRec(bar, uistyle::Panel);
    DrawRectangleRec(Rectangle{ bar.x, bar.y, bar.width * fraction, bar.height },
                     playerCar.boosting ? Color{ 255, 232, 150, 255 } : uistyle::Boost);
    DrawRectangleLinesEx(bar, 2.0f, uistyle::Border);

    uistyle::DrawTextAt(TextFormat("BOOST %3.0f", playerCar.boostAmount),
                        bar.x, bar.y - 24.0f * scale, 18.0f, uistyle::TextDim);
}

void MatchScene::Draw()
{
    BeginMode3D(camera);
    for (GameObject *object : objects)
        object->Draw();
    // Last, and only after everything else: see the note on DrawGlassWalls.
    arena.DrawGlassWalls();
    EndMode3D();

    DrawMatchStatus();
    DrawBoostMeter();

    uistyle::DrawTextAt(TextFormat("SPEED %3.0f km/h", fabsf(playerCar.GetForwardSpeed()) * 3.6f),
                        20.0f, 20.0f, 24.0f, uistyle::Text);
    // Nothing has been simulated yet during the kickoff freeze, so the grounded
    // flag would read stale for the first three seconds of every match.
    if (!match.IsFrozen())
        uistyle::DrawTextAt(playerCar.grounded ? "GROUNDED" : "AIRBORNE", 20.0f, 52.0f, 18.0f, uistyle::TextDim);
    uistyle::DrawTextAt("WASD drive / air pitch+yaw - Space jump - Shift boost - Q/E air roll - R reset - Esc pause",
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
