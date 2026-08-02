#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include <raylib.h>

// Single source of truth for the menu and HUD look. Every UI draw goes through
// these colors and helpers.
namespace uistyle
{
    const Color Background = { 12, 14, 22, 255 };
    const Color Panel = { 22, 27, 40, 240 };
    const Color PanelHighlight = { 34, 46, 70, 245 };
    const Color Border = { 58, 72, 100, 255 };
    const Color Accent = { 90, 190, 255, 255 };
    const Color Text = { 232, 238, 248, 255 };
    const Color TextDim = { 138, 154, 182, 255 };
    const Color TeamBlue = { 60, 140, 255, 255 };
    const Color TeamOrange = { 255, 142, 48, 255 };
    const Color Warning = { 255, 96, 96, 255 };

    // The UI is laid out for a 720p window and scaled from there.
    float Scale();
    int FontSize(float baseSize);

    void DrawPanel(Rectangle bounds);
    void DrawTitle(const char *text, float centerX, float y, float baseSize);
    void DrawTextAt(const char *text, float x, float y, float baseSize, Color color);
    void DrawTextCentered(const char *text, float centerX, float y, float baseSize, Color color);
    void DrawDimmer(float alpha);

    // Vertical menu driven by both keyboard and mouse. Set itemCount, call
    // Update once, then draw the rows.
    struct MenuList
    {
        int selected = 0;
        int itemCount = 0;

        void Update();
        // Plain row: true when it is activated.
        bool Item(Rectangle bounds, const char *label, int index);
        // Row with a value on the right: -1, 0 or +1.
        int ValueItem(Rectangle bounds, const char *label, const char *value, int index);
    };
}

#endif
