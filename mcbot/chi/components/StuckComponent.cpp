#include "StuckComponent.h"
#include "../../components/PhysicsComponent.h"
#include "../../Utility.h"
#include "../../Actor.h"
#include "../../Math.h"

#include <iostream>
#include <random>

const char* StuckComponent::name = "Stuck";

namespace {

const s64 ResetDelay = 1000;
std::random_device dev;
std::mt19937 engine(dev());

}

void StuckComponent::Update(double dt) {
    s64 time = util::GetTime();

    if (time >= m_LastReset + ResetDelay) {
        m_LastReset = time;
        m_Corrections = 0;
    }
}

void StuckComponent::HandlePacket(mc::protocol::packets::in::PlayerPositionAndLookPacket* packet) {
    m_Corrections++;
    if (m_Corrections >= m_Threshold) {
        auto physics = GetActorComponent(m_Owner, PhysicsComponent);
        if (physics) {
            double x = m_MaxAcceleration.x * RandomBinomial(engine);
            double z = m_MaxAcceleration.z * RandomBinomial(engine);

            physics->ApplyAcceleration(mc::Vector3d(x, 0, z));
        }
    }
}
