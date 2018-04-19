#ifndef MCBOT_ACTOR_H_
#define MCBOT_ACTOR_H_

#include "Component.h"

#include <memory>

class Actor {
public:
    typedef std::map<ComponentId, std::unique_ptr<Component>> ActorComponents;

private:
    ActorComponents m_Components;

public:
    Actor() = default;
    virtual ~Actor() { }

    Actor(const Actor& rhs) = delete;
    Actor& operator=(const Actor& rhs) = delete;
    Actor(Actor&& rhs) = delete;
    Actor& operator=(Actor&& rhs) = delete;

    void Update(double dt);

    template <class ComponentType>
    ComponentType* GetComponent(ComponentId id)
    {
        ActorComponents::iterator find = m_Components.find(id);

        if (find == m_Components.end()) return nullptr;

        return dynamic_cast<ComponentType*>(find->second.get());
    }

    template <class ComponentType>
    ComponentType* GetComponent(const char* name) {
        ComponentId id = Component::GetIdFromName(name);
        return GetComponent<ComponentType>(id);
    }

    const ActorComponents& GetComponents() { return m_Components; }
    void AddComponent(std::unique_ptr<Component> component);
    void RemoveComponent(ComponentId id);
};

#endif
