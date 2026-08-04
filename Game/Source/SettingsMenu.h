#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H

#include "GameSettings.h"
#include "UserInterface.h"

// What the panel is being drawn on top of, which is the one thing that differs
// between its two homes (CLAUDE.md Milestone 19).
enum class SettingsBackground
{
    Dimmed,  // the pause menu has already darkened the whole screen
    Showcase // the main menu's live arena, which stays visible behind the panel
};

// Settings panel reused by the main menu and the pause menu.
class SettingsMenu
{
public:
    // Draws the panel and handles its input (raylib is immediate mode, so both
    // happen together). Returns false when the user closed it.
    bool Draw(GameSettings &settings, Rectangle area, SettingsBackground background);

    // Size the panel needs for all of its rows, so callers never clip it.
    static float PreferredWidth();
    static float PreferredHeight();

    uistyle::MenuList menu;
};

#endif
