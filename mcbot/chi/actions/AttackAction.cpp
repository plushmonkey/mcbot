#include "AttackAction.h"
#include "../../Utility.h"
#include "../../components/PhysicsComponent.h"
#include "../../components/TargetingComponent.h"

using mc::Vector3d;
using mc::Vector3i;
using mc::ToVector3i;

bool AttackAction::SelectItem(s32 id) {
    std::shared_ptr<Inventory> inventory = m_Client->GetInventory();

    mc::inventory::Slot* slot = inventory->GetSlot(inventory->GetSelectedHotbarSlot() + Inventory::HOTBAR_SLOT_START);

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

bool AttackAction::CanAttack() {
    s64 time = util::GetTime();

    auto targeting = GetActorComponent(m_Client, TargetingComponent);

    mc::core::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByEntityId(targeting->GetTargetEntity());
    if (!targetPlayer) return false;

    auto entity = targetPlayer->GetEntity();
    if (!entity) return false;

    auto physics = GetActorComponent(m_Client, PhysicsComponent);

    ai::FaceSteering align(m_Client, entity->GetPosition(), 0.1, 1, 1);
    physics->ApplyRotation(align.GetSteering().rotation);

    Vector3d entityHeading = util::OrientationToVector(entity->GetYaw());
    Vector3d target = entity->GetPosition() - entityHeading * 2;

    targeting->SetTarget(ToVector3i(target));

    return (time >= s_LastGlobalAttack + s_GlobalAttackCooldown) && (time >= m_LastAttack + m_AttackCooldown);
}

void AttackAction::Act() {
    auto targeting = GetActorComponent(m_Client, TargetingComponent);

    mc::core::PlayerPtr targetPlayer = m_Client->GetPlayerManager()->GetPlayerByEntityId(targeting->GetTargetEntity());
    if (!targetPlayer) return;

    auto entity = targetPlayer->GetEntity();
    if (!entity) return;

    if (m_Slot != -1) {
        if (m_Client->GetInventory()->GetSelectedHotbarSlot() != m_Slot) {
            m_Client->GetInventory()->SelectHotbarSlot(m_Slot);
            return;
        }
    }

    if (Attack(entity)) {
        std::wcout << L"Attacking " << targetPlayer->GetName() << " with slot " << m_Slot << std::endl;
        s_LastGlobalAttack = util::GetTime();
        m_LastAttack = util::GetTime();
    }
}

s64 AttackAction::s_LastGlobalAttack = 0;
s64 AttackAction::s_GlobalAttackCooldown = 600;


bool MeleeAction::Attack(mc::entity::EntityPtr entity) {
    auto physics = GetActorComponent(m_Client, PhysicsComponent);

    Vector3d botPos = physics->GetPosition();
    Vector3d targetPos = entity->GetPosition();

    if ((targetPos - botPos).LengthSq() > AttackRangeSq) return false;

    using namespace mc::protocol::packets::out;

    // Send attack
    UseEntityPacket useEntityPacket(entity->GetEntityId(), UseEntityPacket::Action::Attack);
    m_Client->GetConnection()->SendPacket(&useEntityPacket);

    // Send arm swing
    AnimationPacket animationPacket(mc::Hand::Main);
    m_Client->GetConnection()->SendPacket(&animationPacket);

    return true;
}
