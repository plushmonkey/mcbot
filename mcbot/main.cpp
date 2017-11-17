#include "BotUpdate.h"
#include "components/EffectComponent.h"
#include "components/JumpComponent.h"
#include "components/PhysicsComponent.h"
#include "components/SpeedComponent.h"
#include "components/TargetingComponent.h"
#include "actions/WanderAction.h"
#include <mclib/util/VersionFetcher.h>

using mc::Vector3d;
using mc::Vector3i;
using mc::ToVector3d;
using mc::ToVector3i;

namespace {

const std::string name = "bot";
const std::string password = "pw";
const std::string server = "127.0.0.1";
const u16 port = 25565;

const std::wstring TargetPlayer = L"plushmonkey";

} // ns

class TargetPlayerAction : public DecisionAction {
private:
    GameClient* m_Client;

public:
    TargetPlayerAction(GameClient* client) : m_Client(client) { }

    bool HasTarget() {
        mc::core::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByName(TargetPlayer);
        if (!targetPlayer) return false;

        mc::entity::EntityPtr entity = targetPlayer->GetEntity();
        return entity != nullptr;
    }

    void Act() override {
        mc::core::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByName(TargetPlayer);
        auto entity = targetPlayer->GetEntity();

        auto targeting = GetActorComponent(m_Client, TargetingComponent);
        if (!targeting) return;

        targeting->SetTargetEntity(entity->GetEntityId());
        targeting->SetTarget(mc::ToVector3i(entity->GetPosition()));
    }
};

class AttackAction : public DecisionAction {
protected:
    GameClient* m_Client;
    s64 m_AttackCooldown;
    s64 m_LastAttack;

    bool SelectItem(s32 id) {
        mc::inventory::Inventory* inventory = m_Client->GetInventory();

        if (!inventory) return false;

        auto& hotbar = m_Client->GetHotbar();
        auto slot = hotbar.GetCurrentItem();

        if (slot.GetItemId() != id) {
            s32 itemIndex = inventory->FindItemById(id);

            std::cout << "Selecting item id " << id << std::endl;
            if (itemIndex == -1) {
                std::cout << "Not carrying item with id of " << id << std::endl;
                return false;
            } else {
                std::cout << "Item is in index " << itemIndex << std::endl;
            }

            s32 hotbarIndex = itemIndex - mc::inventory::Inventory::HOTBAR_SLOT_START;
            if (hotbarIndex >= 0)
                hotbar.SelectSlot(hotbarIndex);
        }

        return true;
    }

public:
    AttackAction(GameClient* client, s64 attackCooldown) : m_Client(client), m_LastAttack(0), m_AttackCooldown(attackCooldown) { }

    void Act() override {
        mc::core::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByName(TargetPlayer);
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

    virtual void Attack(mc::entity::EntityPtr entity) = 0;
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

    void Attack(mc::entity::EntityPtr entity) {
        auto physics = GetActorComponent(m_Client, PhysicsComponent);

        Vector3d botPos = physics->GetPosition();
        Vector3d targetPos = entity->GetPosition();

        if ((targetPos - botPos).LengthSq() > AttackRangeSq) return;

        using namespace mc::protocol::packets::out;

        const s32 StoneSwordId = 272;

        SelectItem(StoneSwordId);

        // Send attack
        UseEntityPacket useEntityPacket(entity->GetEntityId(), UseEntityPacket::Action::Attack);
        m_Client->GetConnection()->SendPacket(&useEntityPacket);

        // Send arm swing
        AnimationPacket animationPacket(mc::Hand::Main);
        m_Client->GetConnection()->SendPacket(&animationPacket);
    }

    double DistanceToTarget() {
        mc::core::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByName(TargetPlayer);

        if (targetPlayer) {
            mc::entity::EntityPtr entity = targetPlayer->GetEntity();
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

    auto effectComponent = std::make_unique<EffectComponent>(client->GetDispatcher());
    update->AddComponent(std::move(effectComponent));

    auto jump = std::make_unique<JumpComponent>(client->GetWorld(), 200);
    update->AddComponent(std::move(jump));

    auto targeting = std::make_unique<TargetingComponent>();
    update->AddComponent(std::move(targeting));

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

void CleanupBot(BotUpdate* update) {
    GameClient* client = update->GetClient();

    client->RemoveComponent(Component::GetIdFromName(EffectComponent::name));
    client->RemoveComponent(Component::GetIdFromName(JumpComponent::name));
    client->RemoveComponent(Component::GetIdFromName(TargetingComponent::name));
}

int main(void) {
    mc::block::BlockRegistry::GetInstance()->RegisterVanillaBlocks();
    mc::util::VersionFetcher versionFetcher(server, port);

    auto version = versionFetcher.GetVersion();

    GameClient game(version);
    BotUpdate update(&game);

    CreateBot(&update);

    game.login(server, port, name, password);
    
    game.run();

    CleanupBot(&update);
    return 0;
}
