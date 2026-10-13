#include "mod/Mod.h"

#include "ll/api/mod/RegisterHelper.h"

namespace legacy_item_render {

void installHooks();
void removeHooks();

Mod& Mod::getInstance() {
    static Mod instance;
    return instance;
}

bool Mod::load() {
    config.init(getSelf());
    return true;
}

bool Mod::enable() {
    installHooks();
    getSelf().getLogger().info("LegacyItemRender enabled");
    return true;
}

bool Mod::disable() {
    removeHooks();
    return true;
}

} // namespace legacy_item_render

LL_REGISTER_MOD(legacy_item_render::Mod, legacy_item_render::Mod::getInstance());
