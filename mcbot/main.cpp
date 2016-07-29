#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <string>

#include "GameClient.h"
#include "Pathing.h"
#include "Utility.h"

struct CastResult {
    std::vector<Vector3i> hit;
    std::size_t length;
    bool full;
};

CastResult RayCast(Minecraft::World* world, const Vector3d& from, Vector3d direction, std::size_t length) {
    CastResult result;

    std::vector<Vector3i> hit(length);

    direction.Normalize();
    result.length = length;
    result.full = true;

    for (std::size_t i = 0; i < length; ++i) {
        Vector3i check = ToVector3i(from + (direction * i));
        Minecraft::BlockPtr block = world->GetBlock(check);

        if (block && !block->IsSolid()) {
            hit.push_back(check);
        } else {
            result.full = false;
            result.length = i;
            break;
        }
    }

    result.hit = hit;

    return result;
}

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
    bool m_NeedsBuilt;

    ai::path::Node* GetNode(Vector3i pos) {
        auto iter = m_Nodes.find(pos);

        ai::path::Node* node = nullptr;
        if (iter == m_Nodes.end()) {
            node = new ai::path::Node(pos);
            m_Nodes[pos] = node;
        } else {
            node = iter->second;
        }

        return node;
    }

    // The check pos is not solid, the block above is not solid, and the block below is solid
    bool IsWalkable(Vector3i pos) {
        Minecraft::World* world = m_Client->GetWorld();

        Minecraft::BlockPtr checkBlock = world->GetBlock(pos);

        Minecraft::BlockPtr aBlock = world->GetBlock(pos + Vector3i(0, 1, 0));
        Minecraft::BlockPtr bBlock = world->GetBlock(pos - Vector3i(0, 1, 0));

        return checkBlock && !checkBlock->IsSolid() && aBlock && !aBlock->IsSolid() && bBlock && bBlock->IsSolid();
    }

    int IsSafeFall(Vector3i pos) {
        Minecraft::World* world = m_Client->GetWorld();

        Minecraft::BlockPtr checkBlock = world->GetBlock(pos);

        Minecraft::BlockPtr aBlock = world->GetBlock(pos + Vector3i(0, 1, 0));

        if (!checkBlock || checkBlock->IsSolid()) return 0;
        if (!aBlock || aBlock->IsSolid()) return 0;

        for (int i = 0; i < 4; ++i) {
            Minecraft::BlockPtr bBlock = world->GetBlock(pos - Vector3i(0, i+1, 0));

            if (bBlock && bBlock->IsSolid()) return i + 1;
            //if (bBlock && bBlock->IsSolid() || (bBlock->GetType() == 8 || bBlock->GetType() == 9)) return i + 1;
        }

        return 0;
    }

    bool IsWater(Vector3i pos) {
        Minecraft::World* world = m_Client->GetWorld();
        Minecraft::BlockPtr checkBlock = world->GetBlock(pos);

        return checkBlock && (checkBlock->GetType() == 8 || checkBlock->GetType() == 9);
    }

