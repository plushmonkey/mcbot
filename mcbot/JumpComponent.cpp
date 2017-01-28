#include "JumpComponent.h"

#include "Actor.h"
#include "PhysicsComponent.h"
#include "Utility.h"

#include <iostream>

const char* JumpComponent::name = "Jump";
const s64 JumpCooldown = 500;

void JumpComponent::Update(double dt) {
    auto physics = GetActorComponent(m_Owner, PhysicsComponent);
    if (!physics) return;

    s64 time = util::GetTime();

    bool onGround = m_CollisionDetector.DetectCollision(physics->GetPosition(), Vector3d(0, -0.01, 0), nullptr);

    if (onGround && m_Jump) {
        if (time >= m_LastJump + JumpCooldown) {
            physics->ApplyAcceleration(Vector3d(0, m_Power, 0));
            //physics->ClearHorizontalVelocity();
            m_LastJump = time;
        }
    }

    m_Jump = false;
}
