#ifndef MCBOT_COLLISION_H_
#define MCBOT_COLLISION_H_

#include <mclib/World.h>

class PhysicsComponent;

class Collision {
private:
    Vector3d m_Position;
    Vector3d m_Normal;

public:
    Collision() { }
    Collision(Vector3d position, Vector3d normal) : m_Position(position), m_Normal(normal) { }

    Vector3d GetPosition() const { return m_Position; }
    Vector3d GetNormal() const { return m_Normal; }
};

class CollisionDetector {
private:
    Minecraft::World* m_World;

    std::vector<Vector3i> GetSurroundingLocations(AABB bounds);

public:
    CollisionDetector(Minecraft::World* world) : m_World(world) { }

    bool DetectCollision(Vector3d from, Vector3d rayVector, Collision* collision) const;

    void ResolveCollisions(PhysicsComponent* physics, double dt);
};

#endif
