#ifndef MCBOT_BOTUPDATE_H_
#define MCBOT_BOTUPDATE_H_

#include "GameClient.h"
#include "PlayerList.h"
#include "Pathfinder.h"
#include "Decision.h"
#include "Component.h"

class BotUpdate : public mc::core::ClientListener {
private:
    GameClient* m_Client;
    PlayerList m_Players;
    s64 m_StartupTime;

    std::unique_ptr<Pathfinder> m_Pathfinder;

    DecisionTreeNodePtr m_DecisionTree;

public:
    BotUpdate(GameClient* client);
    ~BotUpdate();

    BotUpdate(const BotUpdate& rhs) = delete;
    BotUpdate& operator=(const BotUpdate& rhs) = delete;
    BotUpdate(BotUpdate&& rhs) = delete;
    BotUpdate& operator=(BotUpdate&& rhs) = delete;

    GameClient* GetClient() noexcept { return m_Client; }

    void OnTick() override;

    void AddComponent(std::unique_ptr<Component> component);
    void SetDecisionTree(DecisionTreeNodePtr tree) noexcept;
};

#endif
