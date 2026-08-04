#include "SettingsMenu.h"

// Layout, in 720p units. PreferredHeight is derived from these, so adding a row
// cannot silently overflow the panel.
static const float TITLE_OFFSET = 58.0f;
static const float SECTION_GAP = 26.0f;
static const float ROW_HEIGHT = 32.0f;
static const float ROW_GAP = 4.0f;
static const float PADDING = 26.0f;
static const float BOTTOM_PAD = 22.0f;
static const int ROW_COUNT = 10;    // 9 values plus Back
static const int SECTION_COUNT = 3; // Graphics, Gameplay, Audio

static const int RESOLUTIONS[][2] = { { 0, 0 }, { 1280, 720 }, { 1600, 900 }, { 1920, 1080 } };
static const int RESOLUTION_COUNT = 4;

static void ApplyGraphics(const GameSettings &settings)
{
    if (settings.fullscreen != IsWindowFullscreen())
        ToggleFullscreen();

    if (!settings.fullscreen && settings.resolutionIndex > 0)
        SetWindowSize(RESOLUTIONS[settings.resolutionIndex][0], RESOLUTIONS[settings.resolutionIndex][1]);
}

static int Clamp(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

float SettingsMenu::PreferredWidth()
{
    return 520.0f * uistyle::Scale();
}

float SettingsMenu::PreferredHeight()
{
    // One extra section gap separates Back from the last row.
    return (TITLE_OFFSET + (SECTION_COUNT + 1) * SECTION_GAP +
            ROW_COUNT * (ROW_HEIGHT + ROW_GAP) + BOTTOM_PAD) * uistyle::Scale();
}

// Esc is deliberately not handled here: the scene that owns the panel closes it,
// so the key is never consumed twice in one frame.
bool SettingsMenu::Draw(GameSettings &settings, Rectangle area, SettingsBackground background)
{
    const float scale = uistyle::Scale();

    // Over the showcase the panel has to make itself the focal element, since
    // nothing else on that screen dims. Light enough that the arena and the car
    // still read behind it; the pause menu has already laid down its own.
    if (background == SettingsBackground::Showcase)
        uistyle::DrawDimmer(0.35f);

    uistyle::DrawPanel(area);
    uistyle::DrawTextAt("SETTINGS", area.x + PADDING * scale, area.y + 18.0f * scale, 26.0f, uistyle::Text);

    menu.itemCount = ROW_COUNT;
    menu.Update();

    Rectangle row = { area.x + PADDING * scale, area.y + TITLE_OFFSET * scale,
                      area.width - PADDING * scale * 2.0f, ROW_HEIGHT * scale };
    int index = 0;
    int delta = 0;

    uistyle::DrawTextAt("GRAPHICS", row.x, row.y, 16.0f, uistyle::Accent);
    row.y += SECTION_GAP * scale;

    delta = menu.ValueItem(row, "Fullscreen", settings.fullscreen ? "Yes" : "No", index++);
    if (delta != 0)
    {
        settings.fullscreen = !settings.fullscreen;
        ApplyGraphics(settings);
    }
    row.y += (ROW_HEIGHT + ROW_GAP) * scale;

    const char *resolutionText = settings.resolutionIndex == 0
        ? TextFormat("Custom (%ix%i)", GetScreenWidth(), GetScreenHeight())
        : TextFormat("%ix%i", RESOLUTIONS[settings.resolutionIndex][0], RESOLUTIONS[settings.resolutionIndex][1]);
    delta = menu.ValueItem(row, "Resolution", resolutionText, index++);
    if (delta != 0)
    {
        settings.resolutionIndex = (settings.resolutionIndex + delta + RESOLUTION_COUNT) % RESOLUTION_COUNT;
        ApplyGraphics(settings);
    }
    row.y += (ROW_HEIGHT + ROW_GAP) * scale;

    delta = menu.ValueItem(row, "Post-processing", settings.postProcessing ? "On" : "Off", index++);
    if (delta != 0)
        settings.postProcessing = !settings.postProcessing;
    row.y += (ROW_HEIGHT + ROW_GAP) * scale;

    uistyle::DrawTextAt("GAMEPLAY", row.x, row.y + 4.0f * scale, 16.0f, uistyle::Accent);
    row.y += SECTION_GAP * scale;

    delta = menu.ValueItem(row, "Camera sensitivity", TextFormat("%.1f", settings.cameraSensitivity), index++);
    if (delta != 0)
    {
        settings.cameraSensitivity += delta * 0.1f;
        settings.cameraSensitivity = Clamp((int)(settings.cameraSensitivity * 10.0f + 0.5f), 5, 20) / 10.0f;
    }
    row.y += (ROW_HEIGHT + ROW_GAP) * scale;

    delta = menu.ValueItem(row, "Match duration", TextFormat("%i min", settings.matchDurationMinutes), index++);
    if (delta != 0)
        settings.matchDurationMinutes = Clamp(settings.matchDurationMinutes + delta, 1, 10);
    row.y += (ROW_HEIGHT + ROW_GAP) * scale;

    delta = menu.ValueItem(row, "Bot opponent", settings.botEnabled ? "Enabled" : "Solo practice", index++);
    if (delta != 0)
        settings.botEnabled = !settings.botEnabled;
    row.y += (ROW_HEIGHT + ROW_GAP) * scale;

    uistyle::DrawTextAt("AUDIO", row.x, row.y + 4.0f * scale, 16.0f, uistyle::Accent);
    row.y += SECTION_GAP * scale;

    delta = menu.ValueItem(row, "Master volume", TextFormat("%i%%", settings.masterVolume), index++);
    if (delta != 0)
        settings.masterVolume = Clamp(settings.masterVolume + delta * 5, 0, 100);
    row.y += (ROW_HEIGHT + ROW_GAP) * scale;

    delta = menu.ValueItem(row, "SFX volume", TextFormat("%i%%", settings.sfxVolume), index++);
    if (delta != 0)
        settings.sfxVolume = Clamp(settings.sfxVolume + delta * 5, 0, 100);
    row.y += (ROW_HEIGHT + ROW_GAP) * scale;

    delta = menu.ValueItem(row, "Music volume", TextFormat("%i%%", settings.musicVolume), index++);
    if (delta != 0)
        settings.musicVolume = Clamp(settings.musicVolume + delta * 5, 0, 100);
    row.y += (ROW_HEIGHT + ROW_GAP + SECTION_GAP) * scale;

    bool back = menu.Item(row, "Back", index++);

    return !back;
}
