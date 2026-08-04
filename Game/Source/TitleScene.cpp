#include "TitleScene.h"

#include "AudioSystem.h"
#include "Effects.h"
#include "PostProcess.h"
#include "UserInterface.h"

#include <raymath.h>

#include <algorithm>
#include <cmath>

#if defined(GAME_DEBUG)
    #define BUILD_NAME "Debug"
#elif defined(GAME_DEVELOPMENT)
    #define BUILD_NAME "Development"
#else
    #define BUILD_NAME "Release"
#endif

// Low and off to one side, looking down the field at the far goal, with the ball
// resting in the near corner of the frame - the composition of the reference
// shot (img/press-key-menu.png). The ball is deliberately not centred: the logo
// and the prompt own the middle of the screen.
static const Vector3 CAMERA_POSITION = { 10.0f, 3.6f, -28.0f };
static const Vector3 LOOK_AT = { 10.2f, 1.6f, -67.0f };
static const Vector3 BALL_SPOT = { 16.0f, 0.0f, -40.0f }; // y is filled in from the radius
// The drift yaws the camera in place. It is a slow sweep either side, felt
// rather than watched, and it never moves the camera into anything.
static const float DRIFT_DEGREES = 2.5f;
static const float DRIFT_PERIOD = 24.0f; // seconds for a full sweep and back
static const float FADE_IN = 0.6f;
// Base size of the two name lines; everything else in the mark is measured from it.
static const float NAME_SIZE = 68.0f;

