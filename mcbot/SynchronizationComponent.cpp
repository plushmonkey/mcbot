#include "SynchronizationComponent.h"

#include "Actor.h"
#include "PhysicsComponent.h"
#include <iostream>
const char* SynchronizationComponent::name = "Synchronization";

void SynchronizationComponent::OnClientSpawn(Minecraft::PlayerPtr player) {
    auto physics = GetActorComponent(m_Owner, PhysicsComponent);
    if (physics == nullptr) return;

    physics->SetOrientation(player->GetEntity()->GetYaw());
    physics->SetPosition(player->GetEntity()->GetPosition());
    m_Spawned = true;
}

void SynchronizationComponent::Update(double dt) {
    auto physics = GetActorComponent(m_Owner, PhysicsComponent);
    if (physics == nullptr || !m_Spawned) return;

    Vector3d pos = physics->GetPosition();

    float pitch = 0.0f;

    Minecraft::Packets::Outbound::PlayerPositionAndLookPacket response(pos,
        (float)physics->GetOrientation() * 180.0f / 3.14159f, pitch, true);

    m_Connection->SendPacket(&response);
}
