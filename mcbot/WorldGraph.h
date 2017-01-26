#ifndef MCBOT_WORLD_GRAPH_H_
#define MCBOT_WORLD_GRAPH_H_

#include <mclib/World.h>
#include "Pathing.h"

class GameClient;

// Todo: rebuild graph when world changes. It will probably need to be async and updated infrequently. (500-800ms rebuild)
class WorldGraph : public ai::path::Graph, public Minecraft::WorldListener {
private:
    GameClient* m_Client;
    bool m_NeedsBuilt;

    ai::path::Node* GetNode(Vector3i pos);

    // The check pos is not solid, the block above is not solid, and the block below is solid
    bool IsWalkable(Vector3i pos) const;

    int IsSafeFall(Vector3i pos) const;

    bool IsWater(Vector3i pos) const;
    
    struct Link {
        Vector3i first;
        Vector3i second;
        float weight;

        Link(Vector3i f, Vector3i s, float w) : first(f), second(s), weight(w) { }
    };

    std::vector<Link> ProcessBlock(Vector3i checkPos, Minecraft::World* world, ai::path::Node* target = nullptr);
    std::vector<Link> ProcessChunk(Minecraft::ChunkColumnPtr chunk, Minecraft::World* world);
public:
    WorldGraph(GameClient* client);

    ~WorldGraph();

    void OnChunkLoad(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, u16 yIndex);
    void OnBlockChange(Vector3i position, Minecraft::BlockPtr newBlock, Minecraft::BlockPtr oldBlock);

    void BuildGraph();
    void BuildGraph2();
};


#endif
