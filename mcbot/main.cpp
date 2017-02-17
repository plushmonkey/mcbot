#include "BotUpdate.h"
#include "components/JumpComponent.h"
#include "components/PhysicsComponent.h"
#include "components/SpeedComponent.h"
#include "components/TargetingComponent.h"
#include "actions/WanderAction.h"

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

    double DistanceToTarget() {
        Minecraft::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByName(L"plushmonkey");

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
};

void CreateBot(BotUpdate* update) {
    GameClient* client = update->GetClient();

    auto speed = std::make_shared<SpeedComponent>(client->GetConnection(), client->GetWorld());
    speed->SetMovementType(SpeedComponent::Movement::Normal);
    update->AddComponent(speed);

    auto jump = std::make_shared<JumpComponent>(client->GetWorld(), 200);
    update->AddComponent(jump);

    auto targeting = std::make_shared<TargetingComponent>();
    update->AddComponent(targeting);

    auto targetPlayer = std::make_shared<TargetPlayerAction>(client);

    std::shared_ptr<DecisionTreeNode> pathfindDecision = std::make_shared<RangeDecision<bool>>(
        targetPlayer, std::make_shared<WanderAction>(client), std::bind(&TargetPlayerAction::HasTarget, targetPlayer), true, true
    );

    auto meleeAction = std::make_shared<MeleeAction>(client);
    auto tree = std::make_shared<RangeDecision<double>>(
        meleeAction, pathfindDecision, std::bind(&MeleeAction::DistanceToTarget, meleeAction), 0.0, 2.0
    );

    update->SetDecisionTree(tree);
}

int main(void) {
    Minecraft::BlockRegistry::GetInstance()->RegisterVanillaBlocks();
    GameClient game;
    BotUpdate update(&game);

    CreateBot(&update);

    game.login("127.0.0.1", 25565, "bot", "pw");
    
    game.run();

    return 0;
}
