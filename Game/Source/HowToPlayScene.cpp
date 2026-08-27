#include "HowToPlayScene.h"

// A block of the screen: a heading and up to six lines, ended by a null. Keep
// the lines under about 45 characters — that is what fits a column at 720p, and
// the layout scales from there.
struct Section
{
    const char *title;
    const char *lines[7];
};

static const Section LEFT_COLUMN[] = {
    { "THE GAME", {
        "Drive the ball into the other team's goal",
        "A goal counts only when the ball is fully in",
        "Highest score at full time wins",
        nullptr } },
    { "DRIVING", {
        "W / S or Up / Down - accelerate and reverse",
        "A / D or Left / Right - steer",
        "Shift (hold) - boost",
        "R - reset your car",
        nullptr } },
    { "IN THE AIR", {
        "Space - jump",
        "Space again - double jump, or flip if you",
        "  are holding a direction",
        "W / S pitch, A / D yaw, Q / E roll",
        nullptr } },
};

static const Section RIGHT_COLUMN[] = {
    { "BOOST", {
        "The meter holds 100, and a full tank lasts",
        "  about three seconds",
        "Small pads give 12 and reset after 4 s",
        "The four big pads out wide fill you up",
        nullptr } },
    { "WALLS AND CEILING", {
        "Every edge of the arena is a ramp",
        "Carry speed into one to drive up the wall",
        "With boost you can cross the ceiling",
        nullptr } },
    { "CAMERA AND MATCH", {
        "C - chase cam or ball cam",
        "Esc or P - pause",
        "Kickoff counts down 3 s, then the ball is live",
        nullptr } },
};

// Draws one section at the top of the column and returns the height it used.
static float DrawSection(const Section &section, Rectangle column)
{
    const float scale = uistyle::Scale();

    int lineCount = 0;
    while (section.lines[lineCount] != nullptr)
        ++lineCount;

    Rectangle panel = { column.x, column.y, column.width, (46.0f + lineCount * 23.0f + 12.0f) * scale };
    uistyle::DrawPanel(panel);
    uistyle::DrawTextAt(section.title, panel.x + 18.0f * scale, panel.y + 14.0f * scale, 20.0f, uistyle::Accent);
    for (int i = 0; i < lineCount; ++i)
    {
        uistyle::DrawTextAt(section.lines[i], panel.x + 18.0f * scale,
                            panel.y + (46.0f + i * 23.0f) * scale, 17.0f, uistyle::Text);
    }

    return panel.height;
}

void HowToPlayScene::Initialize()
{
}

void HowToPlayScene::Update(float deltaTime)
{
    (void)deltaTime;

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ENTER) ||
        IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE))
        pendingAction = MenuAction::MainMenu;
}

void HowToPlayScene::Draw()
{
    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;

    ClearBackground(uistyle::Background);

    uistyle::DrawTitle("HOW TO PLAY", centerX, 34.0f * scale, 40.0f);

    const float columnWidth = 540.0f * scale;
    const float columnGap = 36.0f * scale;
    const float sectionGap = 14.0f * scale;

    Rectangle column = { centerX - columnGap * 0.5f - columnWidth, 116.0f * scale, columnWidth, 0.0f };
    for (const Section &section : LEFT_COLUMN)
        column.y += DrawSection(section, column) + sectionGap;

    column = Rectangle{ centerX + columnGap * 0.5f, 116.0f * scale, columnWidth, 0.0f };
    for (const Section &section : RIGHT_COLUMN)
        column.y += DrawSection(section, column) + sectionGap;

    Rectangle back = { centerX - 115.0f * scale, GetScreenHeight() - 88.0f * scale,
                       230.0f * scale, 46.0f * scale };
    if (uistyle::Button(back, "BACK", true))
        pendingAction = MenuAction::MainMenu;

    uistyle::DrawTextCentered("Enter, Space or Esc goes back to the menu",
                              centerX, GetScreenHeight() - 32.0f * scale, 17.0f, uistyle::TextDim);
}

void HowToPlayScene::Shutdown()
{
}
