#ifndef CHI_WANDER_ACTION_H_
#define CHI_WANDER_ACTION_H_

#include "../Decision.h"
#include "../GameClient.h"
#include "../components/PhysicsComponent.h"
#include "../components/TargetingComponent.h"

class WanderAction : public DecisionAction {
private:
    GameClient* m_Client;
    ai::WanderSteering m_Wander;

public:
    WanderAction(GameClient* client)
        : m_Client(client),
        m_Wander(client, 15.0, 2.0, 0.15)
    {

    }
    void Act() override {
        auto physics = GetActorComponent(m_Client, PhysicsComponent);
        if (!physics) return;

        auto targeting = GetActorComponent(m_Client, TargetingComponent);
        if (!targeting) return;

        ai::SteeringAcceleration steering = m_Wander.GetSteering();

        physics->ApplyAcceleration(steering.movement);
        physics->ApplyRotation(steering.rotation);

        Vector3d pos = physics->GetPosition();
        Vector3d vel = physics->GetVelocity();

        Vector3d projectedPos = pos + (vel * 50.0 / 1000.0);

        Minecraft::BlockPtr projectedBlock = m_Client->GetWorld()->GetBlock(projectedPos).GetBlock();
        Minecraft::BlockPtr block = m_Client->GetWorld()->GetBlock(projectedPos - Vector3d(0, 1, 0)).GetBlock();

        if (!projectedBlock || projectedBlock->IsSolid() || !block || !block->IsSolid()) {
            physics->SetVelocity(-vel * 1.5);
            physics->ApplyRotation(3.14159f);
        }

    }
};

#endif
