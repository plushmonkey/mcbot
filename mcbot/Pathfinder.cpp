#include "Pathfinder.h"
#include "WorldGraph.h"
#include "components/PhysicsComponent.h"
#include "components/TargetingComponent.h"
#include "Utility.h"

#include <iostream>

Vector3i GetGroundLevel(Minecraft::World* world, Vector3i pos) {
    s32 y;
    for (y = (s32)pos.y; y >= 0; --y) {
        Minecraft::BlockPtr block = world->GetBlock(Vector3i(pos.x, y, pos.z)).GetBlock();

        if (block && block->IsSolid()) break;
    }

    return Vector3i(pos.x, y + 1, pos.z);
}

CastResult RayCast(Minecraft::World* world, std::shared_ptr<WorldGraph> graph, const Vector3d& from, Vector3d direction, std::size_t length) {
    CastResult result;

    std::vector<Vector3i> hit(length);

    direction.Normalize();
    result.length = length;
    result.full = true;

    for (std::size_t i = 0; i < length; ++i) {
        Vector3i check = ToVector3i(from + (direction * i));
        Minecraft::BlockPtr block = world->GetBlock(check).GetBlock();
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
    Minecraft::World* world = m_Client->GetWorld();
    static const std::vector<Vector3d> directions = {
        Vector3d(-1, 0, 0), Vector3d(1, 0, 0), Vector3d(0, 0, -1), Vector3d(0, 0, 1)
    };

    for (Vector3d dir : directions) {
        Minecraft::BlockPtr block = world->GetBlock(pos + dir).GetBlock();

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
        Vector3d from = ToVector3d(nodes[index]->GetPosition());
        Vector3d to = ToVector3d(output.back()->GetPosition());
        Vector3d direction = to - from;
        std::size_t length = (std::size_t)direction.Length() + 1;
        direction.Normalize();

        if (from.y != to.y || IsNearBlocks(from) || IsNearBlocks(to)) {
            output.push_back(nodes[index - 1]);
        } else {
            CastResult result = RayCast(m_Client->GetWorld(), m_Client->GetGraph(), from, direction, length);
            if (!result.full) {
                output.push_back(nodes[index - 1]);
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
        Vector3d accel = -physics->GetVelocity() * 20;
        accel.y = 0;
        physics->ApplyAcceleration(accel);
        return;
    }

    Vector3i targetGroundPos = GetGroundLevel(m_Client->GetWorld(), target);
    Vector3i currentGroundPos = GetGroundLevel(m_Client->GetWorld(), ToVector3i(physics->GetPosition()));

    if (m_Plan == nullptr || !m_Plan->HasNext() || m_Plan->GetGoal()->GetPosition() != targetGroundPos) {
        s64 startTime = util::GetTime();

        m_Plan = m_Client->GetGraph()->FindPath(currentGroundPos, targetGroundPos);

        SmoothPath();

        //std::cout << "Plan built in " << (util::GetTime() - startTime) << "ms.\n";
    }

    if (!m_Plan || m_Plan->GetSize() == 0) {
        std::cout << "No plan to " << targetGroundPos << std::endl;

        Vector3d accel = -physics->GetVelocity() * 20;
        accel.y = 0;
        physics->ApplyAcceleration(accel);
    }

    Vector3d position = physics->GetPosition();
    Vector3d alignTarget = ToVector3d(target);

    ai::PathFollowSteering path(m_Client, m_Plan, 0.25);
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
