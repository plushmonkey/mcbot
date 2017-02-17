#ifndef MCBOT_CHI_ACTIONS_ABILITYACTION_H_
#define MCBOT_CHI_ACTIONS_ABILITYACTION_H_

#include "AttackAction.h"

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

    void Attack(Minecraft::EntityPtr entity) override;
};

class LungeAction : public MeleeAction {
public:
    LungeAction(GameClient* client, s64 delay, s32 slot)
        : MeleeAction(client, delay, slot)
    {

    }

    bool ShouldUse();

    void Attack(Minecraft::EntityPtr entity) override;
};

class JoinArenaAction : public DecisionAction {
private:
    GameClient* m_Client;
    Vector3i m_SignPosition;
    Vector3i m_StandPosition;
    bool m_FoundSign;
    s64 m_LastClick;
    bool m_MysticEmpire;

    Vector3d GetSignNormal();

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
