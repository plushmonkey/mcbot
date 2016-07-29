#include "Pathing.h"

#include "Utility.h"

namespace ai {
namespace path {

Node::Node(Vector3i position)
    : m_Position(position)
{

}

// Follow all of the edges to grab any immediately connected nodes
std::vector<Node*> Node::GetNeighbors() {
    std::vector<Node*> neighbors;
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

Edge* Node::FindNodeEdge(Node* other) {
    for (Edge* edge : m_Edges) {
        if (edge->GetConnected(this) == other)
            return edge;
    }
    return nullptr;
}

float Node::GetCostFrom(Node* node) {
    Edge* edge = FindNodeEdge(node);
    Vector3i toNode = node->GetPosition() - GetPosition();
    return toNode.Length() * edge->GetWeight();
}

Node* Edge::GetConnected(Node* from) {
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

    while (!m_OpenSet.empty()) {
        PlanningNode* node = m_OpenSet.front();

        if (node->GetNode() == goal)
            return BuildPath(node);

        m_OpenSet.pop_front();
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

            bool isBetter = false;

            if (!planNode)
                planNode = AddToOpenSet(neighbor, node);
            else if (cost < planNode->GetGoalCost())
                isBetter = true;

            if (isBetter) {
                planNode->SetPrevious(node);
                ReinsertNode(planNode);
            }
        }
    }

    return nullptr;
}

Plan* AStar::BuildPath(PlanningNode* goal) {
    Plan* plan = new Plan();
    std::vector<PlanningNode*> path;

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
        m_NodeMap[node] = planNode;
    } else {
        planNode = iter->second;
        planNode->SetClosed(false);
    }

    InsertNode(planNode);
    return planNode;
}

void AStar::InsertNode(PlanningNode* node) {
    if (m_OpenSet.empty()) {
        m_OpenSet.push_back(node);
        return;
    }

    auto iter = m_OpenSet.begin();
    PlanningNode* current = *iter;
    
    while (current->IsBetterThan(node)) {
        ++iter;

        if (iter != m_OpenSet.end())
            current = *iter;
        else
            break;
    }

    m_OpenSet.insert(iter, node);
}

void AStar::ReinsertNode(PlanningNode* node) {
    std::deque<PlanningNode*>::iterator iter;

    for (iter = m_OpenSet.begin(); iter != m_OpenSet.end(); ++iter) {
        if (node == *iter) {
            m_OpenSet.erase(iter);
            InsertNode(node);
            return;
        }
    }

    InsertNode(node);
}

Graph::~Graph() {
    Destroy();
}

void Graph::Destroy() {
    for (auto entry : m_Nodes) {
        delete entry.second;
    }

    m_Nodes.clear();

    for (Edge* edge : m_Edges) {
        delete edge;
    }

    m_Edges.clear();
}

Node* Graph::FindClosest(const Vector3i& pos) {
    if (m_Nodes.empty()) return nullptr;

    auto find = m_Nodes.find(pos);
    if (find != m_Nodes.end())
        return find->second;

    Node* closest = nullptr;
    float length = std::numeric_limits<float>::max();

    for (auto entry : m_Nodes) {
        Node* node = entry.second;

        Vector3i toPos = pos - node->GetPosition();
        float checkLength = toPos.Length();

        if (checkLength < length) {
            closest = node;
            length = checkLength;
        }
    }

    return closest;
}

Plan* Graph::FindPath(const Vector3i& start, const Vector3i& end) {
    Node* startNode = FindClosest(start);
    Node* endNode = FindClosest(end);

    AStar algorithm;

    Plan* plan = algorithm(startNode, endNode);

    if (plan)
        plan->Reset();

    return plan;
}

void Graph::LinkNodes(Node* first, Node* second) {
    std::vector<Node*> neighbors = first->GetNeighbors();
    if (std::find(neighbors.begin(), neighbors.end(), second) != neighbors.end()) return;

    Edge* edge = new Edge(1.0);

    edge->LinkNodes(first, second);

    first->AddEdge(edge);
    second->AddEdge(edge);

    m_Edges.push_back(edge);
}

} // ns path
} // ns ai