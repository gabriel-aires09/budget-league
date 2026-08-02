#include "Match.h"

void Match::Begin(BallObject &ballObject, GoalObject &blue, GoalObject &orange, float durationMinutes)
{
    ball = &ballObject;
    blueGoal = &blue;
    orangeGoal = &orange;

    scoreBlue = 0;
    scoreOrange = 0;
    timeRemaining = durationMinutes * 60.0f;
    lastScoringTeam = -1;

    ResetField();
}

void Match::AddCar(CarObject &car)
{
    cars.push_back(&car);
}

void Match::ResetField()
{
    if (ball != nullptr)
        ball->ResetTo(ball->spawnPosition);

    for (CarObject *car : cars)
        car->ResetTo(car->spawnPosition, car->spawnYawDegrees);

    state = MatchState::Kickoff;
    stateTimer = kickoffCountdown;
}

void Match::Update(float deltaTime)
{
    switch (state)
    {
    case MatchState::Kickoff:
        stateTimer -= deltaTime;
        if (stateTimer <= 0.0f)
            state = MatchState::Playing;
        break;

    case MatchState::Playing:
    {
        timeRemaining -= deltaTime;
        if (timeRemaining <= 0.0f)
        {
            timeRemaining = 0.0f;
            state = MatchState::Finished;
            break;
        }

        // A goal belongs to the team that does not defend that net.
        Vector3 ballCenter = ball->GetBodyPosition();
        const GoalObject *scored = nullptr;
        if (blueGoal->IsBallFullyInside(ballCenter, ball->radius))
            scored = blueGoal;
        else if (orangeGoal->IsBallFullyInside(ballCenter, ball->radius))
            scored = orangeGoal;

        if (scored != nullptr)
        {
            lastScoringTeam = scored->defendingTeam == 0 ? 1 : 0;
            if (lastScoringTeam == 0)
                ++scoreBlue;
            else
                ++scoreOrange;

            state = MatchState::Celebration;
            stateTimer = celebrationTime;
        }
        break;
    }

    case MatchState::Celebration:
        // The clock keeps running through the celebration, as it does in Rocket
        // League, but the ball stays live so the goal is visible.
        timeRemaining -= deltaTime;
        stateTimer -= deltaTime;
        if (stateTimer <= 0.0f)
        {
            if (timeRemaining <= 0.0f)
            {
                timeRemaining = 0.0f;
                state = MatchState::Finished;
            }
            else
            {
                ResetField();
            }
        }
        break;

    case MatchState::Finished:
        break;
    }
}
