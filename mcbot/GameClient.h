#ifndef GAME_CLIENT_H_
#define GAME_CLIENT_H_

#include "Inventory.h"
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
    InventoryManager m_Inventories;
    std::shared_ptr<WorldGraph> m_Graph;

    bool m_Connected;

public:
    GameClient();
    ~GameClient();

    void OnSocketStateChange(mc::network::Socket::Status newState);
    bool login(std::string host, unsigned short port, std::string name, std::string password);

    void run();
    
    mc::protocol::packets::PacketDispatcher* GetDispatcher() { return &m_Dispatcher; }
    mc::core::Connection* GetConnection() { return &m_Connection; }
    mc::entity::EntityManager* GetEntityManager() { return &m_EntityManager; }
    mc::core::PlayerManager* GetPlayerManager() { return &m_PlayerManager; }
    mc::world::World* GetWorld() { return &m_World; }
    InventoryManager* GetInventories() { return &m_Inventories; }

    std::shared_ptr<Inventory> GetInventory() { return m_Inventories.GetInventory(0); }
    std::shared_ptr<WorldGraph> GetGraph() { return m_Graph; }
};

#endif
