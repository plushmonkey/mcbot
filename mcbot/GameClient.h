#ifndef GAME_CLIENT_H_
#define GAME_CLIENT_H_

#include <mclib/inventory/Inventory.h>
#include <mclib/inventory/Hotbar.h>
#include "Steering.h"
#include "Actor.h"

#include <mclib/core/Client.h>

class WorldGraph;

class GameClient : public mc::util::ObserverSubject<mc::core::ClientListener>, public mc::core::ConnectionListener, public Actor {
private:
    mc::protocol::packets::PacketDispatcher m_Dispatcher;
    mc::core::Connection m_Connection;
    mc::entity::EntityManager m_EntityManager;
    mc::core::PlayerManager m_PlayerManager;
    mc::world::World m_World;
    mc::inventory::InventoryManager m_Inventories;
    mc::inventory::Hotbar m_Hotbar;
    std::unique_ptr<WorldGraph> m_Graph;

    bool m_Connected;

public:
    GameClient(mc::protocol::Version version = mc::protocol::Version::Minecraft_1_11_2);
    ~GameClient();

    void OnSocketStateChange(mc::network::Socket::Status newState);
    bool login(std::string host, unsigned short port, std::string name, std::string password);

    void run();
    
    mc::protocol::packets::PacketDispatcher* GetDispatcher() { return &m_Dispatcher; }
    mc::core::Connection* GetConnection() { return &m_Connection; }
    mc::entity::EntityManager* GetEntityManager() { return &m_EntityManager; }
    mc::core::PlayerManager* GetPlayerManager() { return &m_PlayerManager; }
    mc::world::World* GetWorld() { return &m_World; }
    mc::inventory::InventoryManager* GetInventories() { return &m_Inventories; }

    mc::inventory::Inventory* GetInventory() { return m_Inventories.GetInventory(0); }
    mc::inventory::Hotbar& GetHotbar() { return m_Hotbar; }
    WorldGraph* GetGraph() { return m_Graph.get(); }
};

#endif
