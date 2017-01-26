#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <string>
#include <queue>
#include <functional>
#include "WorldGraph.h"
#include "Steering.h"
#include "PhysicsComponent.h"
#include "Collision.h"
#include "SynchronizationComponent.h"
#include "SpeedComponent.h"
#include "GraphComponent.h"

#include "GameClient.h"
#include "Pathing.h"
#include "Utility.h"
#include "Decision.h"

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

public:
    PlayerList(GameClient* client) : m_Client(client) {
        m_Client->RegisterListener(this);
        m_Client->GetPlayerManager()->RegisterListener(this);
    }

    ~PlayerList() {
        m_Client->UnregisterListener(this);
        m_Client->GetPlayerManager()->UnregisterListener(this);
    }

    void OnClientSpawn(Minecraft::PlayerPtr player) {
        m_BotPlayer = player;
    }

    void OnPlayerJoin(Minecraft::PlayerPtr player) {
        m_Players[player->GetName()] = player;
    }

    void OnPlayerLeave(Minecraft::PlayerPtr player) {
        m_Players.erase(player->GetName());
    }

    void UpdatePlayer(Minecraft::PlayerPtr player) {
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
            Vector3d entityVelocity = smoothed * 8000;
            entity->SetVelocity(Vector3s((short)entityVelocity.x, (short)entityVelocity.y, (short)entityVelocity.z));
        }
    }

    void OnTick() {
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

    Minecraft::PlayerPtr GetPlayerByName(const std::wstring& name) {
        auto iter = m_Players.find(name);
        if (iter != m_Players.end())
            return iter->second;
        return nullptr;
    }
};

/*
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

        if ((targetPos - botPos).LengthSq() < 4*4) return;

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
};*/

class PathfindAction : public DecisionAction {
private:
    GameClient* m_Client;
    ai::path::Plan* m_Plan;
    PlayerList* m_PlayerList;

public:
    PathfindAction(GameClient* client, PlayerList* playerList)
        : m_Client(client), m_PlayerList(playerList), m_Plan(nullptr)
    {
        
    }

    Vector3i GetGroundLevel(Vector3i pos) {
        Minecraft::World* world = m_Client->GetWorld();

        s32 y;
        for (y = (s32)pos.y; y >= 0; --y) {
            Minecraft::BlockPtr block = world->GetBlock(Vector3i(pos.x, y, pos.z));

            if (block && block->IsSolid()) break;
        }

        return Vector3i(pos.x, y + 1, pos.z);
    }

    void Act() override {
        auto physics = GetActorComponent(m_Client, PhysicsComponent);
        if (!physics) return;

        Minecraft::PlayerPtr targetPlayer = m_PlayerList->GetPlayerByName(L"plushmonkey");
        if (targetPlayer == nullptr) {
            physics->SetVelocity(Vector3d(0, 0, 0));
            return;
        }

        Minecraft::EntityPtr entity = targetPlayer->GetEntity();
        if (!entity) {
            physics->SetVelocity(Vector3d(0, 0, 0));
            return;
        }

        Vector3i entityGroundPos = GetGroundLevel(ToVector3i(entity->GetPosition()));

        if (m_Plan == nullptr || !m_Plan->HasNext() || m_Plan->GetGoal()->GetPosition() != entityGroundPos) {
            s64 startTime = util::GetTime();

            auto graphComponent = GetActorComponent(m_Client, GraphComponent);
            if (!graphComponent) return;

            m_Plan = graphComponent->GetGraph()->FindPath(GetGroundLevel(ToVector3i(physics->GetPosition())), entityGroundPos);

            std::cout << "Plan built in " << (util::GetTime() - startTime) << "ms.\n";
        }

        if (!m_Plan) {
            std::cout << "No plan to " << entity->GetPosition() << std::endl;
        }

        Vector3d position = physics->GetPosition();

        Vector3d target = entity->GetPosition();
        Vector3d velocity = ToVector3d(entity->GetVelocity()) * (20.0 / 8000.0);

        static CollisionDetector collisionDetector(m_Client->GetWorld());

        ai::PathFollowSteering steer(m_Client, m_Plan, 0.25);
        
        ai::SteeringAcceleration steering = steer.GetSteering();
        physics->ApplyAcceleration(steering.movement);
        physics->ApplyRotation(steering.rotation);

        if (m_Plan && m_Plan->GetCurrent())
            target = (target + ToVector3d(m_Plan->GetCurrent()->GetPosition())) / 2.0;
        else
            physics->SetVelocity(Vector3d(0, 0, 0));
        ai::FaceSteering align(m_Client, target, 0.1, 1, 1);
        physics->ApplyRotation(align.GetSteering().rotation);
    }
};

