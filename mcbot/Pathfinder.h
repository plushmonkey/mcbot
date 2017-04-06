#ifndef MCBOT_PATHFINDER_H_
#define MCBOT_PATHFINDER_H_

#include "GameClient.h"
#include "Collision.h"
#include "Pathing.h"

#include <vector>

mc::Vector3i GetGroundLevel(mc::world::World* world, mc::Vector3i pos);

struct CastResult {
    std::vector<mc::Vector3i> hit;
    std::size_t length;
    bool full;
};

CastResult RayCast(mc::world::World* world, std::shared_ptr<WorldGraph> graph, const mc::Vector3d& from, mc::Vector3d direction, std::size_t length);

class Pathfinder {
private:
    GameClient* m_Client;
    std::shared_ptr<ai::path::Plan> m_Plan;
    CollisionDetector m_CollisionDetector;

    bool IsNearBlocks(mc::Vector3d pos);
    void SmoothPath();

public:
    Pathfinder(GameClient* client);

    void Update();
};

#endif
