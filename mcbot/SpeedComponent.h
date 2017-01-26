#ifndef MCBOT_SPEED_COMPONENT_H_
#define MCBOT_SPEED_COMPONENT_H_

#include "Component.h"

#include <mclib/Connection.h>
#include <mclib/World.h>

// Sets the physics max speed based on current block and whether or not sprint is enabled.
class SpeedComponent : public Component {
public:
    static const char* name;

    enum class Movement { Normal, Sneaking, Sprinting };
private:
    Minecraft::Connection* m_Connection;
    Minecraft::World* m_World;
    Movement m_Movement;

public:
    SpeedComponent(Minecraft::Connection* connection, Minecraft::World* world)
        : m_Connection(connection),
        m_World(world),
        m_Movement(Movement::Normal)
    {
        
    }

    Movement GetMovementType() const { return m_Movement; }

    // Updates the server indicating a change in movement speed.
    void SetMovementType(Movement movement);

    void Update(double dt);

    const char* GetName() const { return name; }
};

#endif
