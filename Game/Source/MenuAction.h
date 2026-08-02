#ifndef MENUACTION_H
#define MENUACTION_H

// What a scene asks App to do. Resume and the settings panel are handled inside
// the scenes themselves, so they never reach App.
enum class MenuAction
{
    None,
    StartMatch,
    MainMenu,
    ExitGame
};

#endif
