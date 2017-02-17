#ifndef MCBOT_PLAYERLIST_H_
#define MCBOT_PLAYERLIST_H_

#include "GameClient.h"
#include "Utility.h"

#include <map>

class PlayerList : public Minecraft::PlayerListener, public ClientListener {
private:
    GameClient* m_Client;
    std::map<std::wstring, Minecraft::PlayerPtr> m_Players;
    Minecraft::PlayerPtr m_BotPlayer;

    struct VelocityTracker {
        util::Smoother<Vector3d, 5> velocity;
        Vector3d lastPosition;
    };

    std::map<Minecraft::EntityId, VelocityTracker> m_VelocityTrackers;

    void UpdatePlayer(Minecraft::PlayerPtr player);

public:
    PlayerList(GameClient* client);

    ~PlayerList();

    std::vector<Minecraft::PlayerPtr> GetPlayers() const;

    void OnClientSpawn(Minecraft::PlayerPtr player);
    void OnPlayerJoin(Minecraft::PlayerPtr player);
    void OnPlayerLeave(Minecraft::PlayerPtr player);

    void OnTick();

    Minecraft::PlayerPtr GetPlayerByName(const std::wstring& name);
};

#endif
