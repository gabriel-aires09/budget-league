#include "AudioSystem.h"

#include "AssetPack.h"

#include <raylib.h>

#include <cmath>
#include <string>
#include <vector>

namespace
{
    const int SAMPLE_RATE = 44100;
    // Enough that two impacts or two ball hits in quick succession do not cut
    // each other off. A cue owns its data once and the rest are aliases, which
    // share it, so the extra voices cost nothing but their buffer state.
    const int VOICE_COUNT = 3;

    // One cue is one line of this. Everything is a swept oscillator plus noise
    // under an attack/decay envelope, which is enough for the whole list: a
    // thump is a fast downward sweep, a chime is an upward one with a fifth on
    // top, a crunch is mostly noise.
    struct CueSpec
    {
        float duration;   // seconds
        float startFreq;
        float endFreq;    // swept exponentially from startFreq
        float chordRatio; // second oscillator, as a ratio of the first; 0 = none
        float harmonics;  // weight of the third harmonic, which adds the edge
        float noiseMix;   // 0 pure tone .. 1 pure noise
        float noiseCut;   // one-pole lowpass on the noise, 1 = none
        float attack;     // seconds
        float decay;      // exponential decay rate, per second
        float amplitude;
    };

    // Indexed by AudioCue, so the order here follows the enum.
    const CueSpec CUES[(int)AudioCue::Count] = {
        // dur   f0     f1     chord  harm  noise  cut   atk    decay  amp
        { 0.06f, 900,   980,   0.0f,  0.20f, 0.00f, 1.0f, 0.004f, 45.0f, 0.16f }, // UiHover
        { 0.10f, 1500,  700,   0.0f,  0.40f, 0.05f, 0.5f, 0.002f, 32.0f, 0.30f }, // UiClick
        { 0.20f, 280,   760,   0.0f,  0.30f, 0.18f, 0.4f, 0.004f, 15.0f, 0.42f }, // Jump
        { 0.30f, 460,   85,    0.0f,  0.35f, 0.30f, 0.22f, 0.002f, 13.0f, 0.85f }, // BallHit
        { 0.22f, 260,   70,    0.0f,  0.15f, 0.70f, 0.14f, 0.001f, 18.0f, 0.60f }, // Impact
        { 0.30f, 620,   1240,  1.5f,  0.15f, 0.00f, 1.0f, 0.004f, 9.0f,  0.34f }, // BoostPad
        { 0.12f, 720,   720,   0.0f,  0.25f, 0.00f, 1.0f, 0.003f, 22.0f, 0.45f }, // CountdownTick
        { 0.36f, 980,   980,   1.5f,  0.30f, 0.00f, 1.0f, 0.003f, 7.0f,  0.55f }, // CountdownGo
        { 0.90f, 440,   880,   1.5f,  0.45f, 0.12f, 0.3f, 0.006f, 3.2f,  0.70f }, // Goal
        { 1.20f, 520,   190,   1.25f, 0.35f, 0.00f, 1.0f, 0.008f, 2.4f,  0.60f }, // MatchEnd
    };

    struct CueSound
    {
        Sound voices[VOICE_COUNT];
        int next;
    };

    CueSound cues[(int)AudioCue::Count] = {};
    bool ready = false;
    float sfxVolume = 0.8f;

    // The soundtrack. One track streams at a time; the rest are just names, so
    // the playlist costs nothing however long it gets.
    std::vector<std::string> trackNames;
    Music track = {};
    bool trackPlaying = false;
    int currentTrack = -1;
    float musicVolume = 0.7f;
    // The bytes of the track being played. raylib's memory decoder reads from
    // this buffer as it goes rather than copying it, so it has to outlive the
    // stream — and it is one track's worth, not the whole soundtrack's.
    unsigned char *trackData = nullptr;

