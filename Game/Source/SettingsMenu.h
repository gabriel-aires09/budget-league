#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H

#include "GameSettings.h"
#include "UserInterface.h"

// Settings panel reused by the main menu and the pause menu.
class SettingsMenu
{
public:
    // Draws the panel and handles its input (raylib is immediate mode, so both
    // happen together). Returns false when the user closed it.
    bool Draw(GameSettings &settings, Rectangle area);

    // Size the panel needs for all of its rows, so callers never clip it.
    static float PreferredWidth();
    static float PreferredHeight();

    uistyle::MenuList menu;
};

#endif
