#ifndef MCBOT_PHYSICS_COMPONENT_H_
#define MCBOT_PHYSICS_COMPONENT_H_

#include <mclib/common/Vector.h>
#include <mclib/common/AABB.h>
#include "../Component.h"
#include "../Collision.h"

class PhysicsComponent : public Component {
public:
    static const char* name;

private:
    CollisionDetector m_CollisionDetector;
    mc::AABB m_BoundingBox;

    mc::Vector3d m_Position;
    mc::Vector3d m_Velocity;
    double m_Orientation;

    mc::Vector3d m_Acceleration;
    double m_Rotation;

    double m_MaxAcceleration;
    double m_MaxRotation;
    double m_MaxSpeed;

public:
    PhysicsComponent(mc::world::World* world, mc::AABB bounds)
        : m_CollisionDetector(world), 
          m_BoundingBox(bounds),
          m_Position(), m_Velocity(),
          m_Orientation(0), 
          m_Acceleration(), m_Rotation(0),
          m_MaxAcceleration(1), m_MaxRotation(3.14 * 2), m_MaxSpeed(1)
    {

    }

    const mc::Vector3d& GetPosition() const { return m_Position; }
    const mc::Vector3d& GetVelocity() const { return m_Velocity; }
    double GetOrientation() const { return m_Orientation; }
    const mc::Vector3d& GetAcceleration() const { return m_Acceleration; }
    double GetRotation() const { return m_Rotation; }
    double GetMaxSpeed() const { return m_MaxSpeed; }
    double GetMaxAcceleration() const { return m_MaxAcceleration; }
    double GetMaxRotation() const { return m_MaxRotation; }
    mc::AABB GetBoundingBox() const { return m_BoundingBox; }

    void SetPosition(const mc::Vector3d& pos) { m_Position = pos; }
    void SetVelocity(const mc::Vector3d& velocity) { m_Velocity = velocity; }
    void ClearHorizontalVelocity() { m_Velocity = mc::Vector3d(0, m_Velocity.y, 0); }
    void SetOrientation(double orientation) { m_Orientation = orientation; }
    void SetMaxSpeed(double maxSpeed) { m_MaxSpeed = maxSpeed; }
    void SetMaxAcceleration(double maxAccel) { m_MaxAcceleration = maxAccel; }
    void SetMaxRotation(double maxAccel) { m_MaxRotation = maxAccel; }

    void ApplyAcceleration(mc::Vector3d acceleration) {
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