    // Starts a track that is not the one just played, so the shuffle never
    // repeats a song back to back. With a single track it plays that one again.
    void StartAnotherTrack()
    {
        if (trackNames.empty())
            return;

        int next = GetRandomValue(0, (int)trackNames.size() - 1);
        if (trackNames.size() > 1)
            while (next == currentTrack)
                next = GetRandomValue(0, (int)trackNames.size() - 1);

        if (trackPlaying)
            UnloadMusicStream(track);
        assets::UnloadData(trackData); // only after the stream that was reading it is gone
        trackData = nullptr;

        // Streamed from memory rather than from a path, because a track inside
        // assets.pak has no path. It is still streamed: raylib decodes it as it
        // plays, and only the compressed MP3 of the one track is resident.
        int size = 0;
        trackData = assets::LoadData(trackNames[next], &size);
        track = trackData != nullptr ? LoadMusicStreamFromMemory(".mp3", trackData, size) : Music{};
        trackPlaying = IsMusicValid(track);
        if (!trackPlaying)
        {
            TraceLog(LOG_WARNING, "MUSIC: could not stream %s", trackNames[next].c_str());
            assets::UnloadData(trackData);
            trackData = nullptr;
            return;
        }

        // Off, or the playlist would never reach a second song.
        track.looping = false;
        currentTrack = next;
        SetMusicVolume(track, musicVolume);
        PlayMusicStream(track);
        TraceLog(LOG_INFO, "MUSIC: %s", GetFileName(trackNames[next].c_str()));
    }

    // The boost loop. gain is written by the game thread and read by the audio
    // one; it is a single float that only ever moves between 0 and 1, and the
    // callback smooths it, so the two never need to agree on an exact frame.
    AudioStream boostStream = {};
    float boostTarget = 0.0f;
    float boostGain = 0.0f;
    float boostPhase = 0.0f;
    float boostNoise = 0.0f;

    // Deliberately not GetRandomValue: the cues are built once at startup and
    // must not shift the sequence the effects draw from, and the boost callback
    // runs on the audio thread, where calling into raylib is not worth the risk.
    unsigned int rngState = 0x2545f491u;

    float NextNoise()
    {
        rngState = rngState * 1664525u + 1013904223u;
        return (float)((rngState >> 9) & 0xffff) / 32767.5f - 1.0f;
    }

    // The oscillator both the cue table and nothing else uses: a sine with some
    // third harmonic mixed in, normalised so harmonics only changes the tone.
    float Osc(float phase, float harmonics)
    {
        return (sinf(phase) + harmonics * sinf(phase * 3.0f) / 3.0f) / (1.0f + harmonics / 3.0f);
    }

    Sound BuildCue(const CueSpec &spec)
    {
        const int frames = (int)(spec.duration * SAMPLE_RATE);
        // MemAlloc, because UnloadWave frees the data with RL_FREE.
        short *samples = (short *)MemAlloc(frames * sizeof(short));

        // A one-pole lowpass takes most of the level out of white noise, so the
        // mix is compensated back up rather than every cue carrying the fix.
        const float noiseGain = 1.0f / sqrtf(spec.noiseCut);
        const float attack = fmaxf(spec.attack, 0.0005f);
        const int fadeFrames = SAMPLE_RATE / 250; // 4 ms, so a cue can never click off
        float phase = 0.0f;
        float chordPhase = 0.0f;
        float noiseState = 0.0f;

        for (int i = 0; i < frames; ++i)
        {
            const float t = (float)i / SAMPLE_RATE;
            const float u = t / spec.duration;
            const float frequency = spec.startFreq * powf(spec.endFreq / spec.startFreq, u);

            phase += 2.0f * PI * frequency / SAMPLE_RATE;
            float tone = Osc(phase, spec.harmonics);
            if (spec.chordRatio > 0.0f)
            {
                chordPhase += 2.0f * PI * frequency * spec.chordRatio / SAMPLE_RATE;
                tone = (tone + 0.7f * Osc(chordPhase, spec.harmonics)) / 1.7f;
            }

            noiseState += (NextNoise() - noiseState) * spec.noiseCut;

            float value = (1.0f - spec.noiseMix) * tone + spec.noiseMix * noiseState * noiseGain;
            value *= (t < attack ? t / attack : expf(-spec.decay * (t - attack))) * spec.amplitude;
            if (i > frames - fadeFrames)
                value *= (float)(frames - i) / fadeFrames;

            samples[i] = (short)(fmaxf(fminf(value, 1.0f), -1.0f) * 32000.0f);
        }

        Wave wave = {};
        wave.frameCount = (unsigned int)frames;
        wave.sampleRate = SAMPLE_RATE;
        wave.sampleSize = 16;
        wave.channels = 1;
        wave.data = samples;

        Sound sound = LoadSoundFromWave(wave);
        UnloadWave(wave);
        return sound;
    }

