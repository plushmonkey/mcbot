#include "BotUpdate.h"
#include "Utility.h"
#include "components/SynchronizationComponent.h"
#include "components/SpeedComponent.h"
#include "components/PhysicsComponent.h"

BotUpdate::BotUpdate(GameClient* client)
    : m_Client(client),
      m_Players(client)
{
    client->RegisterListener(this);

    auto sync = std::make_shared<SynchronizationComponent>(client->GetDispatcher(), client->GetConnection(), client->GetPlayerManager());
    AddComponent(sync);

    auto physics = std::make_shared<PhysicsComponent>(client->GetWorld(), AABB(Vector3d(-0.3, 0.0, -0.3), Vector3d(0.3, 1.8, 0.3)));
    physics->SetMaxAcceleration(100.0f);
    physics->SetMaxRotation(3.14159 * 8);
    AddComponent(physics);

    auto speed = std::make_shared<SpeedComponent>(client->GetConnection(), client->GetWorld());
    speed->SetMovementType(SpeedComponent::Movement::Normal);
    AddComponent(speed);

    m_StartupTime = util::GetTime();
    m_Pathfinder = std::make_shared<Pathfinder>(m_Client);
}

BotUpdate::~BotUpdate() {
    m_Client->RemoveComponent(Component::GetIdFromName(SynchronizationComponent::name));
    m_Client->RemoveComponent(Component::GetIdFromName(SpeedComponent::name));
    m_Client->RemoveComponent(Component::GetIdFromName(PhysicsComponent::name));
    m_Client->UnregisterListener(this);
}

void BotUpdate::AddComponent(ComponentPtr component) {
    component->SetOwner(m_Client);
    m_Client->AddComponent(component);
}

void BotUpdate::SetDecisionTree(DecisionTreeNodePtr tree) {
    m_DecisionTree = tree;
}

void BotUpdate::OnTick() {
    auto sync = GetActorComponent(m_Client, SynchronizationComponent);
    if (!sync || !sync->HasSpawned()) return;

    auto speed = GetActorComponent(m_Client, SpeedComponent);
    if (speed) {
        if (speed->GetMovementType() != SpeedComponent::Movement::Sprinting)
            speed->SetMovementType(SpeedComponent::Movement::Sprinting);
    }

    if (!m_DecisionTree) {
        std::cerr << "No decision tree" << std::endl;
        return;
    }

    DecisionAction* action = m_DecisionTree->Decide();
    if (action)
        action->Act();

    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    if (!physics) return;

    m_Pathfinder->Update();
    physics->Integrate(50.0 / 1000.0);
}
