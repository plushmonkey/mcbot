#include "WorldGraph.h"

#include "GameClient.h"
#include "Utility.h"
#include "PhysicsComponent.h"
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>

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
bool WorldGraph::IsWalkable(Vector3i pos) const {
    Minecraft::World* world = m_Client->GetWorld();

    Minecraft::BlockPtr checkBlock = world->GetBlock(pos);

    Minecraft::BlockPtr aBlock = world->GetBlock(pos + Vector3i(0, 1, 0));
    Minecraft::BlockPtr bBlock = world->GetBlock(pos - Vector3i(0, 1, 0));

    return checkBlock && !checkBlock->IsSolid() && aBlock && !aBlock->IsSolid() && bBlock && bBlock->IsSolid();
}

int WorldGraph::IsSafeFall(Vector3i pos) const {
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

bool WorldGraph::IsWater(Vector3i pos) const {
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

void WorldGraph::OnBlockChange(Vector3i position, Minecraft::BlockPtr newBlock, Minecraft::BlockPtr oldBlock) {
    const s32 InvalidationRadius = 1;
    std::vector<ai::path::Node*> nodes;
    
    // Get all of the nodes surrounding the changed position
    for (s32 y = -InvalidationRadius; y <= InvalidationRadius; ++y) {
        for (s32 z = -InvalidationRadius; z <= InvalidationRadius; ++z) {
            for (s32 x = -InvalidationRadius; x <= InvalidationRadius; ++x) {
                Vector3i nodePos = position + Vector3i(x, y, z);

                if (nodePos.y < 0 || nodePos.y >= 256) continue;

                ai::path::Node* node = GetNode(nodePos);
                nodes.push_back(node);
            }
        }
    }

    // Invalidate all of the edges in the contained nodes
    for (ai::path::Node* node : nodes) {
        std::vector<ai::path::Edge*> edges = node->GetEdges();
        for (ai::path::Edge* edge : edges) {
            // Removes the edge from both the node and its connection
            node->RemoveEdge(edge);

            auto iter = std::find(m_Edges.begin(), m_Edges.end(), edge);
            if (iter != m_Edges.end())
                m_Edges.erase(iter);
            delete edge;
        }
    }

    // Recalculate all of the edges for the area and surrounding edge
    s32 EdgeRadius = InvalidationRadius + 1;
    for (s32 y = -EdgeRadius; y <= EdgeRadius; ++y) {
        for (s32 z = -EdgeRadius; z <= EdgeRadius; ++z) {
            for (s32 x = -EdgeRadius; x <= EdgeRadius; ++x) {
                Vector3i processPos = position + Vector3i(x, y, z);

                std::vector<Link> links = ProcessBlock(processPos, m_Client->GetWorld());

                for (Link link : links) {
                    auto first = GetNode(link.first);
                    auto second = GetNode(link.second);

                    LinkNodes(first, second, link.weight);
                }
            }
        }
    }
}

std::vector<WorldGraph::Link> WorldGraph::ProcessBlock(Vector3i checkPos, Minecraft::World* world, ai::path::Node* target) {
    std::vector<Link> links;

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

    Minecraft::BlockPtr checkBlock = world->GetBlock(checkPos);

    // Skip because it's not loaded yet or it's solid
    if (!checkBlock || checkBlock->IsSolid()) return links;
    
    Vector3i targetPos;

    if (target)
        targetPos = target->GetPosition();

    if (IsWalkable(checkPos)) {
        for (Vector3i direction : directions) {
            Vector3i neighborPos = checkPos + direction;

            if (target && neighborPos != targetPos) continue;

            if (IsWalkable(neighborPos) || (IsSafeFall(neighborPos) && direction.y == 0)) {
                links.emplace_back(checkPos, neighborPos, 1.0f);
            }
        }
    } else if (IsWater(checkPos)) {
        for (Vector3i direction : waterDirections) {
            Vector3i neighborPos = checkPos + direction;

            if (target && neighborPos != targetPos) continue;

            if (IsWalkable(neighborPos) || IsWater(neighborPos)) {
                links.emplace_back(checkPos, neighborPos, 4.0f);
                links.emplace_back(neighborPos, checkPos, 4.0f);
            }
        }
    } else {
        int fallDist = IsSafeFall(checkPos);
        if (fallDist <= 0) return links;

        Vector3i current = checkPos;

        for (int i = 0; i < fallDist; ++i) {
            Vector3i fallPos = checkPos - Vector3i(0, i + 1, 0);

            Minecraft::BlockPtr block = world->GetBlock(fallPos);
            if (!block || block->IsSolid()) break;

            if (target && fallPos == targetPos)
                links.emplace_back(current, fallPos, 1.0f);

            current = fallPos;
        }
    }

    return links;
}

std::vector<WorldGraph::Link> WorldGraph::ProcessChunk(Minecraft::ChunkColumnPtr chunk, Minecraft::World* world) {
    std::vector<Link> links;

    const int YSearchRadius = 16;

    std::shared_ptr<PhysicsComponent> physics = GetActorComponent(m_Client, PhysicsComponent);
    if (physics == nullptr) return links;
    Vector3i position = ToVector3i(physics->GetPosition());

    for (int y = -YSearchRadius; y < YSearchRadius; ++y) {
        int checkY = (int)position.y + y;
        if (checkY <= 0 || checkY >= 256) continue;

        for (int z = 0; z < 16; ++z) {
            for (int x = 0; x < 16; ++x) {
                Vector3i checkPos = Vector3i(chunk->GetMetadata().x * 16 + x, checkY, chunk->GetMetadata().z * 16 + z);

                std::vector<Link> blockLinks = ProcessBlock(checkPos, world);
                links.insert(links.end(), blockLinks.begin(), blockLinks.end());
            }
        }
    }
    return links;
}

void WorldGraph::BuildGraph() {
    if (!m_NeedsBuilt) return;
    m_NeedsBuilt = false;

    this->Destroy();

    m_Edges.reserve(70000);

    std::cout << "Building graph\n";

    s64 startTime = util::GetTime();    

    Minecraft::World* world = m_Client->GetWorld();

    std::shared_ptr<PhysicsComponent> physics = GetActorComponent(m_Client, PhysicsComponent);
    if (physics == nullptr) return;

    const auto& currentMeta = world->GetChunk(ToVector3i(physics->GetPosition()))->GetMetadata();
    Vector3i currentChunkPos(currentMeta.x, 0, currentMeta.z);

    const double ChunkViewRange = 3.0;

    std::queue<Minecraft::ChunkColumnPtr> processQueue;
    for (auto iter = world->begin(); iter != world->end(); ++iter) {
        const auto& meta = iter->second->GetMetadata();
        double dist = currentChunkPos.Distance(Vector3i(meta.x, 0, meta.z));
        
        if (dist <= ChunkViewRange)
            processQueue.push(iter->second);
    }
    std::mutex mutex;

    const std::size_t NumThreads = 4;
    std::thread threads[NumThreads];

    std::cout << "Launching build threads." << std::endl;
    std::cout << "Number of chunks to process: " << processQueue.size() << std::endl;
    for (std::size_t i = 0; i < NumThreads; ++i) {
        threads[i] = std::thread([&]() {
            while (true) {
                Minecraft::ChunkColumnPtr chunk;

                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (processQueue.empty()) break;

                    chunk = processQueue.front();
                    processQueue.pop();
                }

                std::vector<Link> links = ProcessChunk(chunk, world);

                {
                    std::lock_guard<std::mutex> lock(mutex);
                    for (Link link : links) {
                        ai::path::Node* firstNode = GetNode(link.first);
                        ai::path::Node* secondNode = GetNode(link.second);

                        LinkNodes(firstNode, secondNode, link.weight);
                    }
                }
            }
        });
    }

    std::cout << "Waiting for build threads" << std::endl;
    // Wait for all threads to finish.
    for (std::size_t i = 0; i < NumThreads; ++i) {
        threads[i].join();
    }

    std::cout << "Graph built in " << (util::GetTime() - startTime) << "ms.\n";
    std::cout << "Nodes: " << m_Nodes.size() << std::endl;
    std::cout << "Edges: " << m_Edges.size() << std::endl;
}

void WorldGraph::BuildGraph2() {
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
