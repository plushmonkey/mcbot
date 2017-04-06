#ifndef AI_PATHING_H_
#define AI_PATHING_H_

#include <functional>
#include <deque>
#include <map>
#include <unordered_map>
#include <list>

#include <mclib/common/Common.h>

namespace ai {
namespace path {

class Edge;

class Node {
private:
    mc::Vector3i m_Position;
    std::vector<Edge*> m_Edges;

    Edge* FindNodeEdge(Node* other) const;

public:
    Node(mc::Vector3i position);

    mc::Vector3i GetPosition() const { return m_Position; }
    
    void AddEdge(Edge* edge);
    void RemoveEdge(Edge* edge);
    const std::vector<Edge*>& GetEdges() const { return m_Edges; }

    // Follow all of the edges to grab any immediately connected nodes
    std::vector<Node*> GetNeighbors() const;
    float GetCostFrom(Node* node) const;
};

class Edge {
private:
    Node* m_Nodes[2];
    float m_Weight;

public:
    Edge(float weight = 1.0) {
        m_Weight = weight;
    }

    float GetWeight() const { return m_Weight; }

    Node* GetNode(std::size_t index) const { return m_Nodes[index]; }
    Node* GetConnected(const Node* from) const;
    void LinkNodes(Node* first, Node* second);
};

class Plan {
private:
    std::vector<Node*> m_Path;
    typedef std::vector<Node*>::iterator iterator;
    iterator m_Iterator;

public:
    Plan() {
        m_Iterator = m_Path.end();
    }

    bool HasNext() const {
        return m_Iterator != m_Path.end();
    }

    std::size_t GetSize() { return m_Path.size(); }
    std::vector<Node*>::reference operator[](std::size_t index) { return m_Path[index]; }

    void Reset() {
        m_Iterator = m_Path.begin();
    }

    Node* GetCurrent() {
        if (m_Iterator == m_Path.end()) return nullptr;
        return *m_Iterator;
    }

    Node* GetGoal() const {
        if (m_Path.empty()) return nullptr;
        return m_Path.back();
    }

    Node* Next() {
        Node* result = *m_Iterator;
        ++m_Iterator;
        return result;
    }

    void AddNode(Node* node) {
        m_Path.push_back(node);
    }

    std::vector<Node*>& GetNodes() { return m_Path; }
};

class PlanningNode {
private:
    PlanningNode* m_Prev;
    Node* m_Node;
    Node* m_Goal;
    float m_GoalCost;
    float m_HeuristicCost;
    float m_FitnessCost;
    bool m_Closed;

public:
    PlanningNode(PlanningNode* prev, Node* node, Node* goal);

    PlanningNode* GetPrevious() const { return m_Prev; }
    Node* GetNode() const { return m_Node; }
    Node* GetGoal() const { return m_Goal; }
    float GetGoalCost() const { return m_GoalCost; }
    float GetHeuristicCost() const { return m_HeuristicCost; }
    float GetFitnessCost() const { return m_FitnessCost; }
    bool IsClosed() const { return m_Closed; }
    void SetClosed(bool closed) { m_Closed = closed; }

    void SetPrevious(PlanningNode* previous) { 
        m_Prev = previous;

        if (m_Prev) {
            m_GoalCost = m_Prev->GetGoalCost() + m_Node->GetCostFrom(m_Prev->GetNode());
        } else {
            m_GoalCost = 0;
        }

        m_HeuristicCost = (float)(m_Node->GetPosition() - m_Goal->GetPosition()).Length();
        m_FitnessCost = m_GoalCost + m_HeuristicCost;
    }

    bool IsBetterThan(const PlanningNode* other) const {
        return m_FitnessCost < other->GetFitnessCost();
    }
};

template <typename T, typename Compare, typename Container = std::vector<T>>
class PriorityQueue {
private:
    Container m_Container;
    Compare m_Comp;

public:
    void Push(T item) {
        m_Container.push_back(item);
        std::push_heap(m_Container.begin(), m_Container.end(), m_Comp);
    }

    T Pop() {
        T item = m_Container.front();
        std::pop_heap(m_Container.begin(), m_Container.end(), m_Comp);
        m_Container.pop_back();
        return item;
    }

    void Update() {
        std::make_heap(m_Container.begin(), m_Container.end(), m_Comp);
    }

    bool Empty() const { return m_Container.empty(); }
    Container& GetData() { return m_Container; }
};

struct PlanningNodeComparator {
    bool operator()(PlanningNode* first, PlanningNode* second) {
        return !first->IsBetterThan(second);
    }
};

class AStar {
private:
    std::unordered_map<Node*, PlanningNode*> m_NodeMap;
    PriorityQueue<PlanningNode*, PlanningNodeComparator> m_OpenSet;
    Node* m_Goal;

    PlanningNode* AddToOpenSet(Node* node, PlanningNode* prev);

    // Backtrace path and reverse it to get finished plan
    std::shared_ptr<Plan> BuildPath(PlanningNode* goal);

public:
    ~AStar();

    std::shared_ptr<Plan> operator()(Node* start, Node* goal);
};

// Extend this and fill out nodes/edges with world data
class Graph {
protected:
    std::map<mc::Vector3i, Node*> m_Nodes;
    // Store edges by position of first node for quick lookup
    std::map<mc::Vector3i, std::vector<Edge*>> m_Edges;

    bool LinkNodes(Node* first, Node* second, float weight = 1.0);
public:
    virtual ~Graph();

    Node* FindClosest(const mc::Vector3i& pos) const;
    std::shared_ptr<Plan> FindPath(const mc::Vector3i& start, const mc::Vector3i& end) const;

    void Destroy();
};

} // ns path
} // ns ai

#endif
