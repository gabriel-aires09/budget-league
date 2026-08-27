#ifndef GAMEPADINPUT_H
#define GAMEPADINPUT_H

#include "GameSettings.h"

// The one place that knows raylib's gamepad ids, the button layout, the dead
// zone and the trigger curve. Everything else asks in game terms — Throttle,
// Jump, MenuConfirm — and never sees a GAMEPAD_BUTTON_* constant.
//
// The pad is a second input source beside the keyboard, never a replacement:
// both are live at once and neither has a mode (CLAUDE.md Milestone 22).
//
// Layout (Xbox names), from the milestone's table:
//   RT accelerate, LT reverse, left stick steer, A jump, B boost, Y ball cam,
//   X air roll (held, the stick rolls instead of yawing), RB reset, Start pause,
//   left stick or D-pad move a menu, A confirms, B cancels.
namespace gamepad
{
    // Polled once per frame by App, before the scene runs: every query below
    // reads what this stored. It has to be one call rather than reads scattered
    // through the frame, because the edges (a button going down, a stick leaving
    // the centre) can only be found by comparing against the previous frame, and
    // a menu row that asks twice in one frame must get the same answer twice.
    void Update(const GameSettings &settings);

    // A pad is plugged in and the settings have it switched on.
    bool Available();
    // Which device the player last touched, for the prompts that name a button.
    bool LastDeviceWasGamepad();

    // --- Driving. Analogue, already dead-zoned and curved.
    float Throttle(); // -1 .. 1, right trigger forward, left trigger back
    float Steer();    // -1 .. 1, left stick
    float AirPitch(); // left stick, nose down on forward, as W does
    float AirYaw();   // left stick, zero while the air roll button is held
    float AirRoll();  // left stick, only while that button is held
    bool Boost();     // held
    bool Jump();      // held: CarObject finds the rising edge itself
    bool ResetCar();  // the frame it was pressed

    // --- Match actions, true for the frame the button went down on.
    bool Pause();
    bool BallCam();

    // --- Menus, true for the frame they were pressed. The stick counts as a
    // press when it leaves the centre and re-arms when it comes back, so holding
    // it does not run down the list.
    bool MenuUp();
    bool MenuDown();
    bool MenuLeft();
    bool MenuRight();
    bool MenuConfirm();
    bool MenuCancel();

    // Short pulse on both motors. Silent when no pad is connected or vibration
    // is switched off, so callers never have to ask.
    void Rumble(float strength, float seconds);
}

#endif
