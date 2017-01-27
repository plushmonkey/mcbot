#ifndef MCBOT_WORLD_GRAPH_H_
#define MCBOT_WORLD_GRAPH_H_

#include <mclib/World.h>

#include "Pathing.h"

class GameClient;

class PhysicsComponent;

class WorldGraph : public ai::path::Graph, public Minecraft::WorldListener {
private:
    struct ChunkTaskComparator {
        static std::shared_ptr<PhysicsComponent> physics;

        bool operator()(Vector3i first, Vector3i second);
    };

    GameClient* m_Client;
    ai::path::PriorityQueue<Vector3i, ChunkTaskComparator> m_BuildQueue;

    ai::path::Node* GetNode(Vector3i pos);
    bool IsWalkable(Vector3i pos) const;
    int IsSafeFall(Vector3i pos) const;
    bool IsWater(Vector3i pos) const;
    
    struct Link {
        Vector3i first;
        Vector3i second;
        float weight;

        Link(Vector3i f, Vector3i s, float w) : first(f), second(s), weight(w) { }
    };

    std::vector<Link> ProcessBlock(Vector3i checkPos);
    std::vector<Link> ProcessChunk(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, s32 yIndex);
public:
    WorldGraph(GameClient* client);

    ~WorldGraph();

    void OnChunkLoad(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, u16 yIndex) override;
    void OnBlockChange(Vector3i position, Minecraft::BlockState newBlock, Minecraft::BlockState oldBlock) override;
    void OnChunkUnload(Minecraft::ChunkColumnPtr chunk) override;

    void ProcessQueue();
};


#endif
