#include "HUD.h"

#include "UserInterface.h"

#include <cmath>

// Brighter than uistyle::Boost, for the moment the boost is actually firing.
static const Color BoostActive = { 255, 232, 150, 255 };

// HUD text drawn over the field rather than over a panel, where the background
// can be any brightness - the countdown in particular lands over the far goal.
static void DrawShadowedText(const char *text, float centerX, float y, float baseSize, Color color)
{
    float offset = 2.0f * uistyle::Scale();
    uistyle::DrawTextCentered(text, centerX + offset, y + offset, baseSize,
                              Fade(uistyle::Background, 0.85f));
    uistyle::DrawTextCentered(text, centerX, y, baseSize, color);
}

// Score, clock and both team blocks, centred at the top of the screen.
static void DrawScoreboard(const Match &match)
{
    const float scale = uistyle::Scale();
    const float width = 340.0f * scale;
    const float height = 66.0f * scale;
    Rectangle panel = { GetScreenWidth() * 0.5f - width * 0.5f, 12.0f * scale, width, height };
    uistyle::DrawPanel(panel);

    const float blockWidth = 96.0f * scale;
    Rectangle blue = { panel.x + 6.0f * scale, panel.y + 6.0f * scale, blockWidth, height - 12.0f * scale };
    Rectangle orange = { panel.x + panel.width - blockWidth - 6.0f * scale, blue.y, blockWidth, blue.height };
    DrawRectangleRec(blue, Fade(uistyle::TeamBlue, 0.22f));
    DrawRectangleRec(orange, Fade(uistyle::TeamOrange, 0.22f));

    uistyle::DrawTextCentered(TextFormat("%d", match.scoreBlue), blue.x + blue.width * 0.5f,
                              blue.y + 6.0f * scale, 40.0f, uistyle::TeamBlue);
    uistyle::DrawTextCentered(TextFormat("%d", match.scoreOrange), orange.x + orange.width * 0.5f,
                              orange.y + 6.0f * scale, 40.0f, uistyle::TeamOrange);

    // The clock turns red for the last half minute, which is the one moment in a
    // match when the exact number matters.
    int seconds = (int)ceilf(match.timeRemaining);
    Color clockColor = (match.timeRemaining <= 30.0f && match.state != MatchState::Finished)
                           ? uistyle::Warning : uistyle::Text;
    uistyle::DrawTextCentered(TextFormat("%d:%02d", seconds / 60, seconds % 60),
                              panel.x + panel.width * 0.5f, panel.y + 18.0f * scale, 30.0f, clockColor);
}

// The 0-100 meter, bottom right.
static void DrawBoostMeter(const CarObject &car)
{
    const float scale = uistyle::Scale();
    const float width = 230.0f * scale;
    const float height = 26.0f * scale;
    Rectangle bar = { GetScreenWidth() - width - 30.0f * scale,
                      GetScreenHeight() - height - 42.0f * scale, width, height };

    float fraction = car.boostCapacity > 0.0f ? car.boostAmount / car.boostCapacity : 0.0f;
    DrawRectangleRec(bar, uistyle::Panel);
    DrawRectangleRec(Rectangle{ bar.x, bar.y, bar.width * fraction, bar.height },
                     car.boosting ? BoostActive : uistyle::Boost);
    // Quarter ticks, so a glance reads roughly how much is left without reading
    // the number.
    for (int tick = 1; tick < 4; ++tick)
        DrawRectangleRec(Rectangle{ bar.x + bar.width * 0.25f * tick, bar.y, 1.0f * scale, bar.height },
                         Fade(uistyle::Background, 0.6f));
    DrawRectangleLinesEx(bar, 2.0f * scale, uistyle::Border);

    uistyle::DrawTextAt("BOOST", bar.x, bar.y - 26.0f * scale, 18.0f, uistyle::TextDim);

    const char *amount = TextFormat("%.0f", car.boostAmount);
    int size = uistyle::FontSize(34.0f);
    DrawText(amount, (int)(bar.x - 14.0f * scale - MeasureText(amount, size)),
             (int)(bar.y - 6.0f * scale), size, car.boosting ? BoostActive : uistyle::Text);
}

// Speed, bottom left. It was a debug readout until now; an arcade racer shows it.
static void DrawSpeed(const CarObject &car)
{
    const float scale = uistyle::Scale();
    const char *value = TextFormat("%.0f", fabsf(car.GetForwardSpeed()) * 3.6f);
    int size = uistyle::FontSize(38.0f);
    float x = 30.0f * scale;
    float y = GetScreenHeight() - 68.0f * scale;

    DrawText(value, (int)x, (int)y, size, uistyle::Text);
    uistyle::DrawTextAt("KM/H", x + MeasureText(value, size) + 8.0f * scale,
                        y + 20.0f * scale, 18.0f, uistyle::TextDim);
}

static void DrawKickoff(const Match &match)
{
    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;

    // Shadowed, because the countdown lands over the far goal, which is the one
    // bright thing at the centre of the screen.
    DrawShadowedText("KICKOFF", centerX, 152.0f * scale, 24.0f, uistyle::TextDim);

    int count = (int)ceilf(match.stateTimer);
    if (count <= 0)
    {
        uistyle::DrawTitle("GO", centerX, 190.0f * scale, 90.0f);
        return;
    }

    // Each number swells as it lands and settles over its second, so the
    // countdown reads as a beat rather than a static digit. The y offset keeps
    // it centred while the size changes, since DrawTextCentered grows downwards.
    float settle = match.stateTimer - floorf(match.stateTimer);
    float size = 72.0f + 26.0f * settle * settle;
    DrawShadowedText(TextFormat("%d", count), centerX, (190.0f + (98.0f - size) * 0.5f) * scale,
                 size, uistyle::Text);
}

