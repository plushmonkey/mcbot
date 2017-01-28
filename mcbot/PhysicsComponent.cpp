#include "PhysicsComponent.h"
#include <iostream>

#define TAU 3.14159 * 2
const char* PhysicsComponent::name = "Physics";

void PhysicsComponent::Integrate(double dt) {
    Vector3d horizontalAcceleration = m_Acceleration; horizontalAcceleration.y = 0;
    
    if (horizontalAcceleration.LengthSq() > m_MaxAcceleration * m_MaxAcceleration)
        horizontalAcceleration.Truncate(m_MaxAcceleration);

    // Add gravity
    m_Acceleration = Vector3d(horizontalAcceleration.x, -32 + m_Acceleration.y, horizontalAcceleration.z);

    if (m_Rotation > m_MaxRotation)
        m_Rotation = m_MaxRotation;

    Vector3d change = m_Velocity * dt;
    Vector3d hChange(change.x, 0, change.z);

    Collision collision;
    bool vertCollision = false;
    bool horizCollision = false;
    float tau = (float)TAU;
    for (float angle = 0.0f; angle < tau; angle += tau / 8.0f) {
        Vector3d checkPos = m_Position + Vector3d(std::cos(angle), 0, std::sin(angle)) * 0.3;

        if (!vertCollision && m_CollisionDetector.DetectCollision(checkPos, Vector3d(0, change.y, 0), &collision)) {
            m_Position.y = collision.GetPosition().y;
            m_Velocity.y = 0;
            vertCollision = true;
        }

        if (!horizCollision && m_CollisionDetector.DetectCollision(checkPos, hChange, &collision)) {
            m_Position -= hChange;
            horizCollision = true;
        }
    }
    if (!vertCollision) {
        m_Position.y += change.y;
    }
    if (!horizCollision) {
        m_Position += hChange;
    }

    m_Velocity += m_Acceleration * dt;
    m_Orientation += m_Rotation * dt;

    double epsilon = 0.0001;
    if (m_Velocity.LengthSq() < epsilon) m_Velocity = Vector3d(0, 0, 0);

    if (m_Orientation > TAU)
        m_Orientation -= TAU;
    else if (m_Orientation < -TAU)
        m_Orientation += TAU;

    Vector3d horizontalVelocity(m_Velocity.x, 0, m_Velocity.z);
    horizontalVelocity.Truncate(m_MaxSpeed);
    m_Velocity = horizontalVelocity + Vector3d(0, m_Velocity.y, 0);
    m_Acceleration = Vector3d(0, 0, 0);
    m_Rotation = 0;
}