public:
    WorldGraph(GameClient* client)
        : m_Client(client),
          m_NeedsBuilt(false)
    {
        client->GetWorld()->RegisterListener(this);
    }

    ~WorldGraph() {
        m_Client->GetWorld()->UnregisterListener(this);
    }

    void OnChunkLoad(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, u16 yIndex) {
        m_NeedsBuilt = true;
    }

    void BuildGraph() {
        if (!m_NeedsBuilt) return;
        m_NeedsBuilt = false;

        const int SearchRadius = 16 * 4;
        const int YSearchRadius = 32;
        Vector3i position = ToVector3i(m_Client->GetPlayerController()->GetPosition());

        static const std::vector<Vector3i> directions = {
            Vector3i(-1, 0, 0), Vector3i(1, 0, 0), Vector3i(0, -1, 0), Vector3i(0, 1, 0), Vector3i(0, 0, -1), Vector3i(0, 0, 1), // Directly nearby in flat area
            Vector3i(-1, 1, 0), Vector3i(1, 1, 0), Vector3i(0, 1, -1), Vector3i(0, 1, 1), // Up one step
            Vector3i(-1, -1, 0), Vector3i(1, -1, 0), Vector3i(0, -1, -1), Vector3i(0, -1, 1) // Down one step
        };
        static const std::vector<Vector3i> waterDirections = {
            Vector3i(-1, 0, 0), Vector3i(1, 0, 0), Vector3i(0, -1, 0), Vector3i(0, 1, 0), Vector3i(0, 0, -1), Vector3i(0, 0, 1), // Directly nearby in flat area
            Vector3i(-1, 1, 0), Vector3i(1, 1, 0), Vector3i(0, 1, -1), Vector3i(0, 1, 1),
        };
        
        this->Destroy();

        m_Edges.reserve(70000);

        std::cout << "Building graph\n";

        s64 startTime = util::GetTime();

        Minecraft::World* world = m_Client->GetWorld();
        for (int x = -SearchRadius; x < SearchRadius; ++x) {
            for (int y = -YSearchRadius; y < YSearchRadius; ++y) {
                int checkY = (int)position.y + y;
                if (checkY <= 0 || checkY >= 256) continue;
                for (int z = -SearchRadius; z < SearchRadius; ++z) {
                    Vector3i checkPos = position + Vector3i(x, y, z);

                    Minecraft::BlockPtr checkBlock = world->GetBlock(checkPos);

                    // Skip because it's not loaded yet or it's solid
                    if (!checkBlock || checkBlock->IsSolid()) continue;

                    if (IsWalkable(checkPos)) {
                        for (Vector3i direction : directions) {
                            Vector3i neighborPos = checkPos + direction;

                            if (IsWalkable(neighborPos) || (IsSafeFall(neighborPos) && direction.y == 0)) {
                                ai::path::Node* current = GetNode(checkPos);
                                ai::path::Node* neighborNode = GetNode(neighborPos);

                                LinkNodes(current, neighborNode);
                            }
                        }
                    } else if (IsWater(checkPos)) {
                        for (Vector3i direction : waterDirections) {
                            Vector3i neighborPos = checkPos + direction;

                            if (IsWalkable(neighborPos) || IsWater(neighborPos)) {
                                ai::path::Node* current = GetNode(checkPos);
                                ai::path::Node* neighborNode = GetNode(neighborPos);

                                LinkNodes(current, neighborNode, 4.0);
                                LinkNodes(neighborNode, current, 4.0);
                            }
                        }
                    } else {
                        int fallDist = IsSafeFall(checkPos);
                        if (fallDist <= 0) continue;

                        ai::path::Node* current = GetNode(checkPos);

                        for (int i = 0; i < fallDist; ++i) {
                            Vector3i fallPos = checkPos - Vector3i(0, i + 1, 0);

                            Minecraft::BlockPtr block = world->GetBlock(fallPos);
                            if (!block || block->IsSolid()) break;

                            ai::path::Node* neighborNode = GetNode(fallPos);

                            LinkNodes(current, neighborNode);
                            current = neighborNode;
                        }
                    }
                }
            }
        }

        std::cout << "Graph built in " << (util::GetTime() - startTime) << "ms.\n";
        std::cout << "Nodes: " << m_Nodes.size() << std::endl;
        std::cout << "Edges: " << m_Edges.size() << std::endl;
    }
};

class AttackUpdate : public ClientListener {
protected:
    GameClient* m_Client;
    Minecraft::PlayerPtr m_Target;
    s64 m_AttackDelay;
    s64 m_LastAttack;
    

public:
    AttackUpdate(GameClient* client, Minecraft::PlayerPtr target, s64 attackDelay) 
        : m_Client(client),
          m_Target(target),
          m_AttackDelay(attackDelay),
          m_LastAttack(0)
    {
        m_Client->RegisterListener(this);
    }

