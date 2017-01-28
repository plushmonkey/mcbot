#ifndef MCBOT_PHYSICS_COMPONENT_H_
#define MCBOT_PHYSICS_COMPONENT_H_

#include <mclib/Vector.h>
#include "Component.h"
#include "Collision.h"

class PhysicsComponent : public Component {
public:
    static const char* name;

private:
    CollisionDetector m_CollisionDetector;

    Vector3d m_Position;
    Vector3d m_Velocity;
    double m_Orientation;

    Vector3d m_Acceleration;
    double m_Rotation;

    double m_MaxAcceleration;
    double m_MaxRotation;
    double m_MaxSpeed;

public:
    PhysicsComponent(Minecraft::World* world) 
        : m_CollisionDetector(world), m_Orientation(0), m_MaxSpeed(1), m_MaxAcceleration(1), m_MaxRotation(3.14 * 2)
    {

    }

    const Vector3d& GetPosition() const { return m_Position; }
    const Vector3d& GetVelocity() const { return m_Velocity; }
    double GetOrientation() const { return m_Orientation; }
    const Vector3d& GetAcceleration() const { return m_Acceleration; }
    double GetRotation() const { return m_Rotation; }
    double GetMaxSpeed() const { return m_MaxSpeed; }
    double GetMaxAcceleration() const { return m_MaxAcceleration; }
    double GetMaxRotation() const { return m_MaxRotation; }
    
    void SetPosition(const Vector3d& pos) { m_Position = pos; }
    void SetVelocity(const Vector3d& velocity) { m_Velocity = velocity; }
    void ClearHorizontalVelocity() { m_Velocity = Vector3d(0, m_Velocity.y, 0); }
    void SetOrientation(double orientation) { m_Orientation = orientation; }
    void SetMaxSpeed(double maxSpeed) { m_MaxSpeed = maxSpeed; }
    void SetMaxAcceleration(double maxAccel) { m_MaxAcceleration = maxAccel; }
    void SetMaxRotation(double maxAccel) { m_MaxRotation = maxAccel; }

    void ApplyAcceleration(Vector3d acceleration) {
        m_Acceleration += acceleration;
    }

    void ApplyRotation(double rotation) {
        m_Rotation += rotation;
    }

    void Integrate(double dt);
    void Update(double dt) { }

    const char* GetName() const { return name; }
};

#endif
