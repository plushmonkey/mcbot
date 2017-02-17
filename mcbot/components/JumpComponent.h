#ifndef MCBOT_JUMP_COMPONENT_H_
#define MCBOT_JUMP_COMPONENT_H_

#include "../Component.h"
#include "../Collision.h"
#include <mclib/World.h>

class JumpComponent : public Component {
public:
    static const char* name;

private:
    CollisionDetector m_CollisionDetector;
    Minecraft::World* m_World;
    double m_Power;
    s64 m_LastJump;
    bool m_Jump;

public:
    JumpComponent(Minecraft::World* world, double power)
        : m_CollisionDetector(world), m_World(world), m_Power(power), m_LastJump(0), m_Jump(false)
    {

    }

    void Jump() { m_Jump = true; }
    void Update(double dt);
    void SetJumpPower(double power) { m_Power = power; }

    const char* GetName() const { return name; }
};

#endif
