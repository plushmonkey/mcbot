#include "SpeedComponent.h"

#include "../Actor.h"
#include "PhysicsComponent.h"
#include "EffectComponent.h"
#include "../GameClient.h"

const char* SpeedComponent::name = "Speed";

#undef max

namespace {

const float WalkingSpeed = 4.3f;
const float SprintingSpeed = 5.6f;
const float SneakingSpeed = 1.3f;
const float SwimSpeedSurface = 2.2f;
const float SwimSpeedSubmerged = 1.97f;

const u16 StillWater = 9;
const u16 FlowingWater = 8;

bool IsWater(Minecraft::World* world, Vector3d pos) {
    Minecraft::BlockPtr block = world->GetBlock(pos).GetBlock();

    return block && (block->GetType() == StillWater || block->GetType() == FlowingWater);
}

}

void SpeedComponent::Update(double dt) {
    auto physics = GetActorComponent(m_Owner, PhysicsComponent);
    if (physics == nullptr) return;

    auto effectComponent = GetActorComponent(m_Owner, EffectComponent);

    Vector3d pos = physics->GetPosition();

    if (IsWater(m_World, pos)) {
        if (IsWater(m_World, pos + Vector3d(0, 1, 0))) {
            physics->SetMaxSpeed(SwimSpeedSubmerged);
        } else {
            physics->SetMaxSpeed(SwimSpeedSurface);
        }
        return;
    }

    float maxSpeed = WalkingSpeed;

    switch (m_Movement) {
        case Movement::Normal:
            maxSpeed = WalkingSpeed;
        break;
        case Movement::Sneaking:
            maxSpeed = SneakingSpeed;
        break;
        case Movement::Sprinting:
            maxSpeed = SprintingSpeed;
        break;
    }

    if (effectComponent) {
        {
            const EffectComponent::EffectData* edata = effectComponent->GetEffectData(Effect::Speed);
            if (edata) {
                s8 level = (s8)edata->amplifier;
                maxSpeed *= (1.0f + (level + 1) * 0.2f);
            }
        }
        maxSpeed = std::max(0.0f, maxSpeed);

        {
            const EffectComponent::EffectData* edata = effectComponent->GetEffectData(Effect::Slowness);
            if (edata) {
                s8 level = (s8)edata->amplifier;
                maxSpeed *= (1.0f - (level + 1) * 0.15f);
            }
        }
        maxSpeed = std::max(0.0f, maxSpeed);
    }

    physics->SetMaxSpeed(maxSpeed);
}

void SpeedComponent::SetMovementType(Movement movement) {
    if (m_Movement == movement) return;

    GameClient* client = dynamic_cast<GameClient*>(m_Owner);
    if (!client) {
        m_Movement = movement;
        return;
    }

    auto entity = client->GetEntityManager()->GetPlayerEntity();
    if (!entity) {
        m_Movement = movement;
        return;
    }

    using namespace Minecraft::Packets::Outbound;

    EntityActionPacket::Action action;
    if (movement == Movement::Normal) {
        if (m_Movement == Movement::Sneaking)
            action = EntityActionPacket::Action::StopSneak;
        else if (m_Movement == Movement::Sprinting)
            action = EntityActionPacket::Action::StopSprint;
    } else if (movement == Movement::Sneaking) {
        action = EntityActionPacket::Action::StartSneak;

        // Stop sprinting before sneaking
        if (m_Movement == Movement::Sprinting) {
            EntityActionPacket packet(entity->GetEntityId(), EntityActionPacket::Action::StopSprint);
            m_Connection->SendPacket(&packet);
        }
    } else if (movement == Movement::Sprinting) {
        action = EntityActionPacket::Action::StartSprint;

        // Stop sneaking before sprinting
        if (m_Movement == Movement::Sneaking) {
            EntityActionPacket packet(entity->GetEntityId(), EntityActionPacket::Action::StopSneak);
            m_Connection->SendPacket(&packet);
        }
    }

    EntityActionPacket packet(entity->GetEntityId(), action);
    m_Connection->SendPacket(&packet);

    m_Movement = movement;
}
