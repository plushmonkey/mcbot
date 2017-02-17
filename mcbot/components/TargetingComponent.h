#ifndef MCBOT_TARGETING_COMPONENT_H_
#define MCBOT_TARGETING_COMPONENT_H_

#include "../Component.h"
#include "../Collision.h"

class TargetingComponent : public Component {
public:
    static const char* name;

private:
    Vector3i m_Target;
    Minecraft::EntityId m_TargetEntity;

public:
    TargetingComponent()
        : m_TargetEntity(-1)
    {

    }

    void Update(double dt) { }

    Vector3i GetTarget() const { return m_Target; }
    void SetTarget(Vector3i target) { m_Target = target; }

    Minecraft::EntityId GetTargetEntity() const { return m_TargetEntity; }
    void SetTargetEntity(Minecraft::EntityId eid) { m_TargetEntity = eid; }

    const char* GetName() const { return name; }
};

#endif
