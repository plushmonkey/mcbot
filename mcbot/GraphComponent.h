#ifndef MCBOT_GRAPH_COMPONENT_H_
#define MCBOT_GRAPH_COMPONENT_H_

#include "Component.h"
#include "WorldGraph.h"

#include <memory>

class GraphComponent : public Component {
public:
    static const char* name;

private:
    std::shared_ptr<WorldGraph> m_Graph;

public:
    GraphComponent(GameClient* client) 
        : m_Graph(new WorldGraph(client))
    {

    }

    void Update(double dt);

    std::shared_ptr<WorldGraph> GetGraph() { return m_Graph; }
    const char* GetName() const { return name; }
};

#endif
