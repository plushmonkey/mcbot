#include "BotUpdate.h"
#include "chi/ChiBot.h"

const bool MysticEmpire = false;

class Logger : public Minecraft::Packets::PacketHandler {
public:
    Logger(Minecraft::Packets::PacketDispatcher* dispatcher)
        : Minecraft::Packets::PacketHandler(dispatcher)
    {
        using namespace Minecraft::Protocol;

        dispatcher->RegisterHandler(State::Play, Play::Chat, this);
    }

    ~Logger() {
        GetDispatcher()->UnregisterHandler(this);
    }

    void HandlePacket(Minecraft::Packets::Inbound::ChatPacket* packet) {
        std::string message = ParseChatNode(packet->GetChatData());

        std::cout << StripChatMessage(message) << std::endl;
    }
};

int main(void) {
    Minecraft::BlockRegistry::GetInstance()->RegisterVanillaBlocks();
    GameClient game;
    BotUpdate update(&game);
    Logger logger(game.GetDispatcher());
    
    chi::ChiBot::RegisterComponents(&update);
    chi::ChiBot::CreateDecisionTree(&update, MysticEmpire);

    if (MysticEmpire)
        game.login("play.mysticempire.net", 25565, "email", "password");
    else
        game.login("127.0.0.1", 25565, "bot", "pw");
    
    game.run();

    return 0;
}
