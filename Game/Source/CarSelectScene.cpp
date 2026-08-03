#include "CarSelectScene.h"

#include <raymath.h>

#include <cmath>

// Six of the seven cooked cars, as Milestone 08 asks; SportsCar2 is the one it
// says to leave out. The order is the reading order of the grid.
static const struct
{
    const char *model;
    const char *label;
} CARS[CarSelectScene::CAR_COUNT] = {
    { "SportsCar",  "SPORTS CAR" },
    { "Cop",        "POLICE" },
    { "Taxi",       "TAXI" },
    { "NormalCar1", "COUPE" },
    { "NormalCar2", "COMPACT" },
    { "SUV",        "SUV" },
};

// Grid layout, in world metres. The cars sit in the plane z = 0 facing a static
// camera, so every cell is the same distance away and the previews all come out
// the same size — a showroom laid out on the floor would shrink the back row.
static const float COLUMN_X[CarSelectScene::COLUMNS] = { -6.6f, 0.0f, 6.6f };
static const float ROW_Y[2] = { 5.9f, 1.3f };
static const float PEDESTAL_RADIUS = 2.2f;
static const float PEDESTAL_HEIGHT = 0.28f;
static const float PREVIEW_LENGTH = 3.8f; // every car is fitted to this length
static const float SELECTED_SCALE = 1.12f;
static const float SPIN_RATE = 26.0f; // degrees per second

// Where a car stands, at the top of its pedestal.
static Vector3 CarPosition(int index)
{
    return Vector3{ COLUMN_X[index % CarSelectScene::COLUMNS], ROW_Y[index / CarSelectScene::COLUMNS], 0.0f };
}

// The two buttons along the bottom. Not a uistyle::MenuList: the keyboard
// selection belongs to the grid, so these are mouse-only and Enter/Esc do the
// same two things from anywhere on the screen.
static bool Button(Rectangle bounds, const char *label, bool primary)
{
    const float scale = uistyle::Scale();
    bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);

    DrawRectangleRec(bounds, hovered ? uistyle::PanelHighlight : uistyle::Panel);
    DrawRectangleLinesEx(bounds, 2.0f * scale, (hovered || primary) ? uistyle::Accent : uistyle::Border);
    uistyle::DrawTextCentered(label, bounds.x + bounds.width * 0.5f,
                              bounds.y + (bounds.height - uistyle::FontSize(21.0f)) * 0.5f, 21.0f,
                              (hovered || primary) ? uistyle::Text : uistyle::TextDim);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void CarSelectScene::Initialize()
{
    // Above every car and tilted down, so both rows are seen from slightly above
    // as a car preview should be. A level camera puts the top row overhead and
    // shows its underside.
    camera.position = Vector3{ 0.0f, 9.2f, 16.0f };
    camera.target = Vector3{ 0.0f, 3.2f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 44.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    for (int i = 0; i < CAR_COUNT; ++i)
    {
        if (!previews[i].Load(CARS[i].model))
            continue;

        float length = previews[i].bounds.max.z - previews[i].bounds.min.z;
        previewScale[i] = length > 0.0f ? PREVIEW_LENGTH / length : 1.0f;
        previews[i].SetPaintColor(uistyle::TeamBlue);
    }

    // Come back to the car that is already picked.
    for (int i = 0; i < CAR_COUNT; ++i)
    {
        if (settings->playerCarModel == CARS[i].model)
            selected = i;
    }
}

void CarSelectScene::Update(float deltaTime)
{
    spinDegrees = fmodf(spinDegrees + SPIN_RATE * deltaTime, 360.0f);

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))
        pendingAction = MenuAction::MainMenu;
}

