#ifndef MATCH_H
#define MATCH_H

#include "GameObjects/BallObject.h"
#include "GameObjects/CarObject.h"
#include "GameObjects/GoalObject.h"

#include <vector>

// Score, clock and the kickoff/goal/full time state machine, plus the reset that
// re-centres the ball and the cars.
enum class MatchState
{
    Kickoff,     // counting down, nothing may move
    Playing,
    Celebration, // a goal just went in, the ball is still live
    Finished
};

struct Match
{
    void Begin(BallObject &ballObject, GoalObject &blue, GoalObject &orange, float durationMinutes);
    void AddCar(CarObject &car);

    void Update(float deltaTime);
    // The kickoff freeze: physics must not run while this is true.
    bool IsFrozen() const { return state == MatchState::Kickoff || state == MatchState::Finished; }

    void ResetField();

    MatchState state = MatchState::Kickoff;
    int scoreBlue = 0;
    int scoreOrange = 0;
    float timeRemaining = 300.0f;
    float stateTimer = 0.0f;  // counts down inside Kickoff and Celebration
    int lastScoringTeam = -1; // 0 blue, 1 orange

    float kickoffCountdown = 3.0f;
    float celebrationTime = 2.5f;

    BallObject *ball = nullptr;
    GoalObject *blueGoal = nullptr;
    GoalObject *orangeGoal = nullptr;
    std::vector<CarObject *> cars;
};

#endif
