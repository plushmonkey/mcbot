#include "WorldGraph.h"

#include "GameClient.h"
#include "Utility.h"
#include "PhysicsComponent.h"
#include <iostream>

// Todo: rebuild graph when world changes. It will probably need to be async and updated infrequently.
ai::path::Node* WorldGraph::GetNode(Vector3i pos) {
    auto iter = m_Nodes.find(pos);

    ai::path::Node* node = nullptr;
    if (iter == m_Nodes.end()) {
        node = new ai::path::Node(pos);
        m_Nodes[pos] = node;
    } else {
        node = iter->second;
    }

    return node;
}

// The check pos is not solid, the block above is not solid, and the block below is solid
bool WorldGraph::IsWalkable(Vector3i pos) {
    Minecraft::World* world = m_Client->GetWorld();

    Minecraft::BlockPtr checkBlock = world->GetBlock(pos);

    Minecraft::BlockPtr aBlock = world->GetBlock(pos + Vector3i(0, 1, 0));
    Minecraft::BlockPtr bBlock = world->GetBlock(pos - Vector3i(0, 1, 0));

    return checkBlock && !checkBlock->IsSolid() && aBlock && !aBlock->IsSolid() && bBlock && bBlock->IsSolid();
}

int WorldGraph::IsSafeFall(Vector3i pos) {
    Minecraft::World* world = m_Client->GetWorld();

    Minecraft::BlockPtr checkBlock = world->GetBlock(pos);

    Minecraft::BlockPtr aBlock = world->GetBlock(pos + Vector3i(0, 1, 0));

    if (!checkBlock || checkBlock->IsSolid()) return 0;
    if (!aBlock || aBlock->IsSolid()) return 0;

    for (int i = 0; i < 4; ++i) {
        Minecraft::BlockPtr bBlock = world->GetBlock(pos - Vector3i(0, i + 1, 0));

        if (bBlock && bBlock->IsSolid()) return i + 1;
        //if (bBlock && bBlock->IsSolid() || (bBlock->GetType() == 8 || bBlock->GetType() == 9)) return i + 1;
    }

    return 0;
}

bool WorldGraph::IsWater(Vector3i pos) {
    Minecraft::World* world = m_Client->GetWorld();
    Minecraft::BlockPtr checkBlock = world->GetBlock(pos);

    return checkBlock && (checkBlock->GetType() == 8 || checkBlock->GetType() == 9);
}


WorldGraph::WorldGraph(GameClient* client)
    : m_Client(client),
    m_NeedsBuilt(false)
{
    client->GetWorld()->RegisterListener(this);
}

WorldGraph::~WorldGraph() {
    m_Client->GetWorld()->UnregisterListener(this);
}

void WorldGraph::OnChunkLoad(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, u16 yIndex) {
    m_NeedsBuilt = true;
}

void WorldGraph::BuildGraph() {
    if (!m_NeedsBuilt) return;
    m_NeedsBuilt = false;

    const int SearchRadius = 16 * 3;
    const int YSearchRadius = 16;
    
    std::shared_ptr<PhysicsComponent> physics = GetActorComponent(m_Client, PhysicsComponent);
    if (physics == nullptr) return;
    Vector3i position = ToVector3i(physics->GetPosition());

    static const std::vector<Vector3i> directions = {
        Vector3i(-1, 0, 0), Vector3i(1, 0, 0), Vector3i(0, 0, -1), Vector3i(0, 0, 1), // Directly nearby in flat area
        Vector3i(0, 1, 0), Vector3i(0, -1, 0), // Directly up/down
        Vector3i(-1, 1, 0), Vector3i(1, 1, 0), Vector3i(0, 1, -1), Vector3i(0, 1, 1), // Up one step
        Vector3i(-1, -1, 0), Vector3i(1, -1, 0), Vector3i(0, -1, -1), Vector3i(0, -1, 1) // Down one step
    };
    static const std::vector<Vector3i> waterDirections = {
        Vector3i(-1, 0, 0), Vector3i(1, 0, 0), Vector3i(0, 0, -1), Vector3i(0, 0, 1), // Directly nearby in flat area
        Vector3i(0, 1, 0), Vector3i(0, -1, 0), // Directly up/down
        Vector3i(-1, 1, 0), Vector3i(1, 1, 0), Vector3i(0, 1, -1), Vector3i(0, 1, 1),
    };

    this->Destroy();

    m_Edges.reserve(70000);

    std::cout << "Building graph\n";

    s64 startTime = util::GetTime();

    Minecraft::World* world = m_Client->GetWorld();
    for (int x = -SearchRadius; x < SearchRadius; ++x) {
        for (int y = -YSearchRadius; y < YSearchRadius; ++y) {
            int checkY = (int)position.y + y;
            if (checkY <= 0 || checkY >= 256) continue;
            for (int z = -SearchRadius; z < SearchRadius; ++z) {
                Vector3i checkPos = position + Vector3i(x, y, z);

                Minecraft::BlockPtr checkBlock = world->GetBlock(checkPos);

                // Skip because it's not loaded yet or it's solid
                if (!checkBlock || checkBlock->IsSolid()) continue;

                if (IsWalkable(checkPos)) {
                    for (Vector3i direction : directions) {
                        Vector3i neighborPos = checkPos + direction;

                        if (IsWalkable(neighborPos) || (IsSafeFall(neighborPos) && direction.y == 0)) {
                            ai::path::Node* current = GetNode(checkPos);
                            ai::path::Node* neighborNode = GetNode(neighborPos);

                            LinkNodes(current, neighborNode);
                        }
                    }
                } else if (IsWater(checkPos)) {
                    for (Vector3i direction : waterDirections) {
                        Vector3i neighborPos = checkPos + direction;

                        if (IsWalkable(neighborPos) || IsWater(neighborPos)) {
                            ai::path::Node* current = GetNode(checkPos);
                            ai::path::Node* neighborNode = GetNode(neighborPos);

                            LinkNodes(current, neighborNode, 4.0);
                            LinkNodes(neighborNode, current, 4.0);
                        }
                    }
                } else {
                    int fallDist = IsSafeFall(checkPos);
                    if (fallDist <= 0) continue;

                    ai::path::Node* current = GetNode(checkPos);

                    for (int i = 0; i < fallDist; ++i) {
                        Vector3i fallPos = checkPos - Vector3i(0, i + 1, 0);

                        Minecraft::BlockPtr block = world->GetBlock(fallPos);
                        if (!block || block->IsSolid()) break;

                        ai::path::Node* neighborNode = GetNode(fallPos);

                        LinkNodes(current, neighborNode);
                        current = neighborNode;
                    }
                }
            }
        }
    }

    std::cout << "Graph built in " << (util::GetTime() - startTime) << "ms.\n";
    std::cout << "Nodes: " << m_Nodes.size() << std::endl;
    std::cout << "Edges: " << m_Edges.size() << std::endl;
}
