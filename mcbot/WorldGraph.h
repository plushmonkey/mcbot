#ifndef MCBOT_WORLD_GRAPH_H_
#define MCBOT_WORLD_GRAPH_H_

#include <mclib/world/World.h>

#include "Pathing.h"

class GameClient;

class PhysicsComponent;

class WorldGraph : public ai::path::Graph, public mc::world::WorldListener {
private:
    struct ChunkTaskComparator {
        static PhysicsComponent* physics;

        bool operator()(mc::Vector3i first, mc::Vector3i second);
    };

    GameClient* m_Client;
    ai::path::PriorityQueue<mc::Vector3i, ChunkTaskComparator> m_BuildQueue;
    std::vector<mc::Vector3i> m_ProcessedChunks;

    ai::path::Node* GetNode(mc::Vector3i pos);
    int IsSafeFall(mc::Vector3i pos) const;
    bool IsWater(mc::Vector3i pos) const;
    
    struct Link {
        mc::Vector3i first;
        mc::Vector3i second;
        float weight;

        Link(mc::Vector3i f, mc::Vector3i s, float w) : first(f), second(s), weight(w) { }
    };

    std::vector<Link> ProcessBlock(mc::Vector3i checkPos);
    std::vector<Link> ProcessChunk(mc::world::ChunkPtr chunk, const mc::world::ChunkColumnMetadata& meta, s32 yIndex);
public:
    WorldGraph(GameClient* client);

    ~WorldGraph();

    bool IsWalkable(mc::Vector3i pos) const;

    void OnChunkLoad(mc::world::ChunkPtr chunk, const mc::world::ChunkColumnMetadata& meta, u16 yIndex) override;
    void OnBlockChange(mc::Vector3i position, mc::block::BlockState newBlock, mc::block::BlockState oldBlock) override;
    void OnChunkUnload(mc::world::ChunkColumnPtr chunk) override;

    void ProcessQueue();
};


#endif
