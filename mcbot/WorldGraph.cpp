#include "WorldGraph.h"

#include "GameClient.h"
#include "Utility.h"
#include "PhysicsComponent.h"
#include <iostream>

std::shared_ptr<PhysicsComponent> WorldGraph::ChunkTaskComparator::physics = nullptr;

Vector3d Hadamard(Vector3d first, Vector3d second) {
    return Vector3d(first.x * second.x, first.y * second.y, first.z * second.z);
}

// Prioritize chunks whose y value is closest to the player's y value.
bool WorldGraph::ChunkTaskComparator::operator()(Vector3i first, Vector3i second) {
    if (!physics) {
        return first < second;
    }

    Vector3i playerChunk = ToVector3i(physics->GetPosition()) / 16;

    static const Vector3d weights(1.2, 1.0, 1.2);

    double valFirst = Hadamard(ToVector3d(playerChunk - first), weights).LengthSq();
    double valSecond = Hadamard(ToVector3d(playerChunk - second), weights).LengthSq();

    return valFirst > valSecond;

    /*
    s32 yIndex = (s32)physics->GetPosition().y / 16;

    s32 firstDiff = std::abs(yIndex - (s32)first.y);
    s32 secondDiff = std::abs(yIndex - (s32)second.y);

    return firstDiff > secondDiff;*/
}

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
    : m_Client(client)
{
    client->GetWorld()->RegisterListener(this);
}

WorldGraph::~WorldGraph() {
    m_Client->GetWorld()->UnregisterListener(this);
}

void WorldGraph::OnChunkLoad(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, u16 yIndex) {
    for (s32 i = 0; i < 16; ++i) {
        if (meta.sectionmask & (1 << i)) {
            m_BuildQueue.Push(Vector3i(meta.x, i, meta.z));
        }
    }

    // todo: invalidate null chunks
}

void WorldGraph::OnChunkUnload(Minecraft::ChunkColumnPtr chunk) {
    
    auto remove = std::remove_if(m_BuildQueue.GetData().begin(), m_BuildQueue.GetData().end(), [&](Vector3i area) {
        auto meta = chunk->GetMetadata();
        return area.x == meta.x && area.z == meta.z;
    });

    // Just remove the chunk from the build queue because it's not part of the graph yet.
    if (remove != m_BuildQueue.GetData().end()) {
        m_BuildQueue.GetData().erase(remove, m_BuildQueue.GetData().end());
        m_BuildQueue.Update();
        return;
    }
    
    // todo: Invalidate the nodes in the chunk
    
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

                std::vector<Link> links = ProcessBlock(processPos);

                for (Link link : links) {
                    auto first = GetNode(link.first);
                    auto second = GetNode(link.second);

                    LinkNodes(first, second, link.weight);
                }
            }
        }
    }
}

void WorldGraph::ProcessQueue() {
    ChunkTaskComparator::physics = GetActorComponent(m_Client, PhysicsComponent);
    if (m_BuildQueue.Empty()) return;

    m_BuildQueue.Update();

    Vector3i current = m_BuildQueue.Pop();

    Minecraft::ChunkColumnPtr col = m_Client->GetWorld()->GetChunk(current * 16);
    if (!col) return;

    Minecraft::ChunkPtr chunk = (*col)[(std::size_t)current.y];
    if (!chunk) return;

    std::vector<Link> links = ProcessChunk(chunk, col->GetMetadata(), (s32)current.y);
    for (Link link : links) {
        ai::path::Node* firstNode = GetNode(link.first);
        ai::path::Node* secondNode = GetNode(link.second);

        LinkNodes(firstNode, secondNode, link.weight);
    }
}

std::vector<WorldGraph::Link> WorldGraph::ProcessBlock(Vector3i checkPos) {
    Minecraft::World* world = m_Client->GetWorld();
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

    if (IsWalkable(checkPos)) {
        for (Vector3i direction : directions) {
            Vector3i neighborPos = checkPos + direction;

            if (IsWalkable(neighborPos) || (IsSafeFall(neighborPos) && direction.y == 0)) {
                links.emplace_back(checkPos, neighborPos, 1.0f);
            }
        }
    } else if (IsWater(checkPos)) {
        for (Vector3i direction : waterDirections) {
            Vector3i neighborPos = checkPos + direction;

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

            links.emplace_back(current, fallPos, 1.0f);

            current = fallPos;
        }
    }

    return links;
}

std::vector<WorldGraph::Link> WorldGraph::ProcessChunk(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, s32 yIndex) {
    Minecraft::World* world = m_Client->GetWorld();
    std::vector<Link> links;
    
    std::shared_ptr<PhysicsComponent> physics = GetActorComponent(m_Client, PhysicsComponent);
    if (physics == nullptr) return links;
    Vector3i position = ToVector3i(physics->GetPosition());

    for (int y = 0; y < 16; ++y) {
        for (int z = 0; z < 16; ++z) {
            for (int x = 0; x < 16; ++x) {
                Vector3i checkPos = Vector3i(meta.x * 16 + x, yIndex * 16 + y, meta.z * 16 + z);

                std::vector<Link> blockLinks = ProcessBlock(checkPos);
                links.insert(links.end(), blockLinks.begin(), blockLinks.end());
            }
        }
    }
    return links;
}
