#ifndef MCBOT_EFFECT_COMPONENT_H_
#define MCBOT_EFFECT_COMPONENT_H_

#include "../../Component.h"
#include <mclib/Common.h>
#include <mclib/Packets/PacketHandler.h>
#include <mclib/Packets/PacketDispatcher.h>

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

class EffectComponent : public Component, Minecraft::Packets::PacketHandler {
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
    EffectComponent(Minecraft::Packets::PacketDispatcher* dispatcher)
        : Minecraft::Packets::PacketHandler(dispatcher)
    {
        dispatcher->RegisterHandler(Minecraft::Protocol::State::Play, Minecraft::Protocol::Play::EntityEffect, this);
        dispatcher->RegisterHandler(Minecraft::Protocol::State::Play, Minecraft::Protocol::Play::RemoveEntityEffect, this);
        dispatcher->RegisterHandler(Minecraft::Protocol::State::Play, Minecraft::Protocol::Play::UpdateHealth, this);
    }

    ~EffectComponent() {
        GetDispatcher()->UnregisterHandler(this);
    }

    void Update(double dt);
    void HandlePacket(Minecraft::Packets::Inbound::EntityEffectPacket* packet) override;
    void HandlePacket(Minecraft::Packets::Inbound::RemoveEntityEffectPacket* packet) override;
    void HandlePacket(Minecraft::Packets::Inbound::UpdateHealthPacket* packet) override;

    bool HasEffect(Effect effect) const;
    const EffectData* GetEffectData(Effect effect) const;

    const char* GetName() const { return name; }
};

#endif
