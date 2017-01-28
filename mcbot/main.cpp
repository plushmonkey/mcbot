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
#include "JumpComponent.h"
#include "TargetingComponent.h"

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
            //entity->SetVelocity(smoothed);
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

class Pathfinder {
private:
    GameClient* m_Client;
    ai::path::Plan* m_Plan;

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
            Minecraft::BlockPtr block = world->GetBlock(check).GetBlock();
            bool walkable = m_Client->GetGraph()->IsWalkable(check);
            if (walkable && block && !block->IsSolid()) {
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

    bool IsNearBlocks(Vector3d pos) {
        Minecraft::World* world = m_Client->GetWorld();
        static const std::vector<Vector3d> directions = {
            Vector3d(-1, 0, 0), Vector3d(1, 0, 0), Vector3d(0, 0, -1), Vector3d(0, 0, 1)
        };

        for (Vector3d dir : directions) {
            Minecraft::BlockPtr block = world->GetBlock(pos + dir).GetBlock();

            if (!block || block->IsSolid()) return true;
        }
        return false;
    }

    void SmoothPath() {
        if (!m_Plan) return;

        std::vector<ai::path::Node*>& nodes = m_Plan->GetNodes();
        if (nodes.size() <= 2) return;

        std::vector<ai::path::Node*> output;
        output.push_back(nodes[0]);
        std::size_t index = 2;

        while (index < nodes.size() - 1) {
            Vector3d from = ToVector3d(nodes[index]->GetPosition());
            Vector3d to = ToVector3d(output.back()->GetPosition());
            Vector3d direction = to - from;
            std::size_t length = (std::size_t)direction.Length() + 1;
            direction.Normalize();

            if (from.y != to.y || IsNearBlocks(from) || IsNearBlocks(to)) {
                output.push_back(nodes[index - 1]);
            } else {
                CastResult result = RayCast(m_Client->GetWorld(), from, direction, length);
                if (!result.full) {
                    output.push_back(nodes[index - 1]);
                }
            }

            ++index;
        }

        output.push_back(nodes.back());

        nodes.clear();
        nodes.insert(nodes.end(), output.begin(), output.end());
    }
public:
    Pathfinder(GameClient* client)
        : m_Client(client), m_Plan(nullptr)
    {
        
    }

    Vector3i GetGroundLevel(Vector3i pos) {
        Minecraft::World* world = m_Client->GetWorld();

        s32 y;
        for (y = (s32)pos.y; y >= 0; --y) {
            Minecraft::BlockPtr block = world->GetBlock(Vector3i(pos.x, y, pos.z)).GetBlock();

            if (block && block->IsSolid()) break;
        }

        return Vector3i(pos.x, y + 1, pos.z);
    }

    void Update() {
        auto physics = GetActorComponent(m_Client, PhysicsComponent);
        if (!physics) return;

        auto targeting = GetActorComponent(m_Client, TargetingComponent);
        if (!targeting) return;
        
        Vector3i target = targeting->GetTarget();
        Vector3i toTarget = target - ToVector3i(physics->GetPosition());
        if (toTarget.LengthSq() <= 2.0 * 2.0) {
            Vector3d accel = -physics->GetVelocity() * 20;
            accel.y = 0;
            physics->ApplyAcceleration(accel);
            //physics->ClearHorizontalVelocity();
            return;
        }
        
        Vector3i targetGroundPos = GetGroundLevel(target);

        if (m_Plan == nullptr || !m_Plan->HasNext() || m_Plan->GetGoal()->GetPosition() != targetGroundPos) {
            s64 startTime = util::GetTime();

            m_Plan = m_Client->GetGraph()->FindPath(GetGroundLevel(ToVector3i(physics->GetPosition())), targetGroundPos);

            SmoothPath();

            //std::cout << "Plan built in " << (util::GetTime() - startTime) << "ms.\n";
        }

        if (!m_Plan || m_Plan->GetSize() == 0) {
            std::cout << "No plan to " << targetGroundPos << std::endl;
        }

        Vector3d position = physics->GetPosition();
        Vector3d alignTarget = ToVector3d(target);

        ai::PathFollowSteering steer(m_Client, m_Plan, 0.25);
        
        ai::SteeringAcceleration steering = steer.GetSteering();
        physics->ApplyAcceleration(steering.movement);
        physics->ApplyRotation(steering.rotation);

        if (m_Plan && m_Plan->GetCurrent())
            alignTarget = (alignTarget + ToVector3d(m_Plan->GetCurrent()->GetPosition())) / 2.0;
        //else
          //  physics->ClearHorizontalVelocity();
        ai::FaceSteering align(m_Client, alignTarget, 0.1, 1, 1);
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
        
        Minecraft::BlockPtr projectedBlock = m_Client->GetWorld()->GetBlock(projectedPos).GetBlock();
        Minecraft::BlockPtr block = m_Client->GetWorld()->GetBlock(projectedPos - Vector3d(0, 1, 0)).GetBlock();
        
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
        auto targeting = GetActorComponent(m_Client, TargetingComponent);

        ai::FaceSteering align(m_Client, entity->GetPosition(), 0.1, 1, 1);

        physics->ApplyRotation(align.GetSteering().rotation);
        //physics->ClearHorizontalVelocity();
        Vector3d entityHeading = util::OrientationToVector(entity->GetYaw());

        Vector3d target = entity->GetPosition() - entityHeading * 2;
        targeting->SetTarget(ToVector3i(target));

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

class TargetPlayerAction : public DecisionAction {
private:
    GameClient* m_Client;

public:
    TargetPlayerAction(GameClient* client) : m_Client(client) { }

    bool HasTarget() {
        Minecraft::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByName(L"plushmonkey");
        if (!targetPlayer) return false;

        Minecraft::EntityPtr entity = targetPlayer->GetEntity();
        return entity != nullptr;
    }

    void Act() override {
        Minecraft::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByName(L"plushmonkey");
        auto entity = targetPlayer->GetEntity();

        auto targeting = GetActorComponent(m_Client, TargetingComponent);
        if (!targeting) return;

        targeting->SetTargetEntity(entity->GetEntityId());
        targeting->SetTarget(ToVector3i(entity->GetPosition()));
    }
};

class BotUpdate : public ClientListener {
private:
    GameClient* m_Client;
    PlayerList m_Players;
    s64 m_StartupTime;
    bool m_Built;

    std::shared_ptr<Pathfinder> m_Pathfinder;

    DecisionTreeNodePtr m_DecisionTree;

public:
    BotUpdate(GameClient* client)
        : m_Client(client),
          m_Players(client)
    {
        client->RegisterListener(this);

        auto physics = std::make_shared<PhysicsComponent>(m_Client->GetWorld());
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

        auto jump = std::make_shared<JumpComponent>(m_Client->GetWorld(), 200);
        jump->SetOwner(client);
        client->AddComponent(jump);

        auto targeting = std::make_shared<TargetingComponent>();
        targeting->SetOwner(client);
        client->AddComponent(targeting);

        m_StartupTime = util::GetTime();
        m_Built = false;

        Minecraft::BlockPtr block = Minecraft::BlockRegistry::GetInstance()->GetBlock(1, 0);

        m_Pathfinder = std::make_shared<Pathfinder>(m_Client);
        auto targetPlayer = std::make_shared<TargetPlayerAction>(m_Client);

        std::shared_ptr<DecisionTreeNode> pathfindDecision = std::make_shared<RangeDecision<bool>>(
            targetPlayer, std::make_shared<WanderAction>(m_Client), std::bind(&TargetPlayerAction::HasTarget, targetPlayer), true, true
        );

        m_DecisionTree = std::make_shared<RangeDecision<double>>(
            std::make_shared<MeleeAction>(m_Client), pathfindDecision, std::bind(&BotUpdate::DistanceToTarget, this), 0.0, 2.0
        );
    }

    ~BotUpdate() {
        m_Client->RemoveComponent(Component::GetIdFromName(SynchronizationComponent::name));
        m_Client->UnregisterListener(this);
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

        auto physics = GetActorComponent(m_Client, PhysicsComponent);
        if (!physics) return;

        m_Pathfinder->Update();
        physics->Integrate(50.0 / 1000.0);
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
