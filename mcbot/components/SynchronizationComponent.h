#ifndef MCBOT_SYNCHRONIZATION_COMPONENT_H_
#define MCBOT_SYNCHRONIZATION_COMPONENT_H_

#include "../Component.h"

#include <mclib/core/Connection.h>
#include <mclib/protocol/packets/PacketDispatcher.h>
#include <mclib/core/PlayerManager.h>

// Keeps the server and client synchronized
class SynchronizationComponent : public Component, public mc::core::PlayerListener, public mc::protocol::packets::PacketHandler {
public:
    static const char* name;

private:
    mc::core::Connection* m_Connection;
    mc::core::PlayerManager* m_PlayerManager;
    mc::Vector3i m_SpawnPosition;
    bool m_Spawned;
    bool m_Dead;

public:
    SynchronizationComponent(mc::protocol::packets::PacketDispatcher* dispatcher, mc::core::Connection* connection, mc::core::PlayerManager* playerManager) 
        : mc::protocol::packets::PacketHandler(dispatcher), 
          m_Connection(connection),
          m_PlayerManager(playerManager), 
          m_Spawned(false),
          m_Dead(false)
    {
        m_PlayerManager->RegisterListener(this);

        dispatcher->RegisterHandler(mc::protocol::State::Play, mc::protocol::play::UpdateHealth, this);
        dispatcher->RegisterHandler(mc::protocol::State::Play, mc::protocol::play::EntityVelocity, this);
        dispatcher->RegisterHandler(mc::protocol::State::Play, mc::protocol::play::SpawnPosition, this);
    }

    ~SynchronizationComponent() {
        m_PlayerManager->UnregisterListener(this);
        GetDispatcher()->UnregisterHandler(this);
    }

    bool HasSpawned() const { return m_Spawned; }
    void OnClientSpawn(mc::core::PlayerPtr player);

    void HandlePacket(mc::protocol::packets::in::UpdateHealthPacket* packet);
    void HandlePacket(mc::protocol::packets::in::EntityVelocityPacket* packet);
    void HandlePacket(mc::protocol::packets::in::SpawnPositionPacket* packet);

    void Update(double dt);

    const char* GetName() const { return name; }
};

#endif
