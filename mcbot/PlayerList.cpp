#include "PlayerList.h"

#include "components/PhysicsComponent.h"
#include <vector>

PlayerList::PlayerList(GameClient* client) : m_Client(client) {
    m_Client->RegisterListener(this);
    m_Client->GetPlayerManager()->RegisterListener(this);
}

PlayerList::~PlayerList() {
    m_Client->UnregisterListener(this);
    m_Client->GetPlayerManager()->UnregisterListener(this);
}

std::vector<Minecraft::PlayerPtr> PlayerList::GetPlayers() const {
    std::vector<Minecraft::PlayerPtr> players;

    for (auto iter = m_Players.begin(); iter != m_Players.end(); ++iter)
        players.push_back(iter->second);

    return players;
}

void PlayerList::OnClientSpawn(Minecraft::PlayerPtr player) {
    m_BotPlayer = player;
}

void PlayerList::OnPlayerJoin(Minecraft::PlayerPtr player) {
    m_Players[player->GetName()] = player;
}

void PlayerList::OnPlayerLeave(Minecraft::PlayerPtr player) {
    m_Players.erase(player->GetName());
}

void PlayerList::UpdatePlayer(Minecraft::PlayerPtr player) {
    auto entity = player->GetEntity();
    if (!entity) return;

    auto find = m_VelocityTrackers.find(entity->GetEntityId());
    if (find == m_VelocityTrackers.end()) {
        VelocityTracker tracker;
        tracker.lastPosition = entity->GetPosition();

        m_VelocityTrackers[entity->GetEntityId()] = tracker;
    } else {
        Vector3d pos = entity->GetPosition();
        Vector3d velocity = (pos - find->second.lastPosition);

        find->second.velocity.AddValue(velocity);
        find->second.lastPosition = pos;

        Vector3d smoothed = find->second.velocity.GetSmoothedValue();
        //entity->SetVelocity(smoothed);
    }
}

void PlayerList::OnTick() {
    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    if (physics != nullptr && m_BotPlayer)
        m_BotPlayer->GetEntity()->SetPosition(physics->GetPosition());

    for (auto entry : m_Players) {
        auto player = entry.second;
        if (!player) continue;

        UpdatePlayer(player);
    }

    if (m_BotPlayer)
        UpdatePlayer(m_BotPlayer);
}

Minecraft::PlayerPtr PlayerList::GetPlayerByName(const std::wstring& name) {
    auto iter = m_Players.find(name);
    if (iter != m_Players.end())
        return iter->second;
    return nullptr;
}