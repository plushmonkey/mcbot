#include "Pathfinder.h"
#include "WorldGraph.h"
#include "components/PhysicsComponent.h"
#include "components/SpeedComponent.h"
#include "components/TargetingComponent.h"
#include "Utility.h"

#include <iostream>

using mc::Vector3i;
using mc::Vector3d;

Vector3i GetGroundLevel(mc::world::World* world, Vector3i pos) {
    s32 y;
    for (y = (s32)pos.y; y >= 0; --y) {
        mc::block::BlockPtr block = world->GetBlock(Vector3i(pos.x, y, pos.z)).GetBlock();

        if (block && block->IsSolid()) break;
    }

    return Vector3i(pos.x, y + 1, pos.z);
}

CastResult RayCast(mc::world::World* world, WorldGraph* graph, const Vector3d& from, Vector3d direction, std::size_t length) {
    CastResult result;

    std::vector<Vector3i> hit(length);

    direction.Normalize();
    result.length = length;
    result.full = true;

    for (std::size_t i = 0; i < length; ++i) {
        Vector3i check = ToVector3i(from + (direction * i));
        mc::block::BlockPtr block = world->GetBlock(check).GetBlock();
        bool walkable = graph->IsWalkable(check);
        if (walkable && block && !block->IsSolid()) {
            hit.push_back(check);
        } else {
            result.full = false;
            result.length = i;
            break;
        }
    }

    result.hit = hit;

    return result;
}


bool Pathfinder::IsNearBlocks(Vector3d pos) {
    mc::world::World* world = m_Client->GetWorld();
    static const std::vector<Vector3d> directions = {
        Vector3d(-1, 0, 0), Vector3d(1, 0, 0), Vector3d(0, 0, -1), Vector3d(0, 0, 1)
    };

    for (Vector3d dir : directions) {
        mc::block::BlockPtr block = world->GetBlock(pos + dir).GetBlock();

        if (!block || block->IsSolid()) return true;
    }
    return false;
}

void Pathfinder::SmoothPath() {
    if (!m_Plan) return;

    std::vector<ai::path::Node*>& nodes = m_Plan->GetNodes();
    if (nodes.size() <= 2) return;

    std::vector<ai::path::Node*> output;
    output.push_back(nodes[0]);
    std::size_t index = 2;

    while (index < nodes.size() - 1) {
        Vector3d from = ToVector3d(nodes[index]->GetPosition()) + Vector3d(0.5, 0, 0.5);
        Vector3d to = ToVector3d(output.back()->GetPosition()) + Vector3d(0.5, 0, 0.5);
        Vector3d direction = to - from;
        std::size_t length = (std::size_t)direction.Length() + 1;
        direction.Normalize();

        Vector3d next = ToVector3d(nodes[index + 1]->GetPosition()) + Vector3d(0.5, 0, 0.5);
        if (from == next + Vector3d(0, 1, 0)) {
            // Skip this one entirely because the block directly below is in the path and the bot will fall on it.
            // This fixes the bug where the bot has to jump in the air to reach the block after touching ground.
            index += 2;
            continue;
        }

        if (from.y != to.y || IsNearBlocks(from) || IsNearBlocks(to)) {
            output.push_back(nodes[index - 1]);
        } else {
            CastResult result = RayCast(m_Client->GetWorld(), m_Client->GetGraph(), from, direction, length);
            if (!result.full) {
                output.push_back(nodes[index - 1]);
            } else {
                Vector3d nextPos = ToVector3d(nodes[index + 1]->GetPosition()) + Vector3d(0.5, 0, 0.5);
                Vector3d nextDir = Vector3Normalize(nextPos - from);

                // Check to see if there's any falls in the path to the next node.
                // Add both of them to the output list if there is.
                // This makes it so it doesn't smooth around corners that are possible to fall off of.
                for (std::size_t i = 0; i < length * 10; ++i) {
                    Vector3d current = from + (nextDir / 10) * i;
                    Vector3d below = current - Vector3d(0, 1, 0);

                    auto blockState = m_Client->GetWorld()->GetBlock(below);
                    auto block = blockState.GetBlock();

                    if (block == nullptr || !block->IsSolid()) {
                        output.push_back(nodes[index - 1]);
                        output.push_back(nodes[index]);
                        break;
                    }
                }
            }
        }

        ++index;
    }

    output.push_back(nodes.back());

    nodes.clear();
    nodes.insert(nodes.end(), output.begin(), output.end());
}

