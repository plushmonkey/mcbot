#ifndef CHI_BOT_H_
#define CHI_BOT_H_

#include "../Decision.h"
#include "../BotUpdate.h"

namespace chi {

class ChiBot {
public:
    static void CreateDecisionTree(BotUpdate* update, bool mystic = false);
    static void RegisterComponents(BotUpdate* update);
};

}

#endif
