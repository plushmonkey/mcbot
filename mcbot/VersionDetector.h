#ifndef _MCBOT_VERSIONDETECTOR_H_
#define _MCBOT_VERSIONDETECTOR_H_

#include <mclib/core/Client.h>

class VersionDetector : public mc::core::ConnectionListener {
private:
    mc::protocol::Version m_Version;
    mc::core::Connection* m_Connection;
    std::string m_Host;
    u16 m_Port;
    bool m_Found;

public:
    VersionDetector(const std::string& host, u16 port);

    mc::protocol::Version GetVersion();
    void OnPingResponse(const Json::Value& node) override;
};

#endif
