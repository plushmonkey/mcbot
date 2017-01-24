#ifndef MCBOT_DECISION_H_
#define MCBOT_DECISION_H_

#include <memory>
#include <iostream>

class DecisionAction;

class DecisionTreeNode {
public:
    virtual DecisionAction* Decide() = 0;
};
typedef std::shared_ptr<DecisionTreeNode> DecisionTreeNodePtr;

class DecisionAction : public DecisionTreeNode {
public:
    DecisionAction* Decide() {
        return this;
    }

    virtual void Act() = 0;
};

class Decision : public DecisionTreeNode {
protected:
    DecisionTreeNodePtr m_TrueNode;
    DecisionTreeNodePtr m_FalseNode;

public:
    Decision(DecisionTreeNodePtr trueNode, DecisionTreeNodePtr falseNode)
        : m_TrueNode(trueNode), m_FalseNode(falseNode)
    {

    }

    virtual DecisionTreeNode* GetBranch() = 0;

    DecisionAction* Decide() {
        return GetBranch()->Decide();
    }
};

template <typename T>
class RangeDecision : public Decision {
public:
    typedef std::function<T()> TestFunction;
private:
    TestFunction m_TestFunction;
    T m_MinValue;
    T m_MaxValue;

public:
    RangeDecision(DecisionTreeNodePtr trueNode, DecisionTreeNodePtr falseNode, TestFunction testFunction, T minValue, T maxValue)
        : Decision(trueNode, falseNode), m_TestFunction(testFunction), m_MinValue(minValue), m_MaxValue(maxValue)
    {

    }

    DecisionTreeNodePtr GetBranch() {
        T testValue = m_TestFunction();

        if (m_MinValue <= testValue && testValue <= m_MaxValue)
            return m_TrueNode;
        return m_FalseNode;
    }

    DecisionAction* Decide() {
        return GetBranch()->Decide();
    }
};

#endif
