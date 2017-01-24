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
    bool IsWalkable(Vector3i pos);

    int IsSafeFall(Vector3i pos);

    bool IsWater(Vector3i pos);

public:
    WorldGraph(GameClient* client);

    ~WorldGraph();

    void OnChunkLoad(Minecraft::ChunkPtr chunk, const Minecraft::ChunkColumnMetadata& meta, u16 yIndex);

    void BuildGraph();
};


#endif
