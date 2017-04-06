#ifndef MCBOT_CHI_ACTIONS_ABILITYACTION_H_
#define MCBOT_CHI_ACTIONS_ABILITYACTION_H_

#include "AttackAction.h"
#include "../../components/EffectComponent.h"

class BackstabAction : public MeleeAction {
public:
    BackstabAction(GameClient* client, s64 delay, s32 slot)
        : MeleeAction(client, delay, slot)
    {

    }

    bool CanAttack();
};

class SwiftKickAction : public MeleeAction {
public:
    SwiftKickAction(GameClient* client, s64 delay, s32 slot)
        : MeleeAction(client, delay, slot)
    {

    }

    bool Attack(mc::entity::EntityPtr entity) override;
};

class LungeAction : public MeleeAction {
public:
    LungeAction(GameClient* client, s64 delay, s32 slot)
        : MeleeAction(client, delay, slot)
    {

    }

    bool ShouldUse();

    bool Attack(mc::entity::EntityPtr entity) override;
};

class StanceAction : public MeleeAction {
private:
    s64 m_ActivationTime;
    Effect m_CheckEffect;
    int m_EffectLevel;

public:
    StanceAction(GameClient* client, s64 delay, s32 slot, Effect checkEffect, int effectLevel)
        : MeleeAction(client, delay, slot), 
          m_ActivationTime(0),
          m_CheckEffect(checkEffect),
          m_EffectLevel(effectLevel)
    {

    }

    bool ShouldUse();

    bool Attack(mc::entity::EntityPtr entity) override;
};

class JoinArenaAction : public DecisionAction {
private:
    GameClient* m_Client;
    mc::Vector3i m_SignPosition;
    mc::Vector3i m_StandPosition;
    bool m_FoundSign;
    s64 m_LastClick;
    bool m_MysticEmpire;

    mc::Vector3d GetSignNormal();

    bool FindSign();

public:
    JoinArenaAction(GameClient* client, bool mystic = false) : m_Client(client), m_FoundSign(false), m_LastClick(0), m_MysticEmpire(mystic) { }

    bool IsNearSign();

    void Act() override;
};

class FindTargetAction : public DecisionAction {
private:
    GameClient* m_Client;

public:
    FindTargetAction(GameClient* client) : m_Client(client) { }

    void Act() override;

    double DistanceToTarget();
};

#endif
