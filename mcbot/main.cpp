#include "BotUpdate.h"
#include "chi/ChiBot.h"

const bool MysticEmpire = false;

class Logger : public Minecraft::Packets::PacketHandler {
private:
    GameClient* m_Client;

public:
    Logger(GameClient* client, Minecraft::Packets::PacketDispatcher* dispatcher)
        : Minecraft::Packets::PacketHandler(dispatcher), m_Client(client)
    {
        using namespace Minecraft::Protocol;

        dispatcher->RegisterHandler(State::Play, Play::Chat, this);
        dispatcher->RegisterHandler(State::Play, Play::EntityEffect, this);
        dispatcher->RegisterHandler(State::Play, Play::RemoveEntityEffect, this);
    }

    ~Logger() {
        GetDispatcher()->UnregisterHandler(this);
    }

    void HandlePacket(Minecraft::Packets::Inbound::ChatPacket* packet) {
        std::string message = ParseChatNode(packet->GetChatData());

        std::cout << StripChatMessage(message) << std::endl;
    }

    void HandlePacket(Minecraft::Packets::Inbound::EntityEffectPacket* packet) {
        Minecraft::EntityId eid = packet->GetEntityId();
        u8 effectId = packet->GetEffectId();
        u8 amplifier = packet->GetAmplifier();

        //std::cout << "Entity " << eid << " got effect " << (int)effectId << " level " << (int)amplifier << std::endl;
    }

    void HandlePacket(Minecraft::Packets::Inbound::RemoveEntityEffectPacket* packet) {
        Minecraft::EntityId eid = packet->GetEntityId();
        u8 effectId = packet->GetEffectId();

        //std::cout << "Entity " << eid << " lost effect " << (int)effectId << std::endl;
    }
};

int main(void) {
    Minecraft::BlockRegistry::GetInstance()->RegisterVanillaBlocks();
    GameClient game;
    BotUpdate update(&game);
    Logger logger(&game, game.GetDispatcher());
    
    chi::ChiBot::RegisterComponents(&update);
    chi::ChiBot::CreateDecisionTree(&update, MysticEmpire);

    if (MysticEmpire)
        game.login("play.mysticempire.net", 25565, "email", "password");
    else
        game.login("127.0.0.1", 25565, "bot", "pw");
    
    game.run();

    return 0;
}
