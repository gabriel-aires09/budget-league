#ifndef HOWTOPLAYSCENE_H
#define HOWTOPLAYSCENE_H

#include "Scene.h"
#include "UserInterface.h"

// The how-to-play screen, reached from the main menu. Two columns of panels:
// what the game is on the left, what the arena and the match do on the right.
//
// No physics and no GameObjects; the text is the whole scene.
class HowToPlayScene final : public Scene
{
public:
    virtual void Initialize() override;
    virtual void Update(float deltaTime) override;
    virtual void Draw() override;
    virtual void Shutdown() override;
};

#endif
