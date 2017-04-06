#ifndef MCBOT_COLLISION_H_
#define MCBOT_COLLISION_H_

#include <mclib/world/World.h>

class PhysicsComponent;

class Collision {
private:
    mc::Vector3d m_Position;
    mc::Vector3d m_Normal;

public:
    Collision() { }
    Collision(mc::Vector3d position, mc::Vector3d normal) : m_Position(position), m_Normal(normal) { }

    mc::Vector3d GetPosition() const { return m_Position; }
    mc::Vector3d GetNormal() const { return m_Normal; }
};

class CollisionDetector {
private:
    mc::world::World* m_World;

    std::vector<mc::Vector3i> GetSurroundingLocations(mc::AABB bounds);

public:
    CollisionDetector(mc::world::World* world) : m_World(world) { }

    bool DetectCollision(mc::Vector3d from, mc::Vector3d rayVector, Collision* collision) const;

    void ResolveCollisions(PhysicsComponent* physics, double dt);
};

#endif
