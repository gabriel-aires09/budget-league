#include "TuningPanel.h"

#ifdef GAME_DEV_TOOLS

#include "MatchScene.h"

#include <raylib.h>

#include <imgui.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// Next to the executable, not in assets/: it is written by the game, not cooked.
static std::string ConfigPath()
{
    return std::string(GetApplicationDirectory()) + "Tuning.cfg";
}

// Pushes the values that are not simply read every step back into the systems
// that cached them. Called once after a slider moves, not every frame.
static void Apply(MatchScene &scene, const TuningPanel &panel)
{
    scene.physicsSystem.SetGravity(JPH::Vec3(0.0f, panel.gravity, 0.0f));
    scene.ball.ApplyTuning();
    scene.playerCar.ApplyTuning();
    if (scene.botActive)
        scene.botCar.ApplyTuning();

    for (BoostPadObject &pad : scene.boostPads)
        pad.refillAmount = pad.fullRefill ? panel.bigPadRefill : panel.smallPadRefill;
}

void TuningPanel::Initialize(MatchScene &scene)
{
    CarObject &car = scene.playerCar;
    CarObject &bot = scene.botCar;
    BallObject &ball = scene.ball;
    ChaseCamera &camera = scene.chaseCamera;

    gravity = scene.physicsSystem.GetGravity().GetY();

    entries.clear();
    entries.push_back({ "World", "gravity", &gravity, nullptr, -30.0f, -1.0f });

    entries.push_back({ "Ball", "gravityFactor", &ball.gravityFactor, nullptr, 0.5f, 3.0f });
    entries.push_back({ "Ball", "restitution", &ball.restitution, nullptr, 0.0f, 1.0f });
    entries.push_back({ "Ball", "friction", &ball.friction, nullptr, 0.0f, 1.0f });
    entries.push_back({ "Ball", "angularDamping", &ball.angularDamping, nullptr, 0.0f, 4.0f });

    // Every car entry carries the bot's matching field as its mirror.
    entries.push_back({ "Car drive", "engineForce", &car.engineForce, &bot.engineForce, 1000.0f, 12000.0f });
    entries.push_back({ "Car drive", "brakeForce", &car.brakeForce, &bot.brakeForce, 1000.0f, 15000.0f });
    entries.push_back({ "Car drive", "maxSpeed", &car.maxSpeed, &bot.maxSpeed, 10.0f, 60.0f });
    entries.push_back({ "Car drive", "steerRate", &car.steerRate, &bot.steerRate, 0.5f, 8.0f });
    entries.push_back({ "Car drive", "highSpeedSteerScale", &car.highSpeedSteerScale, &bot.highSpeedSteerScale, 0.1f, 1.0f });
    entries.push_back({ "Car drive", "grip", &car.grip, &bot.grip, 0.0f, 30.0f });

    entries.push_back({ "Car air", "jumpImpulse", &car.jumpImpulse, &bot.jumpImpulse, 200.0f, 2500.0f });
    entries.push_back({ "Car air", "secondJumpImpulse", &car.secondJumpImpulse, &bot.secondJumpImpulse, 200.0f, 2500.0f });
    entries.push_back({ "Car air", "flipImpulse", &car.flipImpulse, &bot.flipImpulse, 200.0f, 3500.0f });
    entries.push_back({ "Car air", "flipSpin", &car.flipSpin, &bot.flipSpin, 2.0f, 20.0f });
    entries.push_back({ "Car air", "flipDuration", &car.flipDuration, &bot.flipDuration, 0.1f, 1.5f });
    entries.push_back({ "Car air", "flipHitImpulse", &car.flipHitImpulse, &bot.flipHitImpulse, 0.0f, 1500.0f });
    entries.push_back({ "Car air", "airControlRate", &car.airControlRate, &bot.airControlRate, 1.0f, 15.0f });
    entries.push_back({ "Car air", "uprightTorque", &car.uprightTorque, &bot.uprightTorque, 1000.0f, 15000.0f });

    entries.push_back({ "Boost", "drainRate", &car.boostDrainRate, &bot.boostDrainRate, 5.0f, 100.0f });
    entries.push_back({ "Boost", "boostForce", &car.boostForce, &bot.boostForce, 1000.0f, 15000.0f });
    entries.push_back({ "Boost", "boostMaxSpeed", &car.boostMaxSpeed, &bot.boostMaxSpeed, 20.0f, 80.0f });
    entries.push_back({ "Boost", "smallPadRefill", &smallPadRefill, nullptr, 1.0f, 100.0f });
    entries.push_back({ "Boost", "bigPadRefill", &bigPadRefill, nullptr, 1.0f, 100.0f });

    entries.push_back({ "Camera", "positionSmoothing", &camera.positionSmoothing, nullptr, 1.0f, 25.0f });
    entries.push_back({ "Camera", "targetSmoothing", &camera.targetSmoothing, nullptr, 1.0f, 30.0f });
    entries.push_back({ "Camera", "distance", &camera.distance, nullptr, 3.0f, 20.0f });
    entries.push_back({ "Camera", "height", &camera.height, nullptr, 0.5f, 12.0f });

    // The saved config, on top of the defaults the objects were built with.
    std::string path = ConfigPath();
    FILE *file = fopen(path.c_str(), "r");
    if (file == nullptr)
        return;

    char line[128];
    while (fgets(line, sizeof(line), file) != nullptr)
    {
        // Split at the first '=' rather than scanning a token: section names
        // contain spaces ("Car drive"), so any format that stops at whitespace
        // silently reads half a key and drops the line.
        char *separator = strchr(line, '=');
        if (line[0] == '#' || separator == nullptr)
            continue;

        *separator = '\0';
        for (char *end = separator - 1; end >= line && (*end == ' ' || *end == '\t'); --end)
            *end = '\0';

        float value = (float)atof(separator + 1);
        for (Entry &entry : entries)
        {
            if (std::string(entry.section) + "." + entry.name != line)
                continue;

            *entry.value = value;
            if (entry.mirror != nullptr)
                *entry.mirror = value;
            break;
        }
    }
    fclose(file);

    Apply(scene, *this);
    TraceLog(LOG_INFO, "TUNING: loaded %s", path.c_str());
}