class WanderAction : public DecisionAction {
private:
    GameClient* m_Client;
    ai::WanderSteering m_Wander;

public:
    WanderAction(GameClient* client)
        : m_Client(client),
        m_Wander(client, 15.0, 2.0, 0.15)
    {

    }
    void Act() override {
        auto physics = GetActorComponent(m_Client, PhysicsComponent);
        if (!physics) return;

        ai::SteeringAcceleration steering = m_Wander.GetSteering();

        physics->ApplyAcceleration(steering.movement);
        physics->ApplyRotation(steering.rotation);

        Vector3d pos = physics->GetPosition();
        Vector3d vel = physics->GetVelocity();

        Vector3d projectedPos = pos + (vel * 50.0 / 1000.0);
        
        Minecraft::BlockPtr projectedBlock = m_Client->GetWorld()->GetBlock(projectedPos);
        Minecraft::BlockPtr block = m_Client->GetWorld()->GetBlock(projectedPos - Vector3d(0, 1, 0));
        
        if (!projectedBlock || projectedBlock->IsSolid() || !block || !block->IsSolid()) {
            physics->SetVelocity(-vel * 1.5);
            physics->ApplyRotation(3.14159f);
        }
    }
};

class AttackAction : public DecisionAction {
protected:
    GameClient* m_Client;
    s64 m_AttackCooldown;
    s64 m_LastAttack;

    bool SelectItem(s32 id) {
        std::shared_ptr<Inventory> inventory = m_Client->GetInventory();

        Minecraft::Slot* slot = inventory->GetSlot(inventory->GetSelectedHotbarSlot() + Inventory::HOTBAR_SLOT_START);

        if (!slot || slot->GetItemId() != id) {
            s32 itemIndex = inventory->FindItemById(id);

            std::cout << "Selecting item id " << id << std::endl;
            if (itemIndex == -1) {
                std::cout << "Not carrying item with id of " << id << std::endl;
                return false;
            } else {
                std::cout << "Item is in index " << itemIndex << std::endl;
            }

            s32 hotbarIndex = itemIndex - Inventory::HOTBAR_SLOT_START;
            m_Client->GetInventory()->SelectHotbarSlot(hotbarIndex);
        }

        return true;
    }

public:
    AttackAction(GameClient* client, s64 attackCooldown) : m_Client(client), m_LastAttack(0), m_AttackCooldown(attackCooldown) { }

    void Act() override {
        Minecraft::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByName(L"plushmonkey");
        auto entity = targetPlayer->GetEntity();
        auto physics = GetActorComponent(m_Client, PhysicsComponent);

        ai::FaceSteering align(m_Client, entity->GetPosition(), 0.1, 1, 1);

        physics->ApplyRotation(align.GetSteering().rotation);
        physics->SetVelocity(Vector3d(0, 0, 0));

        s64 time = util::GetTime();

        if (time >= m_LastAttack + m_AttackCooldown) {
            Attack(entity);

            m_LastAttack = time;
        }
    }

    virtual void Attack(Minecraft::EntityPtr entity) = 0;
};

class MeleeAction : public AttackAction {
private:
    const double AttackRangeSq = 3 * 3;

    enum { AttackDelay = 2000 };

public:
    MeleeAction(GameClient* client)
        : AttackAction(client, AttackDelay)
    {

    }

