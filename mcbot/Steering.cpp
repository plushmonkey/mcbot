#include "Steering.h"
#include "PhysicsComponent.h"
#include "JumpComponent.h"
#include "Utility.h"
#include "Pathing.h"
#include "Collision.h"
#include "Math.h"

#include <iostream>
#include <random>

namespace ai {

namespace {

std::random_device dev;
std::mt19937 engine(dev());

} // ns

SteeringAcceleration SeekSteering::GetSteering() {
    SteeringAcceleration steering;

    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return steering;

    steering.movement = Vector3Normalize(m_Target - physics->GetPosition()) * physics->GetMaxAcceleration();
    steering.movement.y = 0;
    steering.rotation = 0;

    return steering;
}

SteeringAcceleration FleeSteering::GetSteering() {
    SteeringAcceleration steering;

    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return steering;

    steering.movement = Vector3Normalize(physics->GetPosition() - m_Target) * physics->GetMaxAcceleration();
    steering.movement.y = 0;
    steering.rotation = 0;

    return steering;
}

SteeringAcceleration ArriveSteering::GetSteering() {
    SteeringAcceleration steering;

    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return steering;

    Vector3d direction = m_Target - physics->GetPosition();
    double distance = direction.Length();

    if (distance < m_TargetRadius) {
        steering.movement = -physics->GetVelocity();
        return steering;
    }

    double targetSpeed;
    if (distance > m_SlowRadius)
        targetSpeed = physics->GetMaxSpeed();
    else
        targetSpeed = physics->GetMaxSpeed() * distance / m_SlowRadius;

    Vector3d targetVelocity = Vector3Normalize(direction) * targetSpeed;

    steering.movement = targetVelocity - physics->GetVelocity();
    steering.movement /= m_TimeToTarget;

    return steering;
}

SteeringAcceleration AlignSteering::GetSteering() {
    SteeringAcceleration steering;

    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return steering;

    double rotation = m_TargetOrientation - physics->GetOrientation();
    rotation = wrapToPi(rotation);

    double maxRotation = physics->GetMaxRotation();
    double rotationSize = std::abs(rotation);
    
    if (rotationSize <= m_TargetRadius)
        return steering;

    double targetRotation;
    if (rotationSize > m_SlowRadius)
        targetRotation = maxRotation;
    else
        targetRotation = maxRotation * rotationSize / m_SlowRadius;

    targetRotation *= rotation / rotationSize;
    
    steering.rotation = (targetRotation - physics->GetRotation()) / m_TimeToTarget;

    return steering;
}

SteeringAcceleration PursueSteering::GetSteering() {
    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return SteeringAcceleration();

    Vector3d direction = m_PursueTarget - physics->GetPosition();
    double dist = direction.Length();
    double speed = physics->GetVelocity().Length();

    double prediction;
    if (speed <= dist / m_MaxPrediction)
        prediction = m_MaxPrediction;
    else
        prediction = dist / speed;

    m_Target = m_PursueTarget + m_TargetVelocity * prediction;

    return SeekSteering::GetSteering();
}

SteeringAcceleration FaceSteering::GetSteering() {
    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return SteeringAcceleration();

    Vector3d direction = m_Target - physics->GetPosition();
    if (direction.LengthSq() == 0) return SteeringAcceleration();

    m_TargetOrientation = std::atan2(-direction.x, direction.z);

    return AlignSteering::GetSteering();
}

SteeringAcceleration LookSteering::GetSteering() {
    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return SteeringAcceleration();
    Vector3d velocity = physics->GetVelocity();
    if (velocity.LengthSq() == 0) return SteeringAcceleration();

    m_TargetOrientation = std::atan2(-velocity.x, velocity.z);

    return AlignSteering::GetSteering();
}

SteeringAcceleration WanderSteering::GetSteering() {
    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return SteeringAcceleration();

    m_WanderOrientation += randomBinomial(engine) * m_WanderRate;
    m_TargetOrientation = m_WanderOrientation + physics->GetOrientation();

    m_Target = physics->GetPosition() + util::OrientationToVector(physics->GetOrientation()) * m_WanderOffset;
    m_Target += util::OrientationToVector(m_TargetOrientation) * m_WanderRadius;

    SteeringAcceleration steering = FaceSteering::GetSteering();

    steering.movement = util::OrientationToVector(physics->GetOrientation()) * physics->GetMaxAcceleration();

    return steering;
}

double GetPathLength(path::Plan* plan) {
    std::size_t size = plan->GetSize();

    if (size == 0) return 0;
    double total = 0;
    for (std::size_t i = 0; i < plan->GetSize() - 1; ++i) {
        Vector3i current = (*plan)[i]->GetPosition();
        Vector3i next = (*plan)[i + 1]->GetPosition();

        total += (next - current).Length();
    }

    return total;
}

SteeringAcceleration PathFollowSteering::GetSteering() {
    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return SteeringAcceleration();

    // todo: find closest position on continuous path and follow that instead

    double TargetRadiusSq = 0.5 * 0.5;

    if (!m_Plan) return SteeringAcceleration();

    path::Node* current = m_Plan->GetCurrent();

    if (current && current->GetPosition() == ToVector3i(physics->GetPosition())) {
        current = m_Plan->Next();
    }

    if (!current) return SteeringAcceleration();

    m_Target = ToVector3d(current->GetPosition()) + Vector3d(0.5, 0, 0.5);

    Vector3d pos = physics->GetPosition();
    Vector3i tar = current->GetPosition();

    if (pos.y < tar.y) {
        auto jump = GetActorComponent(m_Player, JumpComponent);
        if (jump) {
            jump->Jump();
        }
    }

    return SeekSteering::GetSteering();
}

SteeringAcceleration ObstacleAvoidSteering::GetSteering() {
    auto physics = GetActorComponent(m_Player, PhysicsComponent);
    if (physics == nullptr) return SteeringAcceleration();

    std::vector<Vector3d> rayVectors(3);

    /*Vector3d velocity = m_SmoothedVelocity;
    if (velocity.LengthSq() == 0)*/

    //Vector3d direction = physics->GetVelocity();

    Vector3d direction = util::OrientationToVector(physics->GetOrientation());
    rayVectors.push_back(Vector3Normalize(direction) * m_RayLength);
    rayVectors.push_back(Vector3RotateAboutY(rayVectors[0], 30.0 * M_PI / 180.0) * 0.75);
    rayVectors.push_back(Vector3RotateAboutY(rayVectors[0], -30.0 * M_PI / 180.0) * 0.75);

    Collision collision;
    bool collided = false;

    for (Vector3d rayVector : rayVectors) {
        rayVector.y = 0;
        if (m_CollisionDetector->DetectCollision(physics->GetPosition() + Vector3d(0, 0.25, 0), rayVector, &collision)) {
            collided = true;
            break;
        }
    }

    if (!collided)
        return SteeringAcceleration();

    m_Target = collision.GetPosition() + collision.GetNormal() * m_AvoidDistance;

    return SeekSteering::GetSteering();
}

} // ns ai
