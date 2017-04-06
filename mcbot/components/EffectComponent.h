#ifndef MCBOT_EFFECT_COMPONENT_H_
#define MCBOT_EFFECT_COMPONENT_H_

#include "../Component.h"
#include <mclib/common/Common.h>
#include <mclib/protocol/packets/PacketDispatcher.h>
#include <mclib/protocol/packets/PacketHandler.h>

enum class Effect {
    Speed = 1,
    Slowness,
    Haste,
    MiningFatigue,
    Strength,
    InstantHealth,
    InstantDamage,
    JumpBoost,
    Nausea,
    Regeneration,
    Resistance,
    FireResistance,
    WaterBreathing,
    Invisibility,
    Blindness,
    NightVision,
    Hunger,
    Weakness,
    Poison,
    Wither,
    HealthBoost,
    Absorption,
    Saturation,
    Glowing,
    Levitation,
    Luck,
    BadLuck
};

class EffectComponent : public Component, mc::protocol::packets::PacketHandler {
public:
    struct EffectData {
        double duration;
        s32 amplifier;
        bool remove;
    };

    static const char* name;

private:
    std::map<Effect, EffectData> m_Effects;

public:
    EffectComponent(mc::protocol::packets::PacketDispatcher* dispatcher)
        : mc::protocol::packets::PacketHandler(dispatcher)
    {
        dispatcher->RegisterHandler(mc::protocol::State::Play, mc::protocol::play::EntityEffect, this);
        dispatcher->RegisterHandler(mc::protocol::State::Play, mc::protocol::play::RemoveEntityEffect, this);
        dispatcher->RegisterHandler(mc::protocol::State::Play, mc::protocol::play::UpdateHealth, this);
    }

    ~EffectComponent() {
        GetDispatcher()->UnregisterHandler(this);
    }

    void Update(double dt);
    void HandlePacket(mc::protocol::packets::in::EntityEffectPacket* packet) override;
    void HandlePacket(mc::protocol::packets::in::RemoveEntityEffectPacket* packet) override;
    void HandlePacket(mc::protocol::packets::in::UpdateHealthPacket* packet) override;

    bool HasEffect(Effect effect) const;
    const EffectData* GetEffectData(Effect effect) const;

    const char* GetName() const { return name; }
};

#endif
