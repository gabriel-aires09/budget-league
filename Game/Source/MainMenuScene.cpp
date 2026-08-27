#include "MainMenuScene.h"
#include "GamepadInput.h"

#include "Effects.h"
#include "PostProcess.h"

#include <raymath.h>

#if defined(GAME_DEBUG)
    #define BUILD_NAME "Debug"
#elif defined(GAME_DEVELOPMENT)
    #define BUILD_NAME "Development"
#else
    #define BUILD_NAME "Release"
#endif

// All seven cooked cars are allowed here. The six-car restriction belongs to the
// picker (CLAUDE.md Milestone 08); a showcase has no reason to hide one.
static const char *SHOWCASE_CARS[] = {
    "SportsCar", "SportsCar2", "Cop", "Taxi", "NormalCar1", "NormalCar2", "SUV",
};
static const int SHOWCASE_CAR_COUNT = 7;

// The car stands at the middle of the pitch and the camera sits in front of it,
// low and to one side, so it fills the right of the frame and leaves the left
// for the menu - the layout of the reference shot (img/menu-rocket-league.png).
static const Vector3 CAR_SPOT = { 0.0f, 0.0f, 0.0f };
static const Vector3 CAMERA_POSITION = { -2.6f, 1.9f, 6.6f };
static const Vector3 LOOK_AT = { -1.6f, 1.0f, 0.0f };
static const float SHOWCASE_LENGTH = 4.2f; // every car is fitted to this, whatever the pack modelled
static const float SPIN_RATE = 12.0f;      // degrees per second, slow enough to read as idle

void MainMenuScene::Initialize()
{
    InitializePhysics();

    arena.Initialize(*this);

    // Both nets, from the arena's own opening, exactly as the match builds them.
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

    objects.push_back(&arena);
    objects.push_back(&blueGoal);
    objects.push_back(&orangeGoal);

    effects::Load(); // the car's contact shadow, which is what sits it on the pitch
    physicsSystem.OptimizeBroadPhase();

    // A different car, in a different team colour, every time the menu opens.
    showcaseColor = GetRandomValue(0, 1) == 0 ? uistyle::TeamBlue : uistyle::TeamOrange;
    if (showcaseCar.Load(SHOWCASE_CARS[GetRandomValue(0, SHOWCASE_CAR_COUNT - 1)]))
    {
        float length = showcaseCar.bounds.max.z - showcaseCar.bounds.min.z;
        showcaseScale = length > 0.0f ? SHOWCASE_LENGTH / length : 1.0f;
        showcaseCar.SetPaintColor(showcaseColor);
    }

    camera.position = CAMERA_POSITION;
    camera.target = LOOK_AT;
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 42.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void MainMenuScene::Update(float deltaTime)
{
    // The turntable is the only thing that moves: physics is never stepped.
    spinDegrees = fmodf(spinDegrees + SPIN_RATE * deltaTime, 360.0f);

    if (settingsOpen && (IsKeyPressed(KEY_ESCAPE) || gamepad::MenuCancel()))
        settingsOpen = false;
}

void MainMenuScene::Draw()
{
    const float scale = uistyle::Scale();

    // The showcase first, then the menu on top of it.
    bool captured = postprocess::Begin(settings->postProcessing);
    if (captured)
        ClearBackground(uistyle::Background);

    BeginMode3D(camera);
    for (GameObject *object : objects)
        object->Draw();

    if (showcaseCar.loaded)
    {
        showcaseCar.model.transform = MatrixMultiply(MatrixScale(showcaseScale, showcaseScale, showcaseScale),
                                                     MatrixRotateY(spinDegrees * DEG2RAD));
        // Lifted so the wheels rest on the pitch, as CarObject and the picker do.
        Vector3 stand = { CAR_SPOT.x, CAR_SPOT.y - showcaseCar.bounds.min.y * showcaseScale, CAR_SPOT.z };
        DrawModel(showcaseCar.model, stand, 1.0f, WHITE);
        effects::DrawContactShadow(*this, Vector3{ stand.x, stand.y + 0.4f, stand.z },
                                   SHOWCASE_LENGTH * 0.4f, JPH::BodyID());
    }
    else
    {
        DrawCube(Vector3{ CAR_SPOT.x, 0.35f, CAR_SPOT.z }, 1.7f, 0.7f, 3.2f, showcaseColor);
    }

    // Last of all, as everywhere else: the glass never writes depth.
    arena.DrawGlassWalls();
    EndMode3D();

    postprocess::End();

    // The menu is a column down the left, clear of the car. Its rows draw their
    // own panels, so they stay readable over the arena without a backdrop.
    const float rowWidth = 320.0f * scale;
    const float rowHeight = 46.0f * scale;
    const float rowGap = 10.0f * scale;
    const float columnX = 64.0f * scale;
    // const float columnCenterX = columnX + rowWidth * 0.5f;
   
    if (settingsOpen)
    {
        // Centred on both axes and with no game title above it: while it is open
        // the panel is the only thing on the screen, over the live showcase
        // rather than over a flat backdrop (CLAUDE.md Milestone 19).
        Rectangle area = { GetScreenWidth() * 0.5f - SettingsMenu::PreferredWidth() * 0.5f,
                           GetScreenHeight() * 0.5f - SettingsMenu::PreferredHeight() * 0.5f,
                           SettingsMenu::PreferredWidth(), SettingsMenu::PreferredHeight() };
        settingsOpen = settingsMenu.Draw(*settings, area, SettingsBackground::Showcase);
    }
    else
    {
        // uistyle::DrawShadowedText("BUDGET LEAGUE", columnCenterX, 92.0f * scale, 40.0f, uistyle::Text);
        //
        Rectangle row = { columnX, 300.0f * scale, rowWidth, rowHeight };

        menu.itemCount = 4;
        menu.Update();

        // Play goes to the car picker, which is what starts the match.
        if (menu.Item(row, "Play", 0))
            pendingAction = MenuAction::SelectCar;
        row.y += rowHeight + rowGap;

        if (menu.Item(row, "How to play", 1))
            pendingAction = MenuAction::HowToPlay;
        row.y += rowHeight + rowGap;

        if (menu.Item(row, "Settings", 2))
            settingsOpen = true;
        row.y += rowHeight + rowGap;

        if (menu.Item(row, "Exit", 3))
            pendingAction = MenuAction::ExitGame;
    }

    // uistyle::DrawTextAt("Made with raylib, Jolt Physics and Dear ImGui",
    //                     columnX, GetScreenHeight() - 58.0f * scale, 16.0f, uistyle::TextDim);
    // uistyle::DrawTextAt(TextFormat("Arcade Car Soccer - %s build - %s", BUILD_NAME, __DATE__),
    //                     columnX, GetScreenHeight() - 34.0f * scale, 16.0f, uistyle::TextDim);
}

void MainMenuScene::Shutdown()
{
    showcaseCar.Unload();
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
