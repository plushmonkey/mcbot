#ifndef MCBOT_SYNCHRONIZATION_COMPONENT_H_
#define MCBOT_SYNCHRONIZATION_COMPONENT_H_

#include "Component.h"

#include <mclib/Connection.h>
#include <mclib/PlayerManager.h>

// Keeps the server and client synchronized
class SynchronizationComponent : public Component, public Minecraft::PlayerListener {
public:
    static const char* name;

private:
    Minecraft::Connection* m_Connection;
    Minecraft::PlayerManager* m_PlayerManager;
    bool m_Spawned;

public:
    SynchronizationComponent(Minecraft::Connection* connection, Minecraft::PlayerManager* playerManager) 
        : m_Connection(connection),
          m_PlayerManager(playerManager), 
          m_Spawned(false)
    {
        m_PlayerManager->RegisterListener(this);
    }

    ~SynchronizationComponent() {
        m_PlayerManager->UnregisterListener(this);
    }

    bool HasSpawned() const { return m_Spawned; }
    void OnClientSpawn(Minecraft::PlayerPtr player);

    void Update(double dt);

    const char* GetName() const { return name; }
};

#endif
