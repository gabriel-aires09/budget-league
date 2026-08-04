#include "UserInterface.h"

#include "AudioSystem.h"

namespace uistyle
{
    // A standalone button has no identity across frames, so the hover cue is
    // keyed on the rectangle plus the last frame it was seen on: moving to a
    // different button, or coming back to the same one after leaving every
    // button, both read as a new hover.
    static Rectangle hoveredButton = {};
    static double hoveredButtonTime = -1.0;

    float Scale()
    {
        float scale = GetScreenHeight() / 720.0f;
        if (scale < 0.7f)
            return 0.7f;
        if (scale > 2.0f)
            return 2.0f;
        return scale;
    }

    int FontSize(float baseSize)
    {
        return (int)(baseSize * Scale());
    }

    void DrawPanel(Rectangle bounds)
    {
        DrawRectangleRec(bounds, Panel);
        DrawRectangleLinesEx(bounds, 2.0f * Scale(), Border);
    }

    void DrawTitle(const char *text, float centerX, float y, float baseSize)
    {
        int size = FontSize(baseSize);
        float width = (float)MeasureText(text, size);
        DrawText(text, (int)(centerX - width * 0.5f), (int)y, size, Text);
        DrawRectangle((int)(centerX - width * 0.5f), (int)(y + size * 1.15f), (int)width,
                      (int)(3.0f * Scale()), Accent);
    }

    void DrawTextAt(const char *text, float x, float y, float baseSize, Color color)
    {
        DrawText(text, (int)x, (int)y, FontSize(baseSize), color);
    }

    void DrawTextCentered(const char *text, float centerX, float y, float baseSize, Color color)
    {
        int size = FontSize(baseSize);
        DrawText(text, (int)(centerX - MeasureText(text, size) * 0.5f), (int)y, size, color);
    }

    void DrawDimmer(float alpha)
    {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, alpha));
    }

    void DrawShadowedText(const char *text, float centerX, float y, float baseSize, Color color)
    {
        float offset = 2.0f * Scale();
        DrawTextCentered(text, centerX + offset, y + offset, baseSize, Fade(Background, 0.85f));
        DrawTextCentered(text, centerX, y, baseSize, color);
    }

    bool Button(Rectangle bounds, const char *label, bool primary)
    {
        bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
        if (hovered)
        {
            if (GetTime() - hoveredButtonTime > 0.1 || bounds.x != hoveredButton.x ||
                bounds.y != hoveredButton.y)
                audio::Play(AudioCue::UiHover);
            hoveredButton = bounds;
            hoveredButtonTime = GetTime();
        }

        DrawRectangleRec(bounds, hovered ? PanelHighlight : Panel);
        DrawRectangleLinesEx(bounds, 2.0f * Scale(), (hovered || primary) ? Accent : Border);
        DrawTextCentered(label, bounds.x + bounds.width * 0.5f,
                         bounds.y + (bounds.height - FontSize(21.0f)) * 0.5f, 21.0f,
                         (hovered || primary) ? Text : TextDim);

        bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (clicked)
            audio::Play(AudioCue::UiClick);
        return clicked;
    }

    // Shared visuals of a menu row. Returns true when the row is the selected one.
    static bool DrawRow(MenuList &menu, Rectangle bounds, const char *label, int index)
    {
        // The mouse only takes over the selection when it actually moves.
        Vector2 mouseDelta = GetMouseDelta();
        bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
        if (hovered && (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f))
        {
            if (menu.selected != index)
                audio::Play(AudioCue::UiHover);
            menu.selected = index;
        }

        bool selected = (menu.selected == index);
        DrawRectangleRec(bounds, selected ? PanelHighlight : Panel);
        if (selected)
            DrawRectangleRec(Rectangle{ bounds.x, bounds.y, 4.0f * Scale(), bounds.height }, Accent);
        DrawRectangleLinesEx(bounds, 1.0f, selected ? Accent : Border);

        int size = FontSize(21.0f);
        DrawText(label, (int)(bounds.x + 18.0f * Scale()),
                 (int)(bounds.y + (bounds.height - size) * 0.5f), size, selected ? Text : TextDim);
        return selected;
    }

    void MenuList::Update()
    {
        if (itemCount <= 0)
            return;

        int previous = selected;
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
            selected = (selected + 1) % itemCount;
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
            selected = (selected + itemCount - 1) % itemCount;
        if (selected != previous)
            audio::Play(AudioCue::UiHover);
    }

    bool MenuList::Item(Rectangle bounds, const char *label, int index)
    {
        bool selected = DrawRow(*this, bounds, label, index);

        bool clicked = CheckCollisionPointRec(GetMousePosition(), bounds) &&
                       IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool activated = selected && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                                      IsKeyPressed(KEY_SPACE));
        if (clicked || activated)
            audio::Play(AudioCue::UiClick);
        return clicked || activated;
    }

    int MenuList::ValueItem(Rectangle bounds, const char *label, const char *value, int index)
    {
        bool selected = DrawRow(*this, bounds, label, index);

        int size = FontSize(21.0f);
        float textY = bounds.y + (bounds.height - size) * 0.5f;
        float valueRight = bounds.x + bounds.width - 18.0f * Scale();
        Color valueColor = selected ? Accent : TextDim;

        DrawText(value, (int)(valueRight - MeasureText(value, size)), (int)textY, size, valueColor);
        if (selected)
        {
            // Arrows hint that left/right change the value.
            DrawText("<", (int)(valueRight - MeasureText(value, size) - 26.0f * Scale()), (int)textY, size, Accent);
            DrawText(">", (int)(valueRight + 12.0f * Scale()), (int)textY, size, Accent);
        }

        if (!selected)
            return 0;

        int delta = 0;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
            delta = 1;
        else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
            delta = -1;
        // Clicking or pressing enter cycles forward.
        else if ((CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ||
                 IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
            delta = 1;

        if (delta != 0)
            audio::Play(AudioCue::UiClick);
        return delta;
    }
}
