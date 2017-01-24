#include "Actor.h"

void Actor::Update(double dt) {
    for (auto entry : m_Components)
        entry.second->Update(dt);
}

void Actor::AddComponent(ComponentPtr component) {
    m_Components.insert(std::make_pair(component->GetId(), component));
}

void Actor::RemoveComponent(ComponentId id) {
    auto iter = m_Components.find(id);
    m_Components.erase(iter);
}
