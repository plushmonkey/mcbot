#include "ChiBot.h"
#include "../GameClient.h"
#include "../components/PhysicsComponent.h"
#include "../components/SynchronizationComponent.h"
#include "../components/SpeedComponent.h"
#include "../components/JumpComponent.h"
#include "../components/TargetingComponent.h"
#include "components/StuckComponent.h"
#include "../components/EffectComponent.h"
#include "actions/AbilityAction.h"
#include "actions/AttackAction.h"
#include "../actions/WanderAction.h"

using namespace chi;

void ChiBot::RegisterComponents(BotUpdate* update) {
    GameClient* client = update->GetClient();

    auto jump = std::make_shared<JumpComponent>(client->GetWorld(), 200);
    update->AddComponent(jump);

    auto targeting = std::make_shared<TargetingComponent>();
    update->AddComponent(targeting);

    auto stuck = std::make_shared<StuckComponent>(client->GetDispatcher(), Vector3d(100, 0, 100), 5);
    update->AddComponent(stuck);

    auto effect = std::make_shared<EffectComponent>(client->GetDispatcher());
    update->AddComponent(effect);
}

void ChiBot::CreateDecisionTree(BotUpdate* update, bool mystic) {
    GameClient* client = update->GetClient();

    auto joinArenaAction = std::make_shared<JoinArenaAction>(client, mystic);
    auto findTarget = std::make_shared<FindTargetAction>(client);

    auto paralyze = std::make_shared<MeleeAction>(client, 10100, 0);
    auto lunge = std::make_shared<LungeAction>(client, 6100, 1);
    auto quickstrike = std::make_shared<MeleeAction>(client, 0, 2);
    auto backstab = std::make_shared<BackstabAction>(client, 8600, 3);
    auto rapidpunch = std::make_shared<MeleeAction>(client, 6100, 4);
    //auto daggerthrow = std::make_shared<MeleeAction>(client, 0, 5);
    auto swiftkick = std::make_shared<SwiftKickAction>(client, 4100, 6);
    auto warrior = std::make_shared<StanceAction>(client, 1000, 7, Effect::Resistance, -1);
    auto acrobat = std::make_shared<StanceAction>(client, 1000, 8, Effect::Speed, 2);

    auto quickstrikeDecision = std::make_shared<BooleanDecision>(
        quickstrike, findTarget, std::bind(&MeleeAction::CanAttack, quickstrike)
    );
    auto rapidpunchDecision = std::make_shared<BooleanDecision>(
        rapidpunch, quickstrikeDecision, std::bind(&MeleeAction::CanAttack, rapidpunch)
    );
    auto swiftkickDecision = std::make_shared<BooleanDecision>(
        swiftkick, rapidpunchDecision, std::bind(&SwiftKickAction::CanAttack, swiftkick)
    );
    auto backstabDecision = std::make_shared<BooleanDecision>(
        backstab, swiftkickDecision, std::bind(&BackstabAction::CanAttack, backstab)
    );
    auto paralyzeDecision = std::make_shared<BooleanDecision>(
        paralyze, backstabDecision, std::bind(&MeleeAction::CanAttack, paralyze)
    );

    auto lungeDecision = std::make_shared<BooleanDecision>(
        lunge, findTarget, std::bind(&LungeAction::ShouldUse, lunge)
    );

    auto meleeDecision = std::make_shared<RangeDecision<double>>(
        paralyzeDecision, lungeDecision, std::bind(&FindTargetAction::DistanceToTarget, findTarget), 0.0, 3
    );

    auto warriorDecision = std::make_shared<BooleanDecision>(
        warrior, meleeDecision, std::bind(&StanceAction::ShouldUse, warrior)
    );

    auto acroDecision = std::make_shared<BooleanDecision>(
        acrobat, meleeDecision, std::bind(&StanceAction::ShouldUse, acrobat)
    );

    auto stanceDecision = std::make_shared<RangeDecision<double>>(
        warriorDecision, acroDecision, std::bind(&FindTargetAction::DistanceToTarget, findTarget), 0.0, 9
    );

    DecisionTreeNodePtr decisionTree = std::make_shared<BooleanDecision>(
        joinArenaAction, stanceDecision, std::bind(&JoinArenaAction::IsNearSign, joinArenaAction.get())
    );

    update->SetDecisionTree(decisionTree);
}

void ChiBot::Cleanup(BotUpdate* update) {
    GameClient* client = update->GetClient();

    client->RemoveComponent(Component::GetIdFromName(JumpComponent::name));
    client->RemoveComponent(Component::GetIdFromName(TargetingComponent::name));
    client->RemoveComponent(Component::GetIdFromName(StuckComponent::name));
    client->RemoveComponent(Component::GetIdFromName(EffectComponent::name));
}
