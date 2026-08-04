#ifndef AUDIOSYSTEM_H
#define AUDIOSYSTEM_H

// Every sound the game makes is generated at startup from the table in
// AudioSystem.cpp. There are no sound files and nothing for the cooker to do.
//
// Like effects, it decides nothing: the scenes call Play from the events they
// already watch for the HUD and the particles.
enum class AudioCue
{
    UiHover,
    UiClick,
    Jump,          // pitched down for the second jump / flip
    BallHit,       // the car hitting the ball
    Impact,        // the car hitting a wall or the other car
    BoostPad,
    CountdownTick, // one per second of the kickoff countdown
    CountdownGo,   // the field going live
    Goal,
    MatchEnd,
    Count
};

namespace audio
{
    // Opens the audio device and builds every cue. Runs silent, without failing,
    // when there is no device - which is the usual case under the smoke test.
    void Load();
    void Unload();

    // From GameSettings, both 0..100. Cheap enough to call every frame, which is
    // what makes the volume sliders felt as they move.
    void SetVolumes(int masterPercent, int sfxPercent);

    void Play(AudioCue cue, float volume = 1.0f, float pitch = 1.0f);

    // The one cue that is held rather than fired. It is a running audio stream,
    // not a repeated sound, so it neither clicks nor gaps while boost is held.
    void SetBoost(bool active);
}

#endif
