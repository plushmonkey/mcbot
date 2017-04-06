#ifndef CHI_ATTACK_ACTION_H_
#define CHI_ATTACK_ACTION_H_

#include "../../Decision.h"
#include "../../GameClient.h"

class AttackAction : public DecisionAction {
protected:
    GameClient* m_Client;
    s64 m_AttackCooldown;
    s64 m_LastAttack;
    s32 m_Slot;

    static s64 s_LastGlobalAttack;
    static s64 s_GlobalAttackCooldown;

    bool SelectItem(s32 id);

public:
    AttackAction(GameClient* client, s64 attackCooldown, s32 slot) : m_Client(client), m_LastAttack(0), m_AttackCooldown(attackCooldown), m_Slot(slot) { }

    virtual bool CanAttack();

    void Act() override;

    virtual bool Attack(mc::entity::EntityPtr entity) = 0;
};

class MeleeAction : public AttackAction {
private:
    const double AttackRangeSq = 3 * 3;

public:
    MeleeAction(GameClient* client, s64 delay, s32 slot)
        : AttackAction(client, delay, slot)
    {

    }

    virtual bool Attack(mc::entity::EntityPtr entity);
};

#endif
