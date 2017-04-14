#ifndef MCBOT_COMPONENT_H_
#define MCBOT_COMPONENT_H_

#include <string>
#include <map>
#include <memory>

typedef std::size_t ComponentId;

class Actor;

#define GetActorComponent(actor, type) actor->GetComponent<type>(type::name)

class Component {
protected:
    Actor* m_Owner;

public:
    Component() = default;
    virtual ~Component() { }

    Component(const Component& rhs) = delete;
    Component& operator=(const Component& rhs) = delete;
    Component(Component&& rhs) = delete;
    Component& operator=(Component&& rhs) = delete;

    void SetOwner(Actor* owner) noexcept { m_Owner = owner; }
    
    virtual void Update(double dt) { }
    ComponentId GetId() const { return GetIdFromName(GetName()); }
    virtual const char* GetName() const = 0;

    static ComponentId GetIdFromName(const char* name) {
        return std::hash<std::string>()(name);
    }
};

#endif
