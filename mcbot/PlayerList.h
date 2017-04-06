#ifndef MCBOT_PLAYERLIST_H_
#define MCBOT_PLAYERLIST_H_

#include "GameClient.h"
#include "Utility.h"

#include <map>

class PlayerList : public mc::core::PlayerListener, public mc::core::ClientListener {
private:
    GameClient* m_Client;
    std::map<std::wstring, mc::core::PlayerPtr> m_Players;
    mc::core::PlayerPtr m_BotPlayer;

    struct VelocityTracker {
        util::Smoother<mc::Vector3d, 5> velocity;
        mc::Vector3d lastPosition;
    };

    std::map<mc::EntityId, VelocityTracker> m_VelocityTrackers;

    void UpdatePlayer(mc::core::PlayerPtr player);

public:
    PlayerList(GameClient* client);

    ~PlayerList();

    std::vector<mc::core::PlayerPtr> GetPlayers() const;

    void OnClientSpawn(mc::core::PlayerPtr player);
    void OnPlayerJoin(mc::core::PlayerPtr player);
    void OnPlayerLeave(mc::core::PlayerPtr player);

    void OnTick();

    mc::core::PlayerPtr GetPlayerByName(const std::wstring& name);
};

#endif
