#include "PhysicsLayers.h"

namespace physics
{
    bool LayerPairFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const
    {
        switch (inLayer1)
        {
        case Arena:   return inLayer2 == Car || inLayer2 == Ball;
        case Car:     return true;
        case Ball:    return true;
        case Trigger: return inLayer2 == Car || inLayer2 == Ball;
        default:      return false;
        }
    }

    JPH::BroadPhaseLayer BroadPhaseMap::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
    {
        return (inLayer == Car || inLayer == Ball) ? MovingPhase : StaticPhase;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char *BroadPhaseMap::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
    {
        return inLayer == MovingPhase ? "Moving" : "Static";
    }
#endif

    bool ObjectVsBroadPhaseFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
    {
        // Static things only need to be tested against the moving tree.
        if (inLayer1 == Arena || inLayer1 == Trigger)
            return inLayer2 == MovingPhase;

        return true;
    }
}
