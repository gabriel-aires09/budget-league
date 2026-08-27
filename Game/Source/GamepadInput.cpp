#include "GamepadInput.h"

#include <raylib.h>

#include <cmath>

namespace gamepad
{
    // How far the stick has to be pushed before a menu reads it as a direction.
    // Well past the dead zone, so a resting hand never moves the selection.
    static const float MENU_THRESHOLD = 0.55f;
    // Slack at the bottom of a trigger, after it has been mapped to 0..1. A pad
    // that rests a hair off zero must not creep the car forwards on the kickoff.
    static const float TRIGGER_FLOOR = 0.06f;
    static const int MAX_PADS = 4;

    // Everything the frame's queries answer from. Two copies of the buttons, so
    // an edge is just a comparison.
    struct State
    {
        bool available = false;
        bool lastDeviceWasGamepad = false;
        bool vibration = true;
        int pad = 0;

        float throttle = 0.0f;
        float steer = 0.0f;
        float stickX = 0.0f;
        float stickY = 0.0f;
        bool boost = false;
        bool jump = false;
        bool airRollHeld = false;

        bool reset = false;
        bool pause = false;
        bool ballCam = false;
        bool menuUp = false;
        bool menuDown = false;
        bool menuLeft = false;
        bool menuRight = false;
        bool menuConfirm = false;
        bool menuCancel = false;
    };

    static State state;

    // Held-down flags from the previous frame, which is all an edge needs. The
    // stick directions are in here too: to a menu they are buttons.
    struct Held
    {
        bool reset = false;
        bool pause = false;
        bool ballCam = false;
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
        bool confirm = false;
        bool cancel = false;
    };

    static Held previous;

    // Which convention each trigger reports in, per pad; see Trigger. Forgotten
    // when a pad goes away, so plugging a different one in re-learns.
    static bool leftTriggerSigned[MAX_PADS] = {};
    static bool rightTriggerSigned[MAX_PADS] = {};

    // Radial rather than per-axis: a per-axis dead zone squares off the circle
    // the stick actually moves in, so a diagonal push clips to the corner and
    // reads as full deflection on both axes. Past the edge the value is rescaled
    // from 0, so the first millimetre of travel is not a step change.
    static void DeadZone(float &x, float &y, float deadZone)
    {
        float magnitude = sqrtf(x * x + y * y);
        if (magnitude <= deadZone || magnitude <= 0.0001f)
        {
            x = 0.0f;
            y = 0.0f;
            return;
        }

        float scaled = (magnitude - deadZone) / (1.0f - deadZone);
        if (scaled > 1.0f)
            scaled = 1.0f;
        x = x / magnitude * scaled;
        y = y / magnitude * scaled;
    }

    // A trigger is reported over -1 .. 1 and rests at -1, so it is remapped to
    // 0 .. 1 here and every caller gets a plain "how far in is it".
    //
    // `signedSeen` is why this is not a one-liner. raylib's axis array is zero
    // until it has polled the pad, which happens in EndDrawing — so on the very
    // first frame a trigger reads 0, and 0 through the remap is half throttle:
    // the car drives off on its own before a frame has been drawn. Measured on
    // this machine, which enumerates its keyboard as a six-axis pad. Until a
    // trigger has been seen at a real resting value the raw number is taken as
    // already being 0 .. 1, which is both what an unpolled axis wants (zero) and
    // what the drivers that report triggers unsigned want.
    static float Trigger(int pad, int axis, bool &signedSeen)
    {
        float raw = GetGamepadAxisMovement(pad, axis);
        if (raw <= -0.5f)
            signedSeen = true;

        float value = signedSeen ? (raw + 1.0f) * 0.5f : (raw < 0.0f ? 0.0f : raw);
        if (value <= TRIGGER_FLOOR)
            return 0.0f;
        return (value - TRIGGER_FLOOR) / (1.0f - TRIGGER_FLOOR);
    }

    // Keyboard activity, for the prompts. GetKeyPressed is deliberately not used:
    // it pops raylib's queue, and the title screen is reading that same queue to
    // leave itself (Milestone 17).
    static bool KeyboardInUse()
    {
        static const int KEYS[] = {
            KEY_W, KEY_A, KEY_S, KEY_D, KEY_Q, KEY_E, KEY_R, KEY_C, KEY_P,
            KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SPACE, KEY_ENTER, KEY_KP_ENTER,
            KEY_ESCAPE, KEY_BACKSPACE, KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT
        };

        for (int key : KEYS)
        {
            if (IsKeyDown(key))
                return true;
        }
        return false;
    }

