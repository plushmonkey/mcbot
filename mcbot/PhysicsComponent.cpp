#include "PhysicsComponent.h"
#include <iostream>

#define TAU 3.14159 * 2
const char* PhysicsComponent::name = "Physics";

void PhysicsComponent::Update(double dt) {
    if (m_Acceleration.LengthSq() > m_MaxAcceleration * m_MaxAcceleration)
        m_Acceleration.Truncate(m_MaxAcceleration);

    if (m_Rotation > m_MaxRotation)
        m_Rotation = m_MaxRotation;

    m_Position += m_Velocity * dt;
    m_Velocity += m_Acceleration * dt;
    m_Orientation += m_Rotation * dt;

    double epsilon = 0.0001;
    if (m_Velocity.LengthSq() < epsilon) m_Velocity = Vector3d(0, 0, 0);

    if (m_Orientation > TAU)
        m_Orientation -= TAU;
    else if (m_Orientation < -TAU)
        m_Orientation += TAU;

    m_Velocity.Truncate(m_MaxSpeed);
    m_Acceleration = Vector3d(0, 0, 0);
    m_Rotation = 0;
}