bool TuningPanel::Save() const
{
    std::string path = ConfigPath();
    FILE *file = fopen(path.c_str(), "w");
    if (file == nullptr)
    {
        TraceLog(LOG_WARNING, "TUNING: could not write %s", path.c_str());
        return false;
    }

    fprintf(file, "# Written by the tuning panel (F1). Loaded when a match starts.\n");
    for (const Entry &entry : entries)
        fprintf(file, "%s.%s = %g\n", entry.section, entry.name, *entry.value);
    fclose(file);

    TraceLog(LOG_INFO, "TUNING: saved %s", path.c_str());
    return true;
}

void TuningPanel::Draw(MatchScene &scene)
{
    if (!open)
        return;

    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(440.0f, 560.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Tuning (F1)", &open))
    {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("The match is frozen while this is open.");
    ImGui::TextUnformatted("Ctrl+click a slider to type a value.");
    // Above the sliders, so it is reachable without scrolling the list.
    if (ImGui::Button("Save"))
        Save();
    ImGui::SameLine();
    ImGui::TextUnformatted(ConfigPath().c_str());
    ImGui::Separator();

    // Leave room for the longest label ("highSpeedSteerScale"), which the
    // default half-and-half split cuts off.
    ImGui::PushItemWidth(-170.0f);

    bool changed = false;
    const char *section = nullptr;
    bool sectionOpen = false;
    for (Entry &entry : entries)
    {
        if (section == nullptr || strcmp(section, entry.section) != 0)
        {
            section = entry.section;
            sectionOpen = ImGui::CollapsingHeader(section, ImGuiTreeNodeFlags_DefaultOpen);
        }
        if (!sectionOpen)
            continue;

        // Forces and speeds are whole numbers; the damping and blend values are
        // not, and would read as a column of zeroes at the same precision.
        const char *format = entry.maximum >= 100.0f ? "%.0f" : "%.3f";
        if (ImGui::SliderFloat(entry.name, entry.value, entry.minimum, entry.maximum, format))
        {
            if (entry.mirror != nullptr)
                *entry.mirror = *entry.value;
            changed = true;
        }
    }

    ImGui::PopItemWidth();
    ImGui::End();

    if (changed)
        Apply(scene, *this);
}

#endif