    void Update(const GameSettings &settings)
    {
        Held now;
        State fresh;
        fresh.lastDeviceWasGamepad = state.lastDeviceWasGamepad;
        fresh.vibration = settings.vibrationEnabled;
        fresh.pad = state.pad; // the last pad that did something keeps the rumble

        // Every pad that is there, not the first one — and this is not multiple
        // players, it is one player and a machine that lies about its hardware.
        // raylib's IsGamepadAvailable is "some joystick is present", and this
        // machine presents its keyboard as a six-axis joystick at index 0, so
        // reading only the first pad read the keyboard and the real controller
        // plugged in behind it was never touched. Merging costs nothing: a
        // joystick GLFW has no gamepad mapping for reports all-zero buttons and
        // resting triggers, so it contributes exactly nothing to the result.
        for (int i = 0; i < MAX_PADS; ++i)
        {
            if (!IsGamepadAvailable(i))
            {
                leftTriggerSigned[i] = rightTriggerSigned[i] = false;
                continue;
            }

            if (!settings.gamepadEnabled)
                continue;

            fresh.available = true;

            float stickX = GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_X);
            float stickY = GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_Y);
            DeadZone(stickX, stickY, settings.stickDeadZone);

            // Analogue on purpose: a half-pressed trigger is half throttle and a
            // small tilt is a small steering angle, which is the whole reason a
            // pad is not just a keyboard with a stick.
            float throttle = Trigger(i, GAMEPAD_AXIS_RIGHT_TRIGGER, rightTriggerSigned[i]) -
                             Trigger(i, GAMEPAD_AXIS_LEFT_TRIGGER, leftTriggerSigned[i]);

            bool jump = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);       // A
            bool boost = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);     // B
            bool airRoll = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);    // X
            bool reset = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);      // RB
            bool pause = IsGamepadButtonDown(i, GAMEPAD_BUTTON_MIDDLE_RIGHT);         // Start
            bool ballCam = IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_UP);      // Y

            // Strongest wins for the analogue values, held is held for the rest.
            if (fabsf(throttle) > fabsf(fresh.throttle))
                fresh.throttle = throttle;
            if (fabsf(stickX) > fabsf(fresh.stickX))
                fresh.stickX = stickX;
            if (fabsf(stickY) > fabsf(fresh.stickY))
                fresh.stickY = stickY;

            fresh.jump = fresh.jump || jump;
            fresh.boost = fresh.boost || boost;
            fresh.airRollHeld = fresh.airRollHeld || airRoll;

            now.reset = now.reset || reset;
            now.pause = now.pause || pause;
            now.ballCam = now.ballCam || ballCam;
            now.confirm = now.confirm || jump;   // A
            now.cancel = now.cancel || boost;    // B

            // A menu takes the D-pad and the stick as the same four buttons.
            now.up = now.up || IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_UP) || stickY < -MENU_THRESHOLD;
            now.down = now.down || IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || stickY > MENU_THRESHOLD;
            now.left = now.left || IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || stickX < -MENU_THRESHOLD;
            now.right = now.right || IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || stickX > MENU_THRESHOLD;

            // Whichever pad is actually being used is the one that gets rumbled
            // and the one that switches the prompts over to button names.
            bool active = fabsf(throttle) > 0.1f || fabsf(stickX) > 0.1f || fabsf(stickY) > 0.1f ||
                          jump || boost || airRoll || reset || pause || ballCam ||
                          IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_UP) ||
                          IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
                          IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
                          IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
            if (active)
            {
                fresh.pad = i;
                fresh.lastDeviceWasGamepad = true;
            }
        }

        fresh.steer = fresh.stickX;

        fresh.reset = now.reset && !previous.reset;
        fresh.pause = now.pause && !previous.pause;
        fresh.ballCam = now.ballCam && !previous.ballCam;
        fresh.menuUp = now.up && !previous.up;
        fresh.menuDown = now.down && !previous.down;
        fresh.menuLeft = now.left && !previous.left;
        fresh.menuRight = now.right && !previous.right;
        fresh.menuConfirm = now.confirm && !previous.confirm;
        fresh.menuCancel = now.cancel && !previous.cancel;

        if (KeyboardInUse())
            fresh.lastDeviceWasGamepad = false;

        previous = now;
        state = fresh;
    }

    bool Available() { return state.available; }
    bool LastDeviceWasGamepad() { return state.available && state.lastDeviceWasGamepad; }

    float Throttle() { return state.throttle; }
    float Steer() { return state.steer; }
    // Pitch is inverted against the stick the same way the keyboard inverts it
    // against the throttle: pushing forward puts the nose down.
    float AirPitch() { return state.stickY; }
    float AirYaw() { return state.airRollHeld ? 0.0f : state.stickX; }
    float AirRoll() { return state.airRollHeld ? state.stickX : 0.0f; }
    bool Boost() { return state.boost; }
    bool Jump() { return state.jump; }
    bool ResetCar() { return state.reset; }

    bool Pause() { return state.pause; }
    bool BallCam() { return state.ballCam; }

    bool MenuUp() { return state.menuUp; }
    bool MenuDown() { return state.menuDown; }
    bool MenuLeft() { return state.menuLeft; }
    bool MenuRight() { return state.menuRight; }
    bool MenuConfirm() { return state.menuConfirm; }
    bool MenuCancel() { return state.menuCancel; }

    void Rumble(float strength, float seconds)
    {
        if (!state.available || !state.vibration || strength <= 0.0f)
            return;

        if (strength > 1.0f)
            strength = 1.0f;
        SetGamepadVibration(state.pad, strength, strength, seconds);
    }
}
