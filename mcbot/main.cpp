#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <string>

#include "GameClient.h"
#include "Pathing.h"
#include "Utility.h"

class PlayerList : public Minecraft::PlayerListener {
private:
    Minecraft::PlayerManager* m_PlayerManager;
    std::map<std::wstring, Minecraft::PlayerPtr> m_Players;

public:
    PlayerList(Minecraft::PlayerManager* pm) : m_PlayerManager(pm) {
        pm->RegisterListener(this);
    }

    ~PlayerList() {
        m_PlayerManager->UnregisterListener(this);
    }

    void OnPlayerJoin(Minecraft::PlayerPtr player) {
        m_Players[player->GetName()] = player;
    }

    void OnPlayerLeave(Minecraft::PlayerPtr player) {
        m_Players.erase(player->GetName());
    }

    Minecraft::PlayerPtr GetPlayerByName(const std::wstring& name) {
        auto iter = m_Players.find(name);
        if (iter != m_Players.end())
            return iter->second;
        return nullptr;
    }
};

// Todo: rebuild graph when world changes. It will probably need to be async and updated infrequently. (500-800ms rebuild)
class WorldGraph : public ai::path::Graph, public Minecraft::WorldListener {
private:
    GameClient* m_Client;
    int chunksReceived = 0;

    ai::path::Node* GetNode(Vector3d pos) {
        Vector3i iPos = ToVector3i(pos);

        auto iter = m_Nodes.find(iPos);

        ai::path::Node* node = nullptr;
        if (iter == m_Nodes.end()) {
            node = new ai::path::Node(iPos);
            m_Nodes[iPos] = node;
        } else {
            node = iter->second;
        }

        return node;
    }

    // The check pos is not solid, the block above is not solid, and the block below is solid
    bool IsWalkable(Vector3d pos) {
        Minecraft::World* world = m_Client->GetWorld();

        Minecraft::BlockPtr checkBlock = world->GetBlock(ToVector3i(pos));

        Minecraft::BlockPtr aBlock = world->GetBlock(ToVector3i(pos + Vector3d(0, 1, 0)));
        Minecraft::BlockPtr bBlock = world->GetBlock(ToVector3i(pos - Vector3d(0, 1, 0)));

        return checkBlock && !checkBlock->IsSolid() && aBlock && !aBlock->IsSolid() && bBlock && bBlock->IsSolid();
    }

public:
    WorldGraph(GameClient* client)
        : m_Client(client)
    {
        client->GetWorld()->RegisterListener(this);
    }

    ~WorldGraph() {
        m_Client->GetWorld()->UnregisterListener(this);
    }

    void OnChunkLoad(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, u16 yIndex) {
        //if (++chunksReceived > 16)
            //BuildGraph();
    }

    void BuildGraph() {
        const int SearchRadius = 16 * 3;
        Vector3d position = m_Client->GetPlayerController()->GetPosition();

        static const std::vector<Vector3d> directions = {
            Vector3d(-1, 0, 0), Vector3d(1, 0, 0), Vector3d(0, -1, 0), Vector3d(0, 1, 0), Vector3d(0, 0, -1), Vector3d(0, 0, 1), // Directly nearby in flat area
            Vector3d(-1, 1, 0), Vector3d(1, 1, 0), Vector3d(0, 1, -1), Vector3d(0, 1, 1) // Up one step
        };

        this->Destroy();

        std::cout << "Building graph\n";

        s64 startTime = util::GetTime();

        Minecraft::World* world = m_Client->GetWorld();
        for (int x = -SearchRadius; x < SearchRadius; ++x) {
            for (int y = -SearchRadius; y < SearchRadius; ++y) {
                for (int z = -SearchRadius; z < SearchRadius; ++z) {
                    Vector3d checkPos = position + Vector3d(x, y, z);

                    Minecraft::BlockPtr checkBlock = world->GetBlock(ToVector3i(checkPos));

                    // Skip because it's not loaded yet or it's solid
                    if (!checkBlock || checkBlock->IsSolid()) continue;

                    if (IsWalkable(checkPos)) {
                        for (Vector3d direction : directions) {
                            Vector3d neighborPos = checkPos + direction;

                            if (IsWalkable(neighborPos)) {
                                ai::path::Node* current = GetNode(checkPos);
                                ai::path::Node* neighborNode = GetNode(neighborPos);

                                LinkNodes(current, neighborNode);
                            }
                        }
                    }
                }
            }
        }

        std::cout << "Graph built in " << (util::GetTime() - startTime) << "ms.\n";
    }
};

