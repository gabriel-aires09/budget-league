#include "MatchScene.h"

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
    playerCar.modelName = settings->playerCarModel; // picked in CarSelectScene
    playerCar.controller = &playerController;
    playerCar.Initialize(*this);

    // With the bot switched off the scene is exactly what it was: solo practice
    // with two working goals and a scoreboard.
    botActive = settings->botEnabled;
    if (botActive)
    {
        // SportsCar2 is the one cooked car the picker never offers, so the bot can
        // never turn up driving the same model as the player.
        botCar.spawnPosition = Vector3{ 0.0f, 0.36f, -arena.length * 0.3f };
        botCar.spawnYawDegrees = 180.0f; // facing the middle, from the other end
        botCar.modelName = "SportsCar2";
        botCar.teamColor = uistyle::TeamOrange;
        botCar.controller = &botController;
        botCar.Initialize(*this);

        botController.car = &botCar;
        botController.ball = &ball;
        botController.targetGoalZ = blueGoal.lineZ; // it attacks the goal the player defends
        botController.fieldHalfWidth = arena.FlatHalfWidth();
        botController.fieldHalfLength = arena.FlatHalfLength();
    }

    BuildBoostPads();

    objects.push_back(&arena);
    objects.push_back(&blueGoal);
    objects.push_back(&orangeGoal);
    objects.push_back(&ball);
    objects.push_back(&playerCar);
    if (botActive)
        objects.push_back(&botCar);
    for (BoostPadObject &pad : boostPads)
    {
        pad.cars.push_back(&playerCar);
        if (botActive)
            pad.cars.push_back(&botCar);
        pad.Initialize(*this);
        objects.push_back(&pad);
    }

    physicsSystem.OptimizeBroadPhase();
    // Keep the camera under the ceiling: above it the ray that keeps the camera
    // out of the walls would start by hitting the ceiling slab itself.
    chaseCamera.maxHeight = arena.wallHeight - 0.8f;
    chaseCamera.Initialize(camera, playerCar);

    match.AddCar(playerCar);
    if (botActive)
        match.AddCar(botCar);
    match.Begin(ball, blueGoal, orangeGoal, (float)settings->matchDurationMinutes);

#ifdef GAME_DEV_TOOLS
    // Last, so the saved config lands on objects that already exist and the
    // panel's sliders point at the real fields.
    tuningPanel.Initialize(*this);
#endif
}

// Four full pads out wide plus a scatter of small ones, so crossing the field
// the long way is always a choice between the quick route and the fed route.
//
// Every pad has to sit inside the flat part of the floor: a pad is a flat disc
// with no body, so one hanging over a ramp would clip into it and read as broken.
// arena.FlatHalfWidth/FlatHalfLength are the limits, minus the pad's own radius.
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
            pad.position = Vector3{ sideX * 19.0f, 0.0f, sideZ * 31.0f };
            pad.radius = 3.0f;
            pad.refillAmount = 100.0f;
            pad.cooldownTime = 10.0f;
            pad.fullRefill = true;
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
        pad.position = Vector3{ sideX * 20.0f, 0.0f, 0.0f };
        boostPads.push_back(pad);
    }
}

void MatchScene::Update(float deltaTime)
{
#ifdef GAME_DEV_TOOLS
    if (IsKeyPressed(KEY_F1))
        tuningPanel.open = !tuningPanel.open;
    // Frozen while the panel is up, so nothing below runs - not the pause key
    // either, which ImGui may be using for a text field.
    if (tuningPanel.open)
        return;
#endif

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

    // Read every frame, so changing it in the pause menu is felt immediately.
    chaseCamera.sensitivity = settings->cameraSensitivity;
    chaseCamera.Update(camera, playerCar, ball.GetBodyPosition(), deltaTime);
}

void MatchScene::Draw()
{
    BeginMode3D(camera);
    for (GameObject *object : objects)
        object->Draw();
    // Last, and only after everything else: see the note on DrawGlassWalls.
    arena.DrawGlassWalls();
    EndMode3D();

    hud::Draw(match, playerCar);

#ifdef GAME_DEV_TOOLS
    tuningPanel.Draw(*this);

    // Development readouts, not shipping UI. Nothing has been simulated yet
    // during the kickoff freeze, so the grounded flag would read stale for the
    // first three seconds of every match.
    if (!match.IsFrozen())
        uistyle::DrawTextAt(playerCar.grounded ? "GROUNDED" : "AIRBORNE", 20.0f, 20.0f, 18.0f, uistyle::TextDim);
    uistyle::DrawTextAt(chaseCamera.ballCam ? "BALL CAM" : "CHASE CAM", 20.0f, 44.0f, 18.0f, uistyle::TextDim);
#endif

    // The end-of-match screen takes input, so it is skipped while the pause menu
    // is up rather than left to draw hover states under the dimmer.
    if (!paused && match.state == MatchState::Finished)
    {
        MenuAction action = hud::DrawFullTime(match);
        if (action != MenuAction::None)
            pendingAction = action;
    }

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