    // Rocket roar: lowpassed noise over a low tone, generated straight into the
    // stream so the loop has no seam to click on.
    void BoostCallback(void *buffer, unsigned int frames)
    {
        short *out = (short *)buffer;
        for (unsigned int i = 0; i < frames; ++i)
        {
            // About 25 ms to reach the target, so tapping boost fades rather
            // than pops, and holding it is at full level almost at once.
            boostGain += (boostTarget - boostGain) * 0.001f;
            boostNoise += (NextNoise() - boostNoise) * 0.14f;
            boostPhase += 2.0f * PI * 62.0f / SAMPLE_RATE;

            float value = (boostNoise * 2.0f + sinf(boostPhase) * 0.35f) * boostGain * 0.35f;
            out[i] = (short)(fmaxf(fminf(value, 1.0f), -1.0f) * 32000.0f);
        }
    }
}

void audio::Load()
{
    if (ready)
        return;

    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        // No device, for instance under the smoke test. Every entry point below
        // checks `ready`, so the game runs silent rather than not at all.
        TraceLog(LOG_WARNING, "AUDIO: device unavailable, running silent");
        return;
    }

    for (int i = 0; i < (int)AudioCue::Count; ++i)
    {
        cues[i].voices[0] = BuildCue(CUES[i]);
        for (int voice = 1; voice < VOICE_COUNT; ++voice)
            cues[i].voices[voice] = LoadSoundAlias(cues[i].voices[0]);
        cues[i].next = 0;
    }

    boostStream = LoadAudioStream(SAMPLE_RATE, 16, 1);
    SetAudioStreamCallback(boostStream, BoostCallback);
    // Left running for the whole session at a gain of zero: starting and
    // stopping it is what would be heard as a click.
    PlayAudioStream(boostStream);

    // The playlist is whatever the cooker copied, found by looking rather than
    // by a list in the code: adding a track is dropping a file in Assets/Sounds.
    // assets::List looks in the archive or in the folder, whichever is in use.
    trackNames = assets::List("Music", ".mp3");

    ready = true;
    TraceLog(LOG_INFO, "AUDIO: %i procedural cues ready, %i music track(s)",
             (int)AudioCue::Count, (int)trackNames.size());

    StartAnotherTrack();
}

void audio::Unload()
{
    if (!ready)
        return;

    if (trackPlaying)
        UnloadMusicStream(track);
    trackPlaying = false;
    assets::UnloadData(trackData);
    trackData = nullptr;
    trackNames.clear();

    StopAudioStream(boostStream);
    UnloadAudioStream(boostStream);

    for (int i = 0; i < (int)AudioCue::Count; ++i)
    {
        // The aliases share the first voice's data, so they go first and only
        // voice 0 actually owns anything to free.
        for (int voice = 1; voice < VOICE_COUNT; ++voice)
            UnloadSoundAlias(cues[i].voices[voice]);
        UnloadSound(cues[i].voices[0]);
    }

    CloseAudioDevice();
    ready = false;
}

void audio::SetVolumes(int masterPercent, int sfxPercent, int musicPercent)
{
    sfxVolume = sfxPercent / 100.0f;
    musicVolume = musicPercent / 100.0f;
    if (!ready)
        return;

    SetMasterVolume(masterPercent / 100.0f);
    SetAudioStreamVolume(boostStream, sfxVolume);
    if (trackPlaying)
        SetMusicVolume(track, musicVolume);
}

void audio::UpdateMusic()
{
    if (!ready || !trackPlaying)
        return;

    UpdateMusicStream(track);

    // With looping off, raylib stops the stream when the track runs out, so
    // "no longer playing" is the whole end-of-track test.
    if (!IsMusicStreamPlaying(track))
        StartAnotherTrack();
}

void audio::Play(AudioCue cue, float volume, float pitch)
{
    if (!ready)
        return;

    // Round robin, so the same cue firing twice in a row overlaps instead of
    // cutting itself short.
    CueSound &sound = cues[(int)cue];
    Sound &voice = sound.voices[sound.next];
    sound.next = (sound.next + 1) % VOICE_COUNT;

    SetSoundVolume(voice, volume * sfxVolume);
    SetSoundPitch(voice, pitch);
    PlaySound(voice);
}

void audio::SetBoost(bool active)
{
    boostTarget = active ? 1.0f : 0.0f;
}
