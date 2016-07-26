#include "GameClient.h"

#include <thread>
#include <chrono>

#include "Utility.h"

GameClient::GameClient()
    : m_Dispatcher(),
    m_Connection(&m_Dispatcher),
    m_EntityManager(&m_Dispatcher),
    m_PlayerManager(&m_Dispatcher, &m_EntityManager),
    m_World(&m_Dispatcher),
    m_PlayerController(&m_Connection, m_World, m_PlayerManager),
    m_Connected(false)
{
    m_Connection.RegisterListener(this);
}

GameClient::~GameClient() {

}

void GameClient::OnSocketStateChange(Network::Socket::Status newState) {
    m_Connected = (newState == Network::Socket::Status::Connected);
}

bool GameClient::login(std::string host, unsigned short port, std::string name, std::string password) {
    if (!m_Connection.Connect(host, port)) {
        return false;
    }

    m_Connection.Login(name, password);
    return true;
}

void GameClient::run() {
    s64 lastUpdate = 0;

    while (m_Connected) {
        try {
            m_Connection.CreatePacket();
        } catch (std::exception&) {

        }

        s64 time = util::GetTime();

        if (time < lastUpdate + 1000 / 20) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        lastUpdate = time;
        m_PlayerController.Update();
        NotifyListeners(&ClientListener::OnTick);
    }
}
