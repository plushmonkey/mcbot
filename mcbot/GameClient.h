#ifndef GAME_CLIENT_H_
#define GAME_CLIENT_H_

#include "Inventory.h"
#include "Steering.h"
#include "Actor.h"

#include <mclib/Client.h>

class WorldGraph;

class GameClient : public ObserverSubject<ClientListener>, public Minecraft::ConnectionListener, public Actor {
private:
    Minecraft::Packets::PacketDispatcher m_Dispatcher;
    Minecraft::Connection m_Connection;
    Minecraft::EntityManager m_EntityManager;
    Minecraft::PlayerManager m_PlayerManager;
    Minecraft::World m_World;
    InventoryManager m_Inventories;

    bool m_Connected;

public:
    GameClient();
    ~GameClient();

    void OnSocketStateChange(Network::Socket::Status newState);
    bool login(std::string host, unsigned short port, std::string name, std::string password);

    void run();
    
    Minecraft::Packets::PacketDispatcher* GetDispatcher() { return &m_Dispatcher; }
    Minecraft::Connection* GetConnection() { return &m_Connection; }
    Minecraft::EntityManager* GetEntityManager() { return &m_EntityManager; }
    Minecraft::PlayerManager* GetPlayerManager() { return &m_PlayerManager; }
    Minecraft::World* GetWorld() { return &m_World; }
    InventoryManager* GetInventories() { return &m_Inventories; }

    std::shared_ptr<Inventory> GetInventory() { return m_Inventories.GetInventory(0); }

};

#endif