    virtual ~AttackUpdate() {
        m_Client->UnregisterListener(this);
    }

    void OnTick() {
        s64 time = util::GetTime();

        if (time < m_LastAttack + m_AttackDelay) return;
        if (!m_Target || !m_Target->GetEntity()) return;

        Attack();

        m_LastAttack = time;
    }

    virtual void Attack() = 0;

    bool SelectItem(s32 id) {
        std::shared_ptr<Inventory> inventory = m_Client->GetInventory();

        Minecraft::Slot* slot = inventory->GetSlot(inventory->GetSelectedHotbarSlot() + Inventory::HOTBAR_SLOT_START);

        if (!slot || slot->GetItemId() != id) {
            s32 itemIndex = inventory->FindItemById(id);

            std::cout << "Selecting item id " << id << std::endl;
            if (itemIndex == -1) {
                std::cout << "Not carrying item with id of " << id << std::endl;
                return false;
            }

            // todo: Swap the bow into a hotbar slot if it isn't there

            s32 hotbarIndex = itemIndex - Inventory::HOTBAR_SLOT_START;
            m_Client->GetInventory()->SelectHotbarSlot(hotbarIndex);
        }

        return true;
    }
};

class MeleeAttackUpdate : public AttackUpdate {
private:
    const double AttackRangeSq = 4 * 4;

    enum { AttackDelay = 1000 };

public:
    MeleeAttackUpdate(GameClient* client, Minecraft::PlayerPtr target)
        : AttackUpdate(client, target, AttackDelay)
    {
        
    }

    void Attack() {
        Vector3d botPos = m_Client->GetPlayerController()->GetPosition();
        Vector3d targetPos = m_Target->GetEntity()->GetPosition();

        if ((targetPos - botPos).LengthSq() > AttackRangeSq) return;

        using namespace Minecraft::Packets::Outbound;

        const s32 StoneSwordId = 272;

        SelectItem(StoneSwordId);

        // Send arm swing
        AnimationPacket animationPacket;
        m_Client->GetConnection()->SendPacket(&animationPacket);

        // Send attack
        UseEntityPacket useEntityPacket(m_Target->GetEntity()->GetEntityId(), UseEntityPacket::Action::Attack);
        m_Client->GetConnection()->SendPacket(&useEntityPacket);
    }
};

class BowAttackUpdate : public AttackUpdate {
private:
    enum { AttackDelay = 1 };
    enum { DrawLength = 1500 };
    enum { ShootDelay = 750 };

    enum State {
        Drawing,
        Idle
    };

    State m_State;
    s64 m_StateStart;

public:
    BowAttackUpdate(GameClient* client, Minecraft::PlayerPtr target)
        : AttackUpdate(client, target, AttackDelay),
          m_State(Idle)
    {

    }

