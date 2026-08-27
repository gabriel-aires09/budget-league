#ifndef CARSELECTSCENE_H
#define CARSELECTSCENE_H

#include "Scene.h"
#include "StaticModelAsset.h"
#include "UserInterface.h"

// The car picker, shown between the main menu and the match (Milestone 08).
// Six of the seven cooked cars stand on a 2 x 3 grid of pedestals; the pick is
// written into GameSettings, which is what MatchScene reads.
//
// No physics and no GameObjects: the previews are drawn straight from their
// cooked models, so this scene never calls InitializePhysics.
class CarSelectScene final : public Scene
{
public:
    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual void Shutdown() override;

    static const int CAR_COUNT = 6;
    static const int COLUMNS = 3;

    StaticModelAsset previews[CAR_COUNT];
    // Fitted per car so every preview is the same length on the grid, whatever
    // the pack modelled it at.
    float previewScale[CAR_COUNT] = {};

    int selected = 0;
    float spinDegrees = 0.0f;
};

#endif
