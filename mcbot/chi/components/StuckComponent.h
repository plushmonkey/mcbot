#ifndef MCBOT_STUCK_COMPONENT_H_
#define MCBOT_STUCK_COMPONENT_H_

#include "../../Component.h"
#include <mclib/Common.h>
#include <mclib/Packets/PacketHandler.h>
#include <mclib/Packets/PacketDispatcher.h>

class StuckComponent : public Component, Minecraft::Packets::PacketHandler {
public:
    static const char* name;

    Vector3d m_MaxAcceleration;
    s64 m_LastReset;
    s64 m_Corrections;
    s64 m_Threshold;

public:
    StuckComponent(Minecraft::Packets::PacketDispatcher* dispatcher, Vector3d maxAcceleration, s64 threshold)
        : Minecraft::Packets::PacketHandler(dispatcher), 
        m_MaxAcceleration(maxAcceleration),
        m_LastReset(0), 
        m_Corrections(0),
        m_Threshold(threshold)
    {
        dispatcher->RegisterHandler(Minecraft::Protocol::State::Play, Minecraft::Protocol::Play::PlayerPositionAndLook, this);
    }

    ~StuckComponent() {
        GetDispatcher()->UnregisterHandler(this);
    }

    void Update(double dt);
    void HandlePacket(Minecraft::Packets::Inbound::PlayerPositionAndLookPacket* packet) override;

    const char* GetName() const { return name; }
};

#endif
