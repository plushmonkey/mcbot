#ifndef MCBOT_STUCK_COMPONENT_H_
#define MCBOT_STUCK_COMPONENT_H_

#include "../../Component.h"
#include <mclib/common/Common.h>
#include <mclib/protocol/packets/PacketHandler.h>
#include <mclib/protocol/packets/PacketDispatcher.h>

class StuckComponent : public Component, mc::protocol::packets::PacketHandler {
public:
    static const char* name;

    mc::Vector3d m_MaxAcceleration;
    s64 m_LastReset;
    s64 m_Corrections;
    s64 m_Threshold;

public:
    StuckComponent(mc::protocol::packets::PacketDispatcher* dispatcher, mc::Vector3d maxAcceleration, s64 threshold)
        : mc::protocol::packets::PacketHandler(dispatcher),
        m_MaxAcceleration(maxAcceleration),
        m_LastReset(0), 
        m_Corrections(0),
        m_Threshold(threshold)
    {
        dispatcher->RegisterHandler(mc::protocol::State::Play, mc::protocol::play::PlayerPositionAndLook, this);
    }

    ~StuckComponent() {
        GetDispatcher()->UnregisterHandler(this);
    }

    void Update(double dt);
    void HandlePacket(mc::protocol::packets::in::PlayerPositionAndLookPacket* packet) override;

    const char* GetName() const { return name; }
};

#endif
