#ifndef MCBOT_STEERING_H_
#define MCBOT_STEERING_H_

#include <mclib/Vector.h>
#include <mclib/Utility.h>
#include <mclib/Entity/Entity.h>
#include "Actor.h"

class CollisionDetector;

namespace ai {

struct SteeringAcceleration {
    Vector3d movement;
    double rotation;

    SteeringAcceleration() : rotation(0) { }
};

class SteeringBehavior {
public:
    virtual SteeringAcceleration GetSteering() = 0;
};

class SeekSteering : public SteeringBehavior {
protected:
    Actor* m_Player;
    Vector3d m_Target;

public:
    SeekSteering(Actor* player, Vector3d target) 
        : m_Player(player), m_Target(target)
    {
    }

    SteeringAcceleration GetSteering();
};

class FleeSteering : public SteeringBehavior {
protected:
    Actor* m_Player;
    Vector3d m_Target;

public:
    FleeSteering(Actor* player, Vector3d target)
        : m_Player(player), m_Target(target)
    {
    }

    SteeringAcceleration GetSteering();
};

class ArriveSteering : public SteeringBehavior {
protected:
    Actor* m_Player;
    Vector3d m_Target;

    // Consider the player to have arrived if within this radius
    double m_TargetRadius;
    // Start slowing down once inside this radius
    double m_SlowRadius;

    double m_TimeToTarget;

public:
    ArriveSteering(Actor* player, Vector3d target, double targetRadius, double slowRadius, double timeToTarget)
        : m_Player(player), m_Target(target), m_TargetRadius(targetRadius), m_SlowRadius(slowRadius), m_TimeToTarget(timeToTarget)
    {

    }

    SteeringAcceleration GetSteering();
};

class AlignSteering : public SteeringBehavior {
protected:
    Actor* m_Player;
    double m_TargetOrientation;

    double m_TargetRadius;
    double m_SlowRadius;
    double m_TimeToTarget;

public:
    AlignSteering(Actor* player, double targetOrientation, double targetRadius, double slowRadius, double timeToTarget)
        : m_Player(player), m_TargetOrientation(targetOrientation), m_TargetRadius(targetRadius), m_SlowRadius(slowRadius), m_TimeToTarget(timeToTarget)
    {
        
    }

    SteeringAcceleration GetSteering();
};

class PursueSteering : public SeekSteering {
private:
    double m_MaxPrediction;
    Vector3d m_PursueTarget;
    Vector3d m_TargetVelocity;

public:
    PursueSteering(Actor* player, double maxPrediction, Vector3d target, Vector3d targetVelocity)
        : SeekSteering(player, target),
          m_MaxPrediction(maxPrediction), m_PursueTarget(target), m_TargetVelocity(targetVelocity)
    {

    }

    SteeringAcceleration GetSteering();
};

class FaceSteering : public AlignSteering {
protected:
    Vector3d m_Target;

public:
    FaceSteering(Actor* player, Vector3d target, double targetRadius, double slowRadius, double timeToTarget)
        : AlignSteering(player, 0, targetRadius, slowRadius, timeToTarget), m_Target(target) { }

    SteeringAcceleration GetSteering();
};

class LookSteering : public AlignSteering {
public:
    LookSteering(Actor* player, double targetRadius, double slowRadius, double timeToTarget)
        : AlignSteering(player, 0, targetRadius, slowRadius, timeToTarget) { }

    SteeringAcceleration GetSteering();
};

class WanderSteering : public FaceSteering {
private:
    double m_WanderOffset;
    double m_WanderRadius;
    double m_WanderRate;
    double m_WanderOrientation;

public:
    WanderSteering(Actor* player, double offset, double radius, double rate)
        : FaceSteering(player, Vector3d(), 0.1, 1, 1),
          m_WanderOffset(offset), m_WanderRadius(radius), m_WanderRate(rate), m_WanderOrientation(0)
    {

    }

    SteeringAcceleration GetSteering();
};

namespace path {
class Plan;
}

class PathFollowSteering : public SeekSteering {
private:
    path::Plan* m_Plan;
    double m_PathOffset;
    double m_PlanPos;

public:
    PathFollowSteering(Actor* player, path::Plan* plan, double offset)
        : SeekSteering(player, Vector3d()),
        m_Plan(plan), m_PathOffset(offset), m_PlanPos(0)
    {

    }

    SteeringAcceleration GetSteering();
};



class ObstacleAvoidSteering : public SeekSteering {
private:
    CollisionDetector* m_CollisionDetector;
    double m_AvoidDistance;
    double m_RayLength;
    Vector3d m_SmoothedVelocity;

public:
    ObstacleAvoidSteering(Actor* player, Vector3d smoothedVelocity, CollisionDetector* detector, double avoidDistance, double lookAhead)
        : SeekSteering(player, Vector3d()),
          m_CollisionDetector(detector), m_AvoidDistance(avoidDistance), m_RayLength(lookAhead)
    {

    }

    SteeringAcceleration GetSteering();
};

} // ns ai

#endif
