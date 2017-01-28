#ifndef MCBOT_SYNCHRONIZATION_COMPONENT_H_
#define MCBOT_SYNCHRONIZATION_COMPONENT_H_

#include "Component.h"

#include <mclib/Connection.h>
#include <mclib/Packets/PacketDispatcher.h>
#include <mclib/PlayerManager.h>

// Keeps the server and client synchronized
class SynchronizationComponent : public Component, public Minecraft::PlayerListener, public Minecraft::Packets::PacketHandler {
public:
    static const char* name;

private:
    Minecraft::Connection* m_Connection;
    Minecraft::PlayerManager* m_PlayerManager;
    bool m_Spawned;

public:
    SynchronizationComponent(Minecraft::Packets::PacketDispatcher* dispatcher, Minecraft::Connection* connection, Minecraft::PlayerManager* playerManager) 
        : Minecraft::Packets::PacketHandler(dispatcher), 
          m_Connection(connection),
          m_PlayerManager(playerManager), 
          m_Spawned(false)
    {
        m_PlayerManager->RegisterListener(this);

        dispatcher->RegisterHandler(Minecraft::Protocol::State::Play, Minecraft::Protocol::Play::UpdateHealth, this);
        dispatcher->RegisterHandler(Minecraft::Protocol::State::Play, Minecraft::Protocol::Play::EntityVelocity, this);
    }

    ~SynchronizationComponent() {
        m_PlayerManager->UnregisterListener(this);
        GetDispatcher()->UnregisterHandler(this);
    }

    bool HasSpawned() const { return m_Spawned; }
    void OnClientSpawn(Minecraft::PlayerPtr player);

    void HandlePacket(Minecraft::Packets::Inbound::UpdateHealthPacket* packet);
    void HandlePacket(Minecraft::Packets::Inbound::EntityVelocityPacket* packet);

    void Update(double dt);

    const char* GetName() const { return name; }
};

#endif