Pathfinder::Pathfinder(GameClient* client)
    : m_Client(client), m_Plan(nullptr), m_CollisionDetector(m_Client->GetWorld())
{

}

void Pathfinder::Update() {
    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    if (!physics) return;

    auto targeting = GetActorComponent(m_Client, TargetingComponent);
    if (!targeting) return;

    Vector3i target = targeting->GetTarget();
    Vector3i toTarget = target - ToVector3i(physics->GetPosition());
    if (toTarget.LengthSq() <= 2.0 * 2.0) {
        physics->ClearHorizontalVelocity();
        return;
    }

    Vector3i targetGroundPos = GetGroundLevel(m_Client->GetWorld(), target);
    Vector3i currentGroundPos = GetGroundLevel(m_Client->GetWorld(), ToVector3i(physics->GetPosition()));

    if (m_Plan == nullptr || !m_Plan->HasNext() || m_Plan->GetGoal()->GetPosition() != targetGroundPos) {
        s64 startTime = util::GetTime();

        m_Plan = m_Client->GetGraph()->FindPath(currentGroundPos, targetGroundPos);

        SmoothPath();

        /*
        if (m_Plan) {
            for (std::size_t i = 0; i < m_Plan->GetSize(); ++i) {
                auto pos = (*m_Plan)[i]->GetPosition();

                std::cout << pos << std::endl;
            }
        }

        std::cout << "Plan built in " << (util::GetTime() - startTime) << "ms.\n";
        */
    }

    if (!m_Plan || m_Plan->GetSize() == 0) {
        std::cout << "No plan to " << targetGroundPos << std::endl;

        physics->ClearHorizontalVelocity();
    }

    Vector3d position = physics->GetPosition();
    Vector3d alignTarget = ToVector3d(target);

    if (m_Plan && m_Plan->GetCurrent() && ToVector3d(m_Plan->GetCurrent()->GetPosition()).DistanceSq(position) >= 5 * 5) {
        auto speed = GetActorComponent(m_Client, SpeedComponent);
        if (speed) {
            if (speed->GetMovementType() != SpeedComponent::Movement::Sprinting)
                speed->SetMovementType(SpeedComponent::Movement::Sprinting);
        }
    } else {
        auto speed = GetActorComponent(m_Client, SpeedComponent);
        if (speed) {
            if (speed->GetMovementType() != SpeedComponent::Movement::Normal)
                speed->SetMovementType(SpeedComponent::Movement::Normal);
        }
    }

    ai::PathFollowSteering path(m_Client, m_Plan.get(), 0.25);
    ai::SteeringAcceleration pathSteering = path.GetSteering();
    physics->ApplyAcceleration(pathSteering.movement);
    physics->ApplyRotation(pathSteering.rotation);

    ai::ObstacleAvoidSteering avoidance(m_Client, physics->GetVelocity(), &m_CollisionDetector, 0.5, 2.0);
    ai::SteeringAcceleration avoidSteering = avoidance.GetSteering();
    physics->ApplyAcceleration(avoidSteering.movement);
    physics->ApplyRotation(avoidSteering.rotation);

    if (m_Plan && m_Plan->GetCurrent())
        alignTarget = (alignTarget + ToVector3d(m_Plan->GetCurrent()->GetPosition())) / 2.0;

    ai::FaceSteering align(m_Client, alignTarget, 0.1, 1, 1);
    physics->ApplyRotation(align.GetSteering().rotation);
}
