#ifndef MCBOT_ACTOR_H_
#define MCBOT_ACTOR_H_


#include "Component.h"

class Actor {
public:
    typedef std::map<ComponentId, ComponentPtr> ActorComponents;

private:
    ActorComponents m_Components;

public:
    virtual ~Actor() { }

    void Update(double dt);

    template <class ComponentType>
    std::weak_ptr<ComponentType> GetComponent(ComponentId id)
    {
        ActorComponents::iterator find = m_Components.find(id);

        if (find == m_Components.end()) return std::weak_ptr<ComponentType>();

        ComponentPtr componentBase(find->second);
        std::shared_ptr<ComponentType> component(std::static_pointer_cast<ComponentType>(componentBase));
        return std::weak_ptr<ComponentType>(component);
    }

    template <class ComponentType>
    std::weak_ptr<ComponentType> GetComponent(const char* name) {
        ComponentId id = Component::GetIdFromName(name);
        return GetComponent<ComponentType>(id);
    }

    const ActorComponents& GetComponents() { return m_Components; }
    void AddComponent(ComponentPtr component);
    void RemoveComponent(ComponentId id);
};

#endif
