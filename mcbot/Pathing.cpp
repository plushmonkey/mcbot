#include "Pathing.h"

#include "Utility.h"
#include <iostream>

namespace ai {
namespace path {

Node::Node(Vector3i position)
    : m_Position(position)
{

}

// Follow all of the edges to grab any immediately connected nodes
std::vector<Node*> Node::GetNeighbors() const {
    std::vector<Node*> neighbors;
    neighbors.reserve(16);
    for (Edge* edge : m_Edges) {
        Node* to = edge->GetNode(1);
        if (to == nullptr || to == this)
            continue;
        neighbors.push_back(to);
    }
    return neighbors;
}

void Node::AddEdge(Edge* edge) {
    m_Edges.push_back(edge);
}

void Node::RemoveEdge(Edge* edge) {
    ai::path::Node* other = edge->GetConnected(this);
    
    auto iter = std::find(m_Edges.begin(), m_Edges.end(), edge);
    if (iter != m_Edges.end())
        m_Edges.erase(iter);

    iter = std::find(other->m_Edges.begin(), other->m_Edges.end(), edge);
    if (iter != other->m_Edges.end())
        other->m_Edges.erase(iter);
}

Edge* Node::FindNodeEdge(Node* other) const {
    for (Edge* edge : m_Edges) {
        if (edge->GetConnected(this) == other)
            return edge;
    }
    return nullptr;
}

float Node::GetCostFrom(Node* node) const {
    Edge* edge = FindNodeEdge(node);
    double dist = (node->GetPosition() - GetPosition()).Length();
    return (float)(dist * edge->GetWeight());
}

Node* Edge::GetConnected(const Node* from) const {
    if (from == m_Nodes[0])
        return m_Nodes[1];
    return m_Nodes[0];
}

void Edge::LinkNodes(Node* first, Node* second) {
    m_Nodes[0] = first;
    m_Nodes[1] = second;
}

PlanningNode::PlanningNode(PlanningNode* prev, Node* node, Node* goal)
    : m_Prev(prev),
    m_Node(node),
    m_Goal(goal),
    m_Closed(false)
{
    SetPrevious(prev);
}

Plan* AStar::operator()(Node* start, Node* goal) {
    if (start == goal) return nullptr;

    m_Goal = goal;

    AddToOpenSet(start, nullptr);

    s64 startTime = util::GetTime();
    static const s64 Timeout = 200;

    while (!m_OpenSet.Empty()) {
        PlanningNode* node = m_OpenSet.Pop();

        // Give up after a few seconds
        if (util::GetTime() > startTime + Timeout) {
            break;
        }

        if (node->GetNode() == goal)
            return BuildPath(node);

        node->SetClosed(true);

        std::vector<Node*> neighbors = node->GetNode()->GetNeighbors();
        for (Node* neighbor : neighbors) {
            auto find = m_NodeMap.find(neighbor);

            if (find != m_NodeMap.end() && find->second->IsClosed())
                continue;

            float cost = node->GetGoalCost() + neighbor->GetCostFrom(node->GetNode());

            PlanningNode* planNode = nullptr;
            if (find != m_NodeMap.end())
                planNode = find->second;

            if (!planNode) {
                planNode = AddToOpenSet(neighbor, node);
            } else if (cost < planNode->GetGoalCost()) {
                planNode->SetPrevious(node);
                m_OpenSet.Update();
            }
        }
    }

    return nullptr;
}

Plan* AStar::BuildPath(PlanningNode* goal) {
    Plan* plan = new Plan();
    std::vector<PlanningNode*> path;
    path.reserve(64);
    PlanningNode* node = goal;
    while (node) {
        path.push_back(node);
        node = node->GetPrevious();
    }

    for (auto it = path.rbegin(); it != path.rend(); ++it)
        plan->AddNode((*it)->GetNode());

    return plan;
}

PlanningNode* AStar::AddToOpenSet(Node* node, PlanningNode* prev) {
    PlanningNode *planNode = nullptr;

    auto iter = m_NodeMap.find(node);
    if (iter == m_NodeMap.end()) {
        planNode = new PlanningNode(prev, node, m_Goal);
        m_NodeMap.insert(std::make_pair(node, planNode));

        m_OpenSet.Push(planNode);
    } else {
        planNode = iter->second;
        planNode->SetClosed(false);
        m_OpenSet.Update();
    }
    
    return planNode;
}

Graph::~Graph() {
    Destroy();
}

void Graph::Destroy() {
    for (auto entry : m_Nodes) {
        delete entry.second;
    }

    m_Nodes.clear();

    for (auto kv : m_Edges) {
        for (Edge* edge : kv.second)
            delete edge;
        kv.second.clear();
    }

    m_Edges.clear();
}

Node* Graph::FindClosest(const Vector3i& pos) const {
    if (m_Nodes.empty()) return nullptr;

    auto find = m_Nodes.find(pos);
    if (find != m_Nodes.end())
        return find->second;

    /*
    // Slow lookup. Disable and assume no reasonable path if pos is not a traversable node.
    // Maybe do spatial lookups to find an acceptable nearby node.
    Node* closest = nullptr;
    float length = std::numeric_limits<float>::max();

    for (auto entry : m_Nodes) {
        Node* node = entry.second;

        Vector3i toPos = pos - node->GetPosition();
        float checkLength = (float)toPos.Length();

        if (checkLength < length) {
            closest = node;
            length = checkLength;
        }
    }

    return closest;*/
    return nullptr;
}

Plan* Graph::FindPath(const Vector3i& start, const Vector3i& end) const {
    Node* startNode = FindClosest(start);
    Node* endNode = FindClosest(end);

    if (startNode == nullptr || endNode == nullptr) return nullptr;

    AStar algorithm;

    Plan* plan = algorithm(startNode, endNode);

    if (plan)
        plan->Reset();

    return plan;
}

bool Graph::LinkNodes(Node* first, Node* second, float weight) {
    std::vector<Node*> neighbors = first->GetNeighbors();
    if (std::find(neighbors.begin(), neighbors.end(), second) != neighbors.end()) return false;

    Edge* edge = new Edge(weight);

    edge->LinkNodes(first, second);

    first->AddEdge(edge);
    second->AddEdge(edge);

    m_Edges[edge->GetNode(0)->GetPosition()].push_back(edge);
    return true;
}

} // ns path
} // ns ai