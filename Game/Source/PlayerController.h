#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include "CarController.h"

class PlayerController final : public CarController
{
public:
    virtual CarInput Poll() override;
};

#endif
