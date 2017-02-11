#include "PhysicsComponent.h"
#include "JumpComponent.h"
#include "../Math.h"
#include "../Actor.h"

#include <iostream>
#include <random>

namespace {

std::random_device dev;
std::mt19937 engine(dev());
std::uniform_real_distribution<double> dist(0, 1);
const Vector3d Up(0, 1, 0);

}

const char* PhysicsComponent::name = "Physics";

void PhysicsComponent::Integrate(double dt) {
    Vector3d horizontalAcceleration = m_Acceleration; horizontalAcceleration.y = 0;

    if (horizontalAcceleration.LengthSq() > m_MaxAcceleration * m_MaxAcceleration)
        horizontalAcceleration.Truncate(m_MaxAcceleration);

    bool onGround = m_CollisionDetector.DetectCollision(m_Position, Vector3d(0, -32 * dt, 0), nullptr);
    if (!onGround) {
        // Move slower in the air
        horizontalAcceleration *= .3;
    }

    // Add gravity
    m_Acceleration = Vector3d(horizontalAcceleration.x, -32 + m_Acceleration.y, horizontalAcceleration.z);

    if (m_Rotation > m_MaxRotation)
        m_Rotation = m_MaxRotation;

    m_CollisionDetector.ResolveCollisions(this, dt);

    const double epsilon = 0.0001;
    m_Velocity += m_Acceleration * dt;
    m_Orientation += m_Rotation * dt;
    
    if (m_Velocity.LengthSq() < epsilon) m_Velocity = Vector3d(0, 0, 0);
    m_Velocity *= 0.97;

    if (m_Orientation > M_TAU)
        m_Orientation -= M_TAU;
    else if (m_Orientation < -M_TAU)
        m_Orientation += M_TAU;

    Vector3d horizontalVelocity(m_Velocity.x, 0, m_Velocity.z);
    horizontalVelocity.Truncate(m_MaxSpeed);
    m_Velocity = horizontalVelocity + Vector3d(0, m_Velocity.y, 0);
    m_Acceleration = Vector3d(0, 0, 0);
    m_Rotation = 0;
}
