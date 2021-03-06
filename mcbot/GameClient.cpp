#include "GameClient.h"

#include <thread>
#include <chrono>
#include <iostream>

#include "Utility.h"
#include "WorldGraph.h"

GameClient::GameClient(mc::protocol::Version version)
    : m_Dispatcher(),
    m_Connection(&m_Dispatcher, version),
    m_EntityManager(&m_Dispatcher),
    m_PlayerManager(&m_Dispatcher, &m_EntityManager),
    m_World(&m_Dispatcher),
    m_Inventories(&m_Dispatcher, &m_Connection),
    m_Graph(new WorldGraph(this)),
    m_Connected(false),
    m_Hotbar(&m_Dispatcher, &m_Connection, &m_Inventories)
{
    m_Connection.RegisterListener(this);
}

GameClient::~GameClient() {
    m_Connection.UnregisterListener(this);
}

void GameClient::OnSocketStateChange(mc::network::Socket::Status newState) {
    m_Connected = (newState == mc::network::Socket::Status::Connected);
    std::cout << "Connected: " << std::boolalpha << m_Connected << std::endl;
}

bool GameClient::login(std::string host, unsigned short port, std::string name, std::string password) {
    if (!m_Connection.Connect(host, port)) {
        return false;
    }

    m_Connection.Login(name, password);
    return true;
}

void GameClient::run() {
    const s64 TickDelay = 1000/20;
    const s64 MaximumUpdates = 3;
    s64 lastUpdate = util::GetTime();
    s64 startupTime = util::GetTime();

    while (m_Connected) {
        try {
            m_Connection.CreatePacket();
        } catch (std::exception&) {

        }

        s64 time = util::GetTime();

        if (time < lastUpdate + TickDelay) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        s64 updateCount = (time - lastUpdate) / TickDelay;

        lastUpdate += TickDelay * updateCount;

        for (s64 i = 0; i < std::min(updateCount, MaximumUpdates); ++i) {
            Update(50.0 / 1000.0);
            NotifyListeners(&mc::core::ClientListener::OnTick);
        }

#ifdef _DEBUG
        if (util::GetTime() < startupTime + 10000) continue;
#endif

        m_Graph->ProcessQueue();
    }
}
