#ifndef HUD_H
#define HUD_H

#include "Match.h"
#include "MenuAction.h"
#include "GameObjects/CarObject.h"

// The in-match HUD: score, clock, boost meter, speed, kickoff countdown and the
// goal celebration.
//
// It is a namespace rather than a class because it holds no state at all -
// everything it shows is read from the Match and the car on the frame it draws.
namespace hud
{
    void Draw(const Match &match, const CarObject &car);

    // The end-of-match screen. Separate from Draw because it takes input, so
    // MatchScene only calls it while the pause menu is not up. Returns
    // StartMatch for a rematch, MainMenu to leave, None otherwise.
    MenuAction DrawFullTime(const Match &match);
}

#endif
