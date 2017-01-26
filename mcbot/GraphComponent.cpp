#include "GraphComponent.h"

#include "Actor.h"
#include "PhysicsComponent.h"

const char* GraphComponent::name = "Graph";


void GraphComponent::Update(double dt) {
    m_Graph->ProcessQueue();
}