void TitleScene::Initialize()
{
    InitializePhysics();

    // The match's own objects, so the title cannot drift from what the game
    // looks like. Neither is ever stepped: see the note in the header.
    arena.Initialize(*this);

    // Both nets, built from the arena's own opening exactly as MatchScene does.
    // Without them the back wall is a black hole in the middle of the shot, and
    // the far goal is the thing the camera is pointed at.
    blueGoal.lineZ = arena.length * 0.5f;
    blueGoal.direction = 1.0f;
    blueGoal.teamColor = uistyle::TeamBlue;
    orangeGoal.lineZ = -arena.length * 0.5f;
    orangeGoal.direction = -1.0f;
    orangeGoal.teamColor = uistyle::TeamOrange;
    for (GoalObject *goal : { &blueGoal, &orangeGoal })
    {
        goal->width = arena.goalWidth;
        goal->height = arena.goalHeight;
        goal->depth = arena.goalDepth;
        goal->Initialize(*this);
    }

    ball.spawnPosition = Vector3{ BALL_SPOT.x, ball.radius, BALL_SPOT.z };
    ball.Initialize(*this);

    objects.push_back(&arena);
    objects.push_back(&blueGoal);
    objects.push_back(&orangeGoal);
    objects.push_back(&ball);

    effects::Load(); // for the ball's contact shadow, which is what sits it on the pitch
    physicsSystem.OptimizeBroadPhase();

    logo.Load("budget-league-logo");

    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 55.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void TitleScene::Update(float deltaTime)
{
    elapsed += deltaTime;

    // Any key, any mouse button, any gamepad button. GetKeyPressed pops the
    // queue, so it is read once and only here.
    bool pressed = GetKeyPressed() != 0 || GetGamepadButtonPressed() != GAMEPAD_BUTTON_UNKNOWN;
    for (int button = MOUSE_BUTTON_LEFT; button <= MOUSE_BUTTON_MIDDLE && !pressed; ++button)
        pressed = IsMouseButtonPressed(button);

    if (pressed && elapsed > FADE_IN)
    {
        audio::Play(AudioCue::UiClick);
        pendingAction = MenuAction::MainMenu;
    }

    // The camera stands still and only yaws, so the ball and the goal both keep
    // their place in the frame while the arena slides behind them.
    float angle = DRIFT_DEGREES * DEG2RAD * sinf(elapsed * 2.0f * PI / DRIFT_PERIOD);
    Vector3 look = Vector3Subtract(LOOK_AT, CAMERA_POSITION);
    camera.position = CAMERA_POSITION;
    camera.target = Vector3Add(CAMERA_POSITION, Vector3RotateByAxisAngle(look, Vector3{ 0.0f, 1.0f, 0.0f }, angle));
}

void TitleScene::Draw()
{
    bool captured = postprocess::Begin(settings->postProcessing);
    if (captured)
        ClearBackground(uistyle::Background);

    BeginMode3D(camera);
    for (GameObject *object : objects)
        object->Draw();
    effects::DrawContactShadow(*this, ball.GetBodyPosition(), ball.radius * 0.95f, ball.bodyID);
    // Last, and only after everything else, exactly as the match does it.
    arena.DrawGlassWalls();
    EndMode3D();

    postprocess::End();

    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;
    const float screenHeight = (float)GetScreenHeight();

    // The mark is the badge and the name side by side, as on the reference shot
    // (img/press-key-menu.png): the shield on the left, BUDGET over LEAGUE to
    // its right, the pair centred as one block.
    //
    // Everything here is measured from the name, so the badge follows the text
    // rather than the other way round and the whole mark scales with uistyle.
    const int lineSize = uistyle::FontSize(NAME_SIZE);
    const float lineGap = lineSize * 0.06f;
    const float nameHeight = lineSize * 2.0f + lineGap;
    const float nameWidth = (float)std::max(MeasureText("BUDGET", lineSize), MeasureText("LEAGUE", lineSize));
    // 1.85 rather than the reference's 1.5, because the shield only fills 82% of
    // its own image - the rest is the glow margin, which is also what spaces the
    // badge off the name, so there is no gap of its own.
    const float badgeHeight = nameHeight * 1.85f;
    const float badgeWidth = logo.loaded ? badgeHeight * logo.texture.width / logo.texture.height : 0.0f;
    const float badgeGap = 0.0f;

    float left = centerX - (badgeWidth + badgeGap + nameWidth) * 0.5f;
    float top = screenHeight * 0.06f;

    if (logo.loaded)
    {
        Rectangle source = { 0.0f, 0.0f, (float)logo.texture.width, (float)logo.texture.height };
        Rectangle destination = { left, top, badgeWidth, badgeHeight };
        DrawTexturePro(logo.texture, source, destination, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);
    }

    // Left aligned against each other, and shadowed: the name lands over the
    // arena, where the background can be any brightness.
    float nameLeft = left + badgeWidth + badgeGap;
    float nameTop = top + (badgeHeight - nameHeight) * 0.5f;
    uistyle::DrawShadowedText("BUDGET", nameLeft + MeasureText("BUDGET", lineSize) * 0.5f,
                              nameTop, NAME_SIZE, uistyle::Text);
    uistyle::DrawShadowedText("LEAGUE", nameLeft + MeasureText("LEAGUE", lineSize) * 0.5f,
                              nameTop + lineSize + lineGap, NAME_SIZE, uistyle::Text);

    // Pulsing, so it is the one thing on the screen that asks to be looked at.
    float pulse = 0.65f + 0.35f * sinf(elapsed * 3.0f);
    uistyle::DrawShadowedText("PRESS ANY BUTTON TO START", centerX, screenHeight * 0.72f, 26.0f,
                              Fade(uistyle::Accent, pulse));

    // Over the pitch rather than over a panel, so both lines are shadowed too.
    uistyle::DrawShadowedText("MiraSoft", centerX,
                              screenHeight - 58.0f * scale, 16.0f, uistyle::TextDim);
    uistyle::DrawShadowedText(TextFormat("Budget League - 2026 MiraSoft. All Rights Reserved"),
                              centerX, screenHeight - 34.0f * scale, 16.0f, uistyle::TextDim);

    if (elapsed < FADE_IN)
        uistyle::DrawDimmer(1.0f - elapsed / FADE_IN);
}

void TitleScene::Shutdown()
{
    logo.Unload();
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