void CarSelectScene::Draw()
{
    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;

    ClearBackground(uistyle::Background);

    // Grid navigation. With only two rows, up and down are the same move.
    int column = selected % COLUMNS;
    int row = selected / COLUMNS;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
        column = (column + 1) % COLUMNS;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
        column = (column + COLUMNS - 1) % COLUMNS;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        row = 1 - row;
    selected = row * COLUMNS + column;

    BeginMode3D(camera);
    for (int i = 0; i < CAR_COUNT; ++i)
    {
        Vector3 base = CarPosition(i);
        bool isSelected = (i == selected);

        DrawCylinder(Vector3{ base.x, base.y - PEDESTAL_HEIGHT, base.z }, PEDESTAL_RADIUS, PEDESTAL_RADIUS,
                     PEDESTAL_HEIGHT, 8, isSelected ? uistyle::PanelHighlight : uistyle::Panel);
        DrawCylinderWires(Vector3{ base.x, base.y - PEDESTAL_HEIGHT, base.z }, PEDESTAL_RADIUS, PEDESTAL_RADIUS,
                          PEDESTAL_HEIGHT, 8, isSelected ? uistyle::Accent : uistyle::Border);

        // Same stand-in as CarObject uses, so an uncooked build still works.
        if (!previews[i].loaded)
        {
            DrawCube(Vector3{ base.x, base.y + 0.35f, base.z }, 1.7f, 0.7f, 3.2f, uistyle::TeamBlue);
            continue;
        }

        float previewSize = previewScale[i] * (isSelected ? SELECTED_SCALE : 1.0f);
        previews[i].model.transform = MatrixMultiply(MatrixScale(previewSize, previewSize, previewSize),
                                                     MatrixRotateY(spinDegrees * DEG2RAD));
        // Lift the model so its wheels rest on the pedestal, as CarObject does.
        Vector3 stand = { base.x, base.y - previews[i].bounds.min.y * previewSize, base.z };
        DrawModel(previews[i].model, stand, 1.0f, WHITE);
    }
    EndMode3D();

    uistyle::DrawTitle("SELECT YOUR CAR", centerX, 40.0f * scale, 40.0f);

    // Each cell is projected from the corners of its own pedestal rather than
    // laid out in screen space, so the frame and the label sit on their car at
    // any window size and in either row, which the camera sees from a different
    // angle.
    for (int i = 0; i < CAR_COUNT; ++i)
    {
        Vector3 base = CarPosition(i);
        Vector2 top = GetWorldToScreen(Vector3{ base.x, base.y + 1.9f, base.z }, camera);
        // The near edge of the pedestal is its lowest point on screen.
        Vector2 front = GetWorldToScreen(Vector3{ base.x, base.y - PEDESTAL_HEIGHT, base.z + PEDESTAL_RADIUS }, camera);
        Vector2 left = GetWorldToScreen(Vector3{ base.x - PEDESTAL_RADIUS, base.y, base.z }, camera);
        Vector2 right = GetWorldToScreen(Vector3{ base.x + PEDESTAL_RADIUS, base.y, base.z }, camera);
        Rectangle cell = { left.x, top.y, right.x - left.x, front.y - top.y };

        // The mouse only takes over the selection when it actually moves, which
        // is how the menu rows behave too.
        Vector2 mouseDelta = GetMouseDelta();
        bool hovered = CheckCollisionPointRec(GetMousePosition(), cell);
        if (hovered && (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f))
            selected = i;
        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            selected = i;

        if (i == selected)
            DrawRectangleLinesEx(cell, 2.0f * scale, uistyle::Accent);
        uistyle::DrawTextCentered(CARS[i].label, cell.x + cell.width * 0.5f, cell.y + cell.height + 8.0f * scale,
                                  20.0f, i == selected ? uistyle::Text : uistyle::TextDim);
    }

    const float buttonWidth = 230.0f * scale;
    const float buttonHeight = 46.0f * scale;
    const float buttonGap = 16.0f * scale;
    Rectangle startButton = { centerX - buttonWidth - buttonGap * 0.5f,
                              GetScreenHeight() - 96.0f * scale, buttonWidth, buttonHeight };
    Rectangle backButton = { centerX + buttonGap * 0.5f, startButton.y, buttonWidth, buttonHeight };

    bool start = Button(startButton, "START MATCH", true);
    bool back = Button(backButton, "BACK", false);

    if (start || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        settings->playerCarModel = CARS[selected].model;
        pendingAction = MenuAction::StartMatch;
    }
    else if (back)
    {
        pendingAction = MenuAction::MainMenu;
    }

    uistyle::DrawTextCentered("Arrows or WASD choose - Enter starts the match - Esc goes back",
                              centerX, GetScreenHeight() - 36.0f * scale, 17.0f, uistyle::TextDim);
}

void CarSelectScene::Shutdown()
{
    for (int i = 0; i < CAR_COUNT; ++i)
        previews[i].Unload();
}
