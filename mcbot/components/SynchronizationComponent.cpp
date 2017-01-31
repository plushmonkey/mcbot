#include "SynchronizationComponent.h"

#include "../Actor.h"
#include "PhysicsComponent.h"
#include "../GameClient.h"
#include <iostream>

const char* SynchronizationComponent::name = "Synchronization";

void SynchronizationComponent::OnClientSpawn(Minecraft::PlayerPtr player) {
    auto physics = GetActorComponent(m_Owner, PhysicsComponent);
    if (physics == nullptr) return;

    Vector3d oldPos = physics->GetPosition();
    physics->SetOrientation(player->GetEntity()->GetYaw());
    physics->SetPosition(player->GetEntity()->GetPosition());
    physics->SetVelocity(Vector3d(0, 0, 0));

    m_Spawned = true;
    std::cout << "Position corrected to " << physics->GetPosition() << " from " << oldPos << std::endl;
}

void SynchronizationComponent::HandlePacket(Minecraft::Packets::Inbound::UpdateHealthPacket* packet) {
    float health = packet->GetHealth();

    if (health <= 0) {
        std::cout << "Died." << std::endl;
        m_Dead = true;
    }
}

void SynchronizationComponent::HandlePacket(Minecraft::Packets::Inbound::EntityVelocityPacket* packet) {
    auto physics = GetActorComponent(m_Owner, PhysicsComponent);
    if (physics == nullptr) return;

    GameClient* client = dynamic_cast<GameClient*>(m_Owner);
    if (!client) return;

    Minecraft::EntityId eid = packet->GetEntityId();

    auto playerEntity = client->GetEntityManager()->GetPlayerEntity();
    if (playerEntity && playerEntity->GetEntityId() == eid) {
        Vector3d newVelocity = ToVector3d(packet->GetVelocity()) * 20.0 / 8000.0;
        std::cout << "Set velocity to " << newVelocity << std::endl;
        physics->SetVelocity(newVelocity);
    }
}

void SynchronizationComponent::HandlePacket(Minecraft::Packets::Inbound::SpawnPositionPacket* packet) {
    s64 x = packet->GetLocation().GetX();
    s64 y = packet->GetLocation().GetY();
    s64 z = packet->GetLocation().GetZ();

    m_SpawnPosition = Vector3i((s32)x, (s32)y, (s32)z);
    std::cout << "Set spawn position to " << m_SpawnPosition << std::endl;
}

void SynchronizationComponent::Update(double dt) {
    auto physics = GetActorComponent(m_Owner, PhysicsComponent);
    if (physics == nullptr || !m_Spawned) return;

    if (m_Dead) {
        std::cout << "Sending respawn packet. State: Alive" << std::endl;
        physics->SetPosition(ToVector3d(m_SpawnPosition));

        auto action = Minecraft::Packets::Outbound::ClientStatusPacket::Action::PerformRespawn;
        Minecraft::Packets::Outbound::ClientStatusPacket status(action);

        m_Connection->SendPacket(&status);

        m_Dead = false;
    }

    float pitch = 0.0f;

    // todo: Track position and orientation. Only send the larger packets when needed.
    Minecraft::Packets::Outbound::PlayerPositionAndLookPacket response(physics->GetPosition(),
        (float)physics->GetOrientation() * 180.0f / 3.14159f, pitch, true);

    m_Connection->SendPacket(&response);
}