class AttackUpdate : public ClientListener {
private:
    GameClient* m_Client;
    Minecraft::PlayerPtr m_Target;
    s64 m_LastAttack;

    const double AttackRangeSq = 4 * 4;
    const s64 AttackDelay = 1000;

public:
    AttackUpdate(GameClient* client, Minecraft::PlayerPtr target)
        : m_Client(client),
          m_Target(target),
          m_LastAttack(0)
    {
        m_Client->RegisterListener(this);
    }

    ~AttackUpdate() {
        m_Client->UnregisterListener(this);
    }

    void OnTick() {
        s64 time = util::GetTime();

        if (time < m_LastAttack + AttackDelay) return;
        if (!m_Target || !m_Target->GetEntity()) return;

        Vector3d botPos = m_Client->GetPlayerController()->GetPosition();
        Vector3d targetPos = m_Target->GetEntity()->GetPosition();

        if ((targetPos - botPos).LengthSq() > AttackRangeSq) return;

        using namespace Minecraft::Packets::Outbound;

        // Send arm swing
        AnimationPacket animationPacket;
        m_Client->GetConnection()->SendPacket(&animationPacket);

        // Send attack
        UseEntityPacket useEntityPacket(m_Target->GetEntity()->GetEntityId(), UseEntityPacket::Action::Attack);
        m_Client->GetConnection()->SendPacket(&useEntityPacket);

        m_LastAttack = time;
    }

};

class BotUpdate : public ClientListener {
private:
    GameClient* m_Client;
    PlayerList m_Players;
    WorldGraph m_Graph;
    s64 m_StartupTime;
    bool m_Built;
    ai::path::Plan* m_Plan;
    std::shared_ptr<AttackUpdate> m_AttackUpdate;

    // Player is 0.3 wide, so it should only need to be within 0.2 of center of block to ensure no wall sticking. 0.15 is safer though.
    const double CenterToleranceSq = 0.15 * 0.15;

public:
    BotUpdate(GameClient* client)
        : m_Client(client),
          m_Players(client->GetPlayerManager()),
          m_Graph(client)
    {
        client->RegisterListener(this);

        m_StartupTime = util::GetTime();
        m_Built = false;
        m_Plan = nullptr;
    }

    ~BotUpdate() {
        m_Client->UnregisterListener(this);
    }

    void OnTick() {
        Vector3d target;

        Minecraft::PlayerPtr targetPlayer = m_Players.GetPlayerByName(L"plushmonkey");

        if (targetPlayer == nullptr) {
            if (m_AttackUpdate) {
                m_AttackUpdate.reset();
            }
        } else {
            Minecraft::EntityPtr entity = targetPlayer->GetEntity();

            if (entity != nullptr) {
                if (!m_AttackUpdate) {
                    m_AttackUpdate = std::make_shared<AttackUpdate>(m_Client, targetPlayer);
                }

                target = entity->GetPosition();

                if (util::GetTime() > m_StartupTime + 5000) {
                    if (!m_Built) {
                        m_Graph.BuildGraph();
                        m_Built = true;
                    }

                    Vector3d botPos = m_Client->GetPlayerController()->GetPosition();
                    if (m_Plan == nullptr || !m_Plan->HasNext()) {
                        s64 startTime = util::GetTime();

                        m_Plan = m_Graph.FindPath(ToVector3i(botPos), ToVector3i(target));

                        std::cout << "Plan built in " << (util::GetTime() - startTime) << "ms.\n";
                    }

                    if (m_Plan) {
                        while (true) {
                            ai::path::Node* current = m_Plan->GetCurrent();

                            if (!current) break;

                            Vector3d planPos = ToVector3d(current->GetPosition()) + Vector3d(0.5, 0, 0.5);

                            Vector3d toPlan = planPos - botPos;

                            if (toPlan.LengthSq() <= CenterToleranceSq) {
                                if (m_Plan->HasNext()) {
                                    current = m_Plan->Next();
                                    continue;
                                } else {
                                    break;
                                }
                            }
                            
                            std::cout << planPos << std::endl;
                            m_Client->GetPlayerController()->SetTargetPosition(planPos);
                            target = planPos;
                            break;
                        }
                    } else {
                        std::cout << "No plan\n";
                    }
                }
            }
        }
        m_Client->GetPlayerController()->LookAt(target);
    }
};


int main(void) {
    Minecraft::BlockRegistry::GetInstance()->RegisterVanillaBlocks();
    GameClient game;
    BotUpdate update(&game);

    game.login("127.0.0.1", 25565, "pathfinder", "pw");
    game.run();

    return 0;
}
