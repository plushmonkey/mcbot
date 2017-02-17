#ifndef MCBOT_BOTUPDATE_H_
#define MCBOT_BOTUPDATE_H_

#include "GameClient.h"
#include "PlayerList.h"
#include "Pathfinder.h"
#include "Decision.h"
#include "Component.h"

class BotUpdate : public ClientListener {
private:
    GameClient* m_Client;
    PlayerList m_Players;
    s64 m_StartupTime;

    std::shared_ptr<Pathfinder> m_Pathfinder;

    DecisionTreeNodePtr m_DecisionTree;

public:
    BotUpdate(GameClient* client);
    ~BotUpdate();

    GameClient* GetClient() { return m_Client; }

    void OnTick();

    void AddComponent(ComponentPtr component);
    void SetDecisionTree(DecisionTreeNodePtr tree);
};

#endif
