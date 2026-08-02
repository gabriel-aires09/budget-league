#ifndef PHYSICSLAYERS_H
#define PHYSICSLAYERS_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace physics
{
    // Object layers.
    static constexpr JPH::ObjectLayer Arena = 0;   // static world geometry
    static constexpr JPH::ObjectLayer Car = 1;
    static constexpr JPH::ObjectLayer Ball = 2;
    static constexpr JPH::ObjectLayer Trigger = 3; // non solid: goals and boost pads
    static constexpr JPH::ObjectLayer LayerCount = 4;

    // Broad phase: one tree for what never moves, one for what does.
    static constexpr JPH::BroadPhaseLayer StaticPhase(0);
    static constexpr JPH::BroadPhaseLayer MovingPhase(1);
    static constexpr JPH::uint BroadPhaseCount = 2;

    class LayerPairFilter final : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override;
    };

    class BroadPhaseMap final : public JPH::BroadPhaseLayerInterface
    {
    public:
        virtual JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseCount; }
        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif
    };

    class ObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
    };

    // The three filters a PhysicsSystem needs; they are stateless but must outlive it.
    struct LayerFilters
    {
        LayerPairFilter objectPair;
        BroadPhaseMap broadPhase;
        ObjectVsBroadPhaseFilter objectVsBroadPhase;
    };
}

#endif
