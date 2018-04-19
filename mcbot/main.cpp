#include "BotUpdate.h"
#include "chi/ChiBot.h"

using mc::Vector3d;
using mc::Vector3i;
using mc::ToVector3d;
using mc::ToVector3i;

const bool MysticEmpire = false;

class Logger : public mc::protocol::packets::PacketHandler {
private:
    GameClient* m_Client;

public:
    Logger(GameClient* client, mc::protocol::packets::PacketDispatcher* dispatcher)
        : mc::protocol::packets::PacketHandler(dispatcher), m_Client(client)
    {
        using namespace mc::protocol;

        dispatcher->RegisterHandler(State::Play, play::Chat, this);
        dispatcher->RegisterHandler(State::Play, play::EntityEffect, this);
        dispatcher->RegisterHandler(State::Play, play::RemoveEntityEffect, this);
    }

    ~Logger() {
        GetDispatcher()->UnregisterHandler(this);
    }

    void HandlePacket(mc::protocol::packets::in::ChatPacket* packet) {
        std::string message = mc::util::ParseChatNode(packet->GetChatData());

        std::cout << mc::util::StripChatMessage(message) << std::endl;
    }

    void HandlePacket(mc::protocol::packets::in::EntityEffectPacket* packet) {
        mc::EntityId eid = packet->GetEntityId();
        u8 effectId = packet->GetEffectId();
        u8 amplifier = packet->GetAmplifier();

        //std::cout << "Entity " << eid << " got effect " << (int)effectId << " level " << (int)amplifier << std::endl;
    }

    void HandlePacket(mc::protocol::packets::in::RemoveEntityEffectPacket* packet) {
        mc::EntityId eid = packet->GetEntityId();
        u8 effectId = packet->GetEffectId();

        //std::cout << "Entity " << eid << " lost effect " << (int)effectId << std::endl;
    }
};

void CleanupBot(BotUpdate* update) {
    GameClient* client = update->GetClient();

    client->RemoveComponent(Component::GetIdFromName(EffectComponent::name));
    client->RemoveComponent(Component::GetIdFromName(JumpComponent::name));
    client->RemoveComponent(Component::GetIdFromName(TargetingComponent::name));
}


int main(void) {
    mc::block::BlockRegistry::GetInstance()->RegisterVanillaBlocks();
    mc::util::VersionFetcher versionFetcher(server, port);

    auto version = versionFetcher.GetVersion();

    GameClient game(version);
    BotUpdate update(&game);
    Logger logger(&game, game.GetDispatcher());
    
    chi::ChiBot::RegisterComponents(&update);
    chi::ChiBot::CreateDecisionTree(&update, MysticEmpire);

    if (MysticEmpire)
        game.login("play.mysticempire.net", 25565, "email", "password");
    else
        game.login("127.0.0.1", 25565, "bot", "pw");
    
    game.run();

    chi::ChiBot::Cleanup(&update);
    return 0;
}
