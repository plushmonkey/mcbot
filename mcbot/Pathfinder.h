#ifndef MCBOT_PATHFINDER_H_
#define MCBOT_PATHFINDER_H_

#include "GameClient.h"
#include "Collision.h"
#include "Pathing.h"

#include <vector>

Vector3i GetGroundLevel(Minecraft::World* world, Vector3i pos);

struct CastResult {
    std::vector<Vector3i> hit;
    std::size_t length;
    bool full;
};

CastResult RayCast(Minecraft::World* world, std::shared_ptr<WorldGraph> graph, const Vector3d& from, Vector3d direction, std::size_t length);

class Pathfinder {
private:
    GameClient* m_Client;
    std::shared_ptr<ai::path::Plan> m_Plan;
    CollisionDetector m_CollisionDetector;

    bool IsNearBlocks(Vector3d pos);
    void SmoothPath();

public:
    Pathfinder(GameClient* client);

    void Update();
};

#endif
