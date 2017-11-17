#include "BotUpdate.h"
#include "Utility.h"
#include "components/SynchronizationComponent.h"
#include "components/SpeedComponent.h"
#include "components/PhysicsComponent.h"

using mc::Vector3d;

BotUpdate::BotUpdate(GameClient* client)
    : m_Client(client),
      m_Players(client)
{
    client->RegisterListener(this);

    auto sync = std::make_unique<SynchronizationComponent>(client->GetDispatcher(), client->GetConnection(), client->GetPlayerManager());
    AddComponent(std::move(sync));

    auto physics = std::make_unique<PhysicsComponent>(client->GetWorld(), mc::AABB(Vector3d(-0.3, 0.0, -0.3), Vector3d(0.3, 1.8, 0.3)));
    physics->SetMaxAcceleration(100.0f);
    physics->SetMaxRotation(3.14159 * 8);
    AddComponent(std::move(physics));

    auto speed = std::make_unique<SpeedComponent>(client->GetConnection(), client->GetWorld());
    speed->SetMovementType(SpeedComponent::Movement::Normal);
    AddComponent(std::move(speed));

    m_StartupTime = util::GetTime();
    m_Pathfinder = std::make_unique<Pathfinder>(m_Client);
}

BotUpdate::~BotUpdate() {
    m_Client->RemoveComponent(Component::GetIdFromName(SynchronizationComponent::name));
    m_Client->RemoveComponent(Component::GetIdFromName(SpeedComponent::name));
    m_Client->RemoveComponent(Component::GetIdFromName(PhysicsComponent::name));
    m_Client->UnregisterListener(this);
}

void BotUpdate::AddComponent(std::unique_ptr<Component> component) {
    component->SetOwner(m_Client);
    m_Client->AddComponent(std::move(component));
}

void BotUpdate::SetDecisionTree(DecisionTreeNodePtr tree) noexcept {
    m_DecisionTree = tree;
}

void BotUpdate::OnTick() {
    auto sync = GetActorComponent(m_Client, SynchronizationComponent);
    if (!sync || !sync->HasSpawned()) return;

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
