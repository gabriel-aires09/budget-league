#include "MatchScene.h"

#include "AudioSystem.h"
#include "PostProcess.h"

#include <raymath.h>

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
        goal->depth = arena.goalDepth;
        goal->Initialize(*this);
    }

    // Spawned at exactly the height the box rests at, not a hair above it: the
    // old 0.36 was a 1 cm drop that every kickoff started with.
    playerCar.spawnPosition = Vector3{ 0.0f, playerCar.halfExtents.y, arena.length * 0.3f };
    playerCar.modelName = settings->playerCarModel; // picked in CarSelectScene
    playerCar.controller = &playerController;
    playerCar.ball = &ball; // so a flip into it lands as the big hit
    playerCar.Initialize(*this);

    // With the bot switched off the scene is exactly what it was: solo practice
    // with two working goals and a scoreboard.
    botActive = settings->botEnabled;
    if (botActive)
    {
        // SportsCar2 is the one cooked car the picker never offers, so the bot can
        // never turn up driving the same model as the player.
        botCar.spawnPosition = Vector3{ 0.0f, botCar.halfExtents.y, -arena.length * 0.3f };
        botCar.spawnYawDegrees = 180.0f; // facing the middle, from the other end
        botCar.modelName = "SportsCar2";
        botCar.teamColor = uistyle::TeamOrange;
        botCar.controller = &botController;
        botCar.ball = &ball;
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

    effects::Load();
    physicsSystem.OptimizeBroadPhase();
    // Keep the camera under the ceiling: above it the ray that keeps the camera
    // out of the walls would start by hitting the ceiling slab itself.
    chaseCamera.maxHeight = arena.wallHeight - 0.8f;
    chaseCamera.Initialize(camera, playerCar);

    // Seeded, or the first frame reads a full tank as a pad pickup.
    previousBoostAmount = playerCar.boostAmount;

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
    const float halfWidth = arena.FlatHalfWidth();
    const float halfLength = arena.FlatHalfLength();
    const float smallRows[] = { -halfLength * 0.8f, -halfLength * 0.4f,
                                 halfLength * 0.4f, halfLength * 0.8f };
    const float smallColumns[] = { -halfWidth * 0.71f, 0.0f, halfWidth * 0.71f };

    // Sized up front: see the note on boostPads in the header.
    boostPads.reserve(4 + 3 * 4 + 2);

    for (float sideX = -1.0f; sideX <= 1.0f; sideX += 2.0f)
    {
        for (float sideZ = -1.0f; sideZ <= 1.0f; sideZ += 2.0f)
        {
            BoostPadObject pad;
            pad.position = Vector3{ sideX * halfWidth * 0.84f, 0.0f,
                                    sideZ * halfLength * 0.89f };
            pad.radius = 2.2f;
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
        pad.position = Vector3{ sideX * halfWidth * 0.89f, 0.0f, 0.0f };
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
    {
        audio::SetBoost(false); // the one cue that is held, so it has to be released
        return;
    }
#endif

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
    {
        if (settingsOpen)
            settingsOpen = false;
        else
            paused = !paused;
    }

    if (paused)
    {
        audio::SetBoost(false);
        return;
    }

    match.Update(deltaTime);

    // The kickoff countdown freezes the field: everything was just re-centred
    // with zero velocity, so simply not stepping holds it there.
    if (!match.IsFrozen())
        StepPhysics(deltaTime);

    UpdateEffects(deltaTime);

    // Read every frame, so changing it in the pause menu is felt immediately.
    chaseCamera.sensitivity = settings->cameraSensitivity;
    chaseCamera.Update(camera, playerCar, ball.GetBodyPosition(), deltaTime);
}

// Every effect fires from a change this function watches for, so the effects
// themselves stay a dumb particle pool with no idea what a match is.
void MatchScene::UpdateEffects(float deltaTime)
{
    effects::Update(deltaTime);

    Vector3 ballPosition = ball.GetBodyPosition();
    float ballSpeed = ball.GetSpeed();

    if (match.state != previousState)
    {
        if (match.state == MatchState::Celebration)
        {
            // In the scoring team's colour, thrown back out of the net.
            Color team = match.lastScoringTeam == 0 ? uistyle::TeamBlue : uistyle::TeamOrange;
            Vector3 outward = { 0.0f, 0.5f, match.lastScoringTeam == 0 ? -1.0f : 1.0f };
            effects::Burst(ballPosition, outward, 90, 26.0f, 0.85f, team, 0.55f, 1.6f);
            effects::Burst(ballPosition, Vector3{ 0.0f, 1.0f, 0.0f }, 40, 18.0f, 1.0f,
                           Color{ 255, 255, 255, 255 }, 0.35f, 1.1f);
            chaseCamera.Shake(1.0f); // the one full-strength punch in the game
            audio::Play(AudioCue::Goal);
        }
        else if (match.state == MatchState::Kickoff)
        {
            effects::Clear(); // the field was just re-centred; nothing should linger
            chaseCamera.shakeStrength = 0.0f; // and neither should the goal punch
        }
        else if (match.state == MatchState::Playing)
        {
            audio::Play(AudioCue::CountdownGo); // the countdown just released the field
        }
        else if (match.state == MatchState::Finished)
        {
            audio::Play(AudioCue::MatchEnd);
        }
        previousState = match.state;
    }

    // One tick per whole second left of the countdown, from the same timer the
    // HUD draws its digit from.
    if (match.state == MatchState::Kickoff)
    {
        int digit = (int)ceilf(match.stateTimer);
        if (digit != previousCountdownDigit)
        {
            audio::Play(AudioCue::CountdownTick);
            previousCountdownDigit = digit;
        }
    }
    else
    {
        previousCountdownDigit = -1; // re-armed for the countdown after a goal
    }

    // A hit is a jump in the ball's speed. Reading it here rather than from a
    // contact listener keeps the physics free of callbacks, and the size of the
    // jump is exactly how hard the hit was.
    float gained = ballSpeed - previousBallSpeed;
    if (gained > 7.0f && match.state == MatchState::Playing)
    {
        float punch = fminf(gained / 25.0f, 1.0f);
        effects::Burst(ballPosition, Vector3{ 0.0f, 1.0f, 0.0f }, 8 + (int)(18.0f * punch),
                       6.0f + 10.0f * punch, 1.0f, Color{ 255, 236, 190, 255 },
                       0.18f + 0.16f * punch, 0.5f);
        // The same punch the burst and the thump are scaled by, so all three read
        // as one hit. Deliberately well short of the goal's: a scramble in front
        // of the net is a lot of these in a row.
        chaseCamera.Shake(0.18f + 0.42f * punch);
        // The harder the hit the louder and the deeper the thump.
        audio::Play(AudioCue::BallHit, 0.55f + 0.45f * punch, 1.15f - 0.3f * punch);
    }
    previousBallSpeed = ballSpeed;

    // A wall or the other car, read the same way the ball hit is: a jump in the
    // player's speed, here downwards. Braking never gets near this in one frame
    // (about 0.65 m/s at 60 Hz), so only a real collision reaches it.
    float carSpeed = Vector3Length(playerCar.GetBodyVelocity());
    float lost = previousCarSpeed - carSpeed;
    if (lost > 5.0f && match.state != MatchState::Kickoff)
        audio::Play(AudioCue::Impact, fminf(0.35f + lost / 25.0f, 1.0f),
                    0.9f + GetRandomValue(0, 200) / 1000.0f); // varied, so repeats do not read as one sample
    previousCarSpeed = carSpeed;

    // Boost only ever goes up by a pad, so the pickup needs no event of its own.
    if (playerCar.boostAmount > previousBoostAmount + 0.01f)
        audio::Play(AudioCue::BoostPad);
    previousBoostAmount = playerCar.boostAmount;

    audio::SetBoost(playerCar.boosting);

    for (CarObject *car : { &playerCar, botActive ? &botCar : &playerCar })
    {
        if (car->jumpPending)
        {
            // Down at the wheels, thrown outwards, as if off the floor.
            Vector3 under = car->GetBodyPosition();
            under.y -= car->halfExtents.y;
            effects::Burst(under, Vector3{ 0.0f, -0.2f, 0.0f }, 10, 4.5f, 1.0f,
                           Color{ 190, 210, 240, 255 }, 0.16f, 0.35f);
            // Pitched down for the second jump, which is what tells a flip from
            // a jump without a second flag on the car.
            if (car == &playerCar)
                audio::Play(AudioCue::Jump, 1.0f, car->doubleJumpUsed ? 0.8f : 1.0f);
            car->jumpPending = false;
        }

        // The trail: embers a frame while the boost is held, which is what makes a
        // boosting car legible from across the arena. Both how many and how big
        // ramp in with the hold, so a tap leaves a puff and a held boost a stream.
        if (car->boosting)
        {
            const float intensity = car->BoostIntensity();
            Matrix rotation = car->GetBodyRotation();
            Vector3 exhaust = Vector3Add(car->GetBodyPosition(),
                                         Vector3Transform(Vector3{ 0.0f, 0.05f, car->halfExtents.z }, rotation));
            effects::Burst(exhaust, Vector3Transform(Vector3{ 0.0f, 0.1f, 1.0f }, rotation),
                           1 + (int)(3.0f * intensity), 3.0f + 2.5f * intensity, 0.5f,
                           Color{ 255, 190, 90, 255 }, 0.16f + 0.14f * intensity,
                           0.35f + 0.25f * intensity);
        }
    }
}

void MatchScene::Draw()
{
    // Bloom wraps the 3D pass only. The HUD is drawn afterwards at full
    // resolution, so the text never goes through a blur.
    bool captured = postprocess::Begin(settings->postProcessing);
    if (captured)
        ClearBackground(uistyle::Background);

    BeginMode3D(camera);
    for (GameObject *object : objects)
        object->Draw();
    DrawEffects();
    // Last, and only after everything else: see the note on DrawGlassWalls.
    arena.DrawGlassWalls();
    EndMode3D();

    postprocess::End();

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
        // Unchanged: the pause overlay has already dimmed the match behind it.
        settingsOpen = settingsMenu.Draw(*settings, area, SettingsBackground::Dimmed);
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

// The parts of the effects that hang off an object rather than off an event.
// After the objects and before the glass, so they blend over the field but the
// see-through walls still come last.
void MatchScene::DrawEffects()
{
    Vector3 ballPosition = ball.GetBodyPosition();

    effects::DrawContactShadow(*this, ballPosition, ball.radius * 0.95f, ball.bodyID);

    for (CarObject *car : { &playerCar, botActive ? &botCar : &playerCar })
    {
        Vector3 position = car->GetBodyPosition();
        effects::DrawContactShadow(*this, position, car->halfExtents.z * 0.8f, car->bodyID);
        if (car->boosting)
        {
            // Flickering, because a constant flame reads as a solid object stuck
            // to the back of the car, and scaled by how long the boost has been
            // held. A tap gets about a third of the cone, a sustained burn all of
            // it — before this the same full-size flame was drawn either way.
            float flicker = 0.85f + 0.15f * sinf((float)GetTime() * 42.0f);
            float intensity = 0.35f + 0.65f * car->BoostIntensity();
            effects::DrawBoostFlame(position, car->GetBodyRotation(), car->halfExtents.z,
                                    flicker * intensity);
        }
    }

    // Last of the three-dimensional effects, so the bursts blend over the
    // shadows and the flames rather than under them.
    effects::Draw();
}

void MatchScene::Shutdown()
{
    audio::SetBoost(false); // nothing else releases it when the scene goes away
    effects::Unload();

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