    void Attack(Minecraft::EntityPtr entity) {
        auto physics = GetActorComponent(m_Client, PhysicsComponent);

        Vector3d botPos = physics->GetPosition();
        Vector3d targetPos = entity->GetPosition();

        if ((targetPos - botPos).LengthSq() > AttackRangeSq) return;

        using namespace Minecraft::Packets::Outbound;

        const s32 StoneSwordId = 272;

        SelectItem(StoneSwordId);

        // Send arm swing
        AnimationPacket animationPacket(Minecraft::Hand::Main);
        m_Client->GetConnection()->SendPacket(&animationPacket);

        // Send attack
        UseEntityPacket useEntityPacket(entity->GetEntityId(), UseEntityPacket::Action::Attack);
        m_Client->GetConnection()->SendPacket(&useEntityPacket);
    }
};

class BotUpdate : public ClientListener {
private:
    GameClient* m_Client;
    PlayerList m_Players;
    s64 m_StartupTime;
    bool m_Built;

    std::shared_ptr<PathfindAction> m_PathfindAction;

    DecisionTreeNodePtr m_DecisionTree;

public:
    BotUpdate(GameClient* client)
        : m_Client(client),
          m_Players(client)
    {
        client->RegisterListener(this);

        auto graphComponent = std::make_shared<GraphComponent>(m_Client);
        graphComponent->SetOwner(m_Client);
        m_Client->AddComponent(graphComponent);

        auto physics = std::make_shared<PhysicsComponent>();
        physics->SetOwner(client);
        physics->SetMaxAcceleration(100.0f);
        physics->SetMaxRotation(3.14159 * 8);
        client->AddComponent(physics);

        auto sync = std::make_shared<SynchronizationComponent>(m_Client->GetDispatcher(), m_Client->GetConnection(), m_Client->GetPlayerManager());
        sync->SetOwner(m_Client);
        m_Client->AddComponent(sync);

        auto speed = std::make_shared<SpeedComponent>(m_Client->GetConnection(), m_Client->GetWorld());
        speed->SetOwner(m_Client);
        speed->SetMovementType(SpeedComponent::Movement::Normal);
        m_Client->AddComponent(speed);

        m_StartupTime = util::GetTime();
        m_Built = false;

        Minecraft::BlockPtr block = Minecraft::BlockRegistry::GetInstance()->GetBlock(1, 0);

        m_PathfindAction = std::make_shared<PathfindAction>(m_Client, &m_Players);

        std::shared_ptr<DecisionTreeNode> pathfindDecision = std::make_shared<RangeDecision<bool>>(
            m_PathfindAction, std::make_shared<WanderAction>(m_Client), std::bind(&BotUpdate::HasValidTarget, this), true, true
        );

        m_DecisionTree = std::make_shared<RangeDecision<double>>(
            std::make_shared<MeleeAction>(m_Client), pathfindDecision, std::bind(&BotUpdate::DistanceToTarget, this), 0.0, 2.0
        );
    }

    ~BotUpdate() {
        m_Client->RemoveComponent(Component::GetIdFromName(SynchronizationComponent::name));
        m_Client->UnregisterListener(this);
    }

    bool HasValidTarget() {
        Minecraft::PlayerPtr targetPlayer = m_Players.GetPlayerByName(L"plushmonkey");
        if (!targetPlayer) return false;

        Minecraft::EntityPtr entity = targetPlayer->GetEntity();
        return entity != nullptr;
    }

    double DistanceToTarget() {
        Minecraft::PlayerPtr targetPlayer = m_Players.GetPlayerByName(L"plushmonkey");
        if (targetPlayer) {
            Minecraft::EntityPtr entity = targetPlayer->GetEntity();
            if (entity) {
                auto physics = GetActorComponent(m_Client, PhysicsComponent);
                if (physics)
                    return entity->GetPosition().Distance(physics->GetPosition());
            }
        }
        return std::numeric_limits<double>::max();
    }

    void OnTick() {
        auto sync = GetActorComponent(m_Client, SynchronizationComponent);
        if (!sync || !sync->HasSpawned()) return;

        auto speed = GetActorComponent(m_Client, SpeedComponent);
        if (speed) {
            if (speed->GetMovementType() != SpeedComponent::Movement::Sprinting)
                speed->SetMovementType(SpeedComponent::Movement::Sprinting);
        }

        DecisionAction* action = m_DecisionTree->Decide();
        if (action)
            action->Act();
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