static void DrawCelebration(const Match &match)
{
    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;
    Color team = match.lastScoringTeam == 0 ? uistyle::TeamBlue : uistyle::TeamOrange;

    // The band wipes open as the goal goes in and closes again before the field
    // resets, so it never simply pops off the screen. Both edges come from
    // stateTimer, so the HUD needs no timer of its own.
    float opening = (match.celebrationTime - match.stateTimer) / 0.25f;
    float closing = match.stateTimer / 0.35f;
    float wipe = fminf(fminf(opening, closing), 1.0f);
    if (wipe <= 0.0f)
        return;

    // A short flash of the scoring team's colour over the whole screen, which is
    // the punch the celebration was missing. It is over in a third of a second.
    float flash = 1.0f - (match.celebrationTime - match.stateTimer) / 0.35f;
    if (flash > 0.0f)
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(team, 0.45f * flash));

    float height = 150.0f * scale * wipe;
    float centerY = GetScreenHeight() * 0.38f;
    Rectangle band = { 0.0f, centerY - height * 0.5f, (float)GetScreenWidth(), height };
    DrawRectangleRec(band, Fade(team, 0.30f));
    DrawRectangleRec(Rectangle{ band.x, band.y, band.width, 3.0f * scale }, team);
    DrawRectangleRec(Rectangle{ band.x, band.y + band.height - 3.0f * scale, band.width, 3.0f * scale }, team);

    uistyle::DrawTextCentered("GOAL!", centerX, centerY - 62.0f * scale, 76.0f, Fade(team, wipe));
    uistyle::DrawTextCentered(match.lastScoringTeam == 0 ? "BLUE SCORES" : "ORANGE SCORES",
                              centerX, centerY + 26.0f * scale, 26.0f, Fade(uistyle::Text, wipe));
}

void hud::Draw(const Match &match, const CarObject &car)
{
    DrawScoreboard(match);
    DrawBoostMeter(car);
    DrawSpeed(car);

    if (match.state == MatchState::Kickoff)
        DrawKickoff(match);
    else if (match.state == MatchState::Celebration)
        DrawCelebration(match);
}

MenuAction hud::DrawFullTime(const Match &match)
{
    const float scale = uistyle::Scale();
    const float centerX = GetScreenWidth() * 0.5f;
    uistyle::DrawDimmer(0.6f);

    const float width = 460.0f * scale;
    const float height = 300.0f * scale;
    Rectangle panel = { centerX - width * 0.5f, GetScreenHeight() * 0.5f - height * 0.5f, width, height };
    uistyle::DrawPanel(panel);

    uistyle::DrawTitle("FULL TIME", centerX, panel.y + 26.0f * scale, 44.0f);

    // The final score, each half in its own team colour.
    int size = uistyle::FontSize(56.0f);
    const char *blue = TextFormat("%d", match.scoreBlue);
    const char *orange = TextFormat("%d", match.scoreOrange);
    float blueWidth = (float)MeasureText(blue, size);
    float orangeWidth = (float)MeasureText(orange, size);
    float dashWidth = (float)MeasureText("-", size);
    float gap = 22.0f * scale;
    float x = centerX - (blueWidth + orangeWidth + dashWidth + gap * 2.0f) * 0.5f;
    float y = panel.y + 104.0f * scale;

    DrawText(blue, (int)x, (int)y, size, uistyle::TeamBlue);
    x += blueWidth + gap;
    DrawText("-", (int)x, (int)y, size, uistyle::TextDim);
    x += dashWidth + gap;
    DrawText(orange, (int)x, (int)y, size, uistyle::TeamOrange);

    bool drawn = match.scoreBlue == match.scoreOrange;
    bool blueWon = match.scoreBlue > match.scoreOrange;
    uistyle::DrawTextCentered(drawn ? "DRAW" : (blueWon ? "BLUE WINS" : "ORANGE WINS"), centerX,
                              panel.y + 180.0f * scale, 30.0f,
                              drawn ? uistyle::Text : (blueWon ? uistyle::TeamBlue : uistyle::TeamOrange));

    const float buttonWidth = 190.0f * scale;
    const float buttonHeight = 48.0f * scale;
    float buttonY = panel.y + height - buttonHeight - 26.0f * scale;
    // Both are drawn before either is acted on, so a click never leaves the other
    // button missing for a frame.
    bool rematch = uistyle::Button(Rectangle{ centerX - buttonWidth - 8.0f * scale, buttonY,
                                              buttonWidth, buttonHeight }, "REMATCH", true);
    bool mainMenu = uistyle::Button(Rectangle{ centerX + 8.0f * scale, buttonY,
                                               buttonWidth, buttonHeight }, "MAIN MENU", false);

    // Enter repeats the match, as on the other screens where the primary action
    // is also on the keyboard. Esc is not read here: it belongs to the pause menu.
    if (rematch || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
        return MenuAction::StartMatch;
    if (mainMenu)
        return MenuAction::MainMenu;

    return MenuAction::None;
}