    void Attack() {
        s64 time = util::GetTime();
        Vector3d botPos = m_Client->GetPlayerController()->GetPosition();
        Vector3d targetPos = m_Target->GetEntity()->GetPosition();

        using namespace Minecraft::Packets::Outbound;

        if ((targetPos - botPos).LengthSq() < 5*5) return;

        Vector3d bowPos = botPos + Vector3d(0, 1, 0);
        Vector3d toTarget = (targetPos + Vector3d(0, 1, 0)) - bowPos;

        CastResult cast = RayCast(m_Client->GetWorld(), bowPos, toTarget, (std::size_t)toTarget.Length());
        // Stop the current attack unless it's almost ready to fire
        if (!cast.full && (m_State != State::Drawing || time < m_StateStart + DrawLength * .90)) {
            m_State = State::Idle;

            m_StateStart = time;
            const s32 StoneSwordId = 272;

            SelectItem(StoneSwordId);
        }

        if (m_State == State::Idle && time >= m_StateStart + ShootDelay) {
            const s32 BowId = 261;

            if (!SelectItem(BowId)) {
                m_StateStart = time;
                m_State = State::Idle;
                return;
            }

            std::cout << "Drawing bow\n";
            m_StateStart = time;
            m_State = State::Drawing;

            s32 selectedSlot = m_Client->GetInventory()->GetSelectedHotbarSlot();
            Minecraft::Slot* slot = m_Client->GetInventory()->GetSlot(selectedSlot + Inventory::HOTBAR_SLOT_START);

            PlayerBlockPlacementPacket bowDrawPacket(Vector3i(-1, -1, -1), 255, *slot, Vector3i(0, 0, 0));
            m_Client->GetConnection()->SendPacket(&bowDrawPacket);
        }

        if (m_State == State::Drawing && time >= m_StateStart + DrawLength) {
            std::cout << "Shooting bow\n";
            m_StateStart = time;
            m_State = State::Idle;

            PlayerDiggingPacket shootPacket(PlayerDiggingPacket::Status::ShootArrow, Vector3i(0, 0, 0), 0);
            m_Client->GetConnection()->SendPacket(&shootPacket);
        }
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
    std::shared_ptr<AttackUpdate> m_MeleeUpdate;
    std::shared_ptr<AttackUpdate> m_BowUpdate;
    
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
        Vector3d botPos = m_Client->GetPlayerController()->GetPosition();
        Vector3d target;

        m_Client->GetPlayerController()->SetTargetPosition(Vector3d(0, 0, 0));
        m_Client->GetPlayerController()->SetHandleFall(false);

        Minecraft::PlayerPtr targetPlayer = m_Players.GetPlayerByName(L"plushmonkey");

        if (targetPlayer == nullptr) {
            m_MeleeUpdate.reset();
            m_BowUpdate.reset();
        } else {
            Minecraft::EntityPtr entity = targetPlayer->GetEntity();

            if (entity != nullptr) {

                if (!m_BowUpdate) {
                    m_BowUpdate = std::make_shared<BowAttackUpdate>(m_Client, targetPlayer);
                }

                if (!m_MeleeUpdate) {
                    m_MeleeUpdate = std::make_shared<MeleeAttackUpdate>(m_Client, targetPlayer);
                }

                target = entity->GetPosition();

                if (util::GetTime() > m_StartupTime + 5000) {
                    if (!m_Built) {
                        m_Graph.BuildGraph();
                        m_Built = true;
                    }
                    
                    if (m_Plan == nullptr || !m_Plan->HasNext() || m_Plan->GetGoal()->GetPosition() != ToVector3i(targetPlayer->GetEntity()->GetPosition())) {
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
                            Minecraft::BlockPtr belowPlanBlock = m_Client->GetWorld()->GetBlock(planPos - Vector3d(0, 1, 0));

                            if (toPlan.LengthSq() <= CenterToleranceSq) {
                                if (m_Plan->HasNext()) {
                                    current = m_Plan->Next();
                                    continue;
                                } else {
                                    break;
                                }
                            }
                            
                            std::cout << planPos << std::endl;
                            Vector3d planDirection = Vector3Normalize(toPlan);
                            const double WalkSpeed = 4.3;
                            double moveSpeed = WalkSpeed;

                            if (belowPlanBlock && !belowPlanBlock->IsSolid())
                                moveSpeed *= 2;
                            Minecraft::BlockPtr currentBlock = m_Client->GetWorld()->GetBlock(botPos);
                            if (currentBlock && (currentBlock->GetType() == 8 || currentBlock->GetType() == 9))
                                moveSpeed = WalkSpeed / 2;

                            Vector3d delta = planDirection * moveSpeed * (50.0 / 1000.0);
                            double toPlanDist = toPlan.Length();

                            if (delta.Length() > toPlanDist)
                                delta = Vector3Normalize(delta) * toPlanDist;

                            m_Client->GetPlayerController()->Move(delta);
                            //target = planPos;
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

    game.login("127.0.0.1", 25565, "bot", "pw");
    game.run();

    return 0;
}
