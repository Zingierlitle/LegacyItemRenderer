#pragma once

#include <string>
#include <vector>

#include "ll/api/mod/NativeMod.h"
#include "mod/ModBase.h"

namespace legacy_item_render {

struct Config {
    int  version = 1;
    bool enabled = true;
    // "json_switch": ComponentItem::shouldUseJsonForRenderMatrix() returns false for affected items, so the
    //                renderer uses _applyDefaultItemTransforms like sticks and swords (a true revert).
    // "render_flag": ItemInHandRenderer::renderItem gets posAndRotSetByJSON = false for affected items.
    std::string mode = "json_switch";
    // Also apply to server/add-on items (namespace other than "minecraft").
    bool applyToCustomItems = true;
    // Only change the answer while held-item rendering is running (safety guard).
    bool onlyDuringHeldItemRender = true;
    // Full names that keep the JSON path, e.g. "minecraft:cake".
    std::vector<std::string> denylist;
    bool                     logItemsOnce = true;
};

class Mod {
public:
    static Mod& getInstance();
    Mod() : mSelf(*ll::mod::NativeMod::current()) {}
    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }
    bool                              load();
    bool                              enable();
    bool                              disable();

    modbase::LiveConfig<Config> config;
    modbase::LogOnce            logOnce;

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace legacy_item_render
