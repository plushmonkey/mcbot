#include "EffectComponent.h"
#include "../GameClient.h"
#include <iostream>

const char* EffectComponent::name = "Effect";

void EffectComponent::Update(double dt) {
    for (auto iter = m_Effects.begin(); iter != m_Effects.end(); ++iter) {
        iter->second.duration -= dt;
    }
}

void EffectComponent::HandlePacket(mc::protocol::packets::in::EntityEffectPacket* packet) {
    GameClient* client = dynamic_cast<GameClient*>(m_Owner);
    if (client == nullptr) return;
    if (client->GetEntityManager()->GetPlayerEntity()->GetEntityId() != packet->GetEntityId()) return;

    Effect effect = static_cast<Effect>(packet->GetEffectId());

    EffectData data;
    data.amplifier = packet->GetAmplifier();
    data.duration = packet->GetDuration();

    m_Effects[effect] = data;
}

void EffectComponent::HandlePacket(mc::protocol::packets::in::RemoveEntityEffectPacket* packet) {
    GameClient* client = dynamic_cast<GameClient*>(m_Owner);
    if (client == nullptr) return;
    if (client->GetEntityManager()->GetPlayerEntity()->GetEntityId() != packet->GetEntityId()) return;

    Effect effect = static_cast<Effect>(packet->GetEffectId());

    m_Effects.erase(effect);
}

void EffectComponent::HandlePacket(mc::protocol::packets::in::UpdateHealthPacket* packet) {
    if (packet->GetHealth() <= 0) {
        m_Effects.clear();
    }
}

bool EffectComponent::HasEffect(Effect effect) const {
    auto iter = m_Effects.find(effect);

    return iter != m_Effects.end();
}

const EffectComponent::EffectData* EffectComponent::GetEffectData(Effect effect) const {
    auto iter = m_Effects.find(effect);
    if (iter == m_Effects.end()) return nullptr;
    return &iter->second;
}
