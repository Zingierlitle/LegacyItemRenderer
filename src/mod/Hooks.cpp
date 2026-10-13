// LegacyItemRender hooks (LeviLamina 26.10.x client). Visual only.
//
// Data-driven items (ComponentItem: snowball, apple, kelp, server custom items...) ask
// ComponentItem::shouldUseJsonForRenderMatrix() whether to use the JSON held-item transform
// (_transformWorldMatrixFromJson, which scales with texture resolution: MCPE-169754) or the default legacy
// transform (_applyDefaultItemTransforms, used by sticks and swords). Answering "false" makes them take the
// legacy path. Attachable items (mIsAttachable) are never touched.

#include <algorithm>

#include "ll/api/memory/Hook.h"
#include "mc/client/renderer/BaseActorRenderContext.h"
#include "mc/client/renderer/game/ItemInHandRenderer.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/minecraft_renderer/game/ItemContextFlags.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/components/ComponentItem.h"

#include "mod/Mod.h"

namespace legacy_item_render {
namespace {

Mod& mod() { return Mod::getInstance(); }

// Depth > 0 while the game is drawing a held item. Only then do we change any answer, in case
// shouldUseJsonForRenderMatrix is also used outside held-item rendering.
thread_local int tHeldDepth = 0;
struct DepthGuard {
    DepthGuard() { ++tHeldDepth; }
    ~DepthGuard() { --tHeldDepth; }
};

bool active() {
    if (mod().config.maybeReload()) {
        mod().logOnce.clear();
        mod().getSelf().getLogger().info("Config reloaded (mode={})", mod().config.cfg.mode);
    }
    return mod().config.cfg.enabled;
}

// Rules 4-6 (attachable and enabled checks are done by the caller).
bool affected(std::string const& fullName) {
    auto const& cfg = mod().config.cfg;
    if (std::find(cfg.denylist.begin(), cfg.denylist.end(), fullName) != cfg.denylist.end()) return false;
    if (!cfg.applyToCustomItems && fullName.rfind("minecraft:", 0) != 0) return false;
    if (cfg.onlyDuringHeldItemRender && tHeldDepth == 0) return false;
    return true;
}

void logItem(std::string const& name, bool fixed) {
    if (mod().config.cfg.logItemsOnce && mod().logOnce.first(name)) {
        mod().getSelf().getLogger().info("{} -> {}", name, fixed ? "legacy held-item path" : "unchanged");
    }
}

} // namespace

// Main switch.
LL_TYPE_INSTANCE_HOOK(
    JsonSwitchHook,
    ll::memory::HookPriority::Normal,
    ComponentItem,
    &ComponentItem::$shouldUseJsonForRenderMatrix,
    bool
) {
    bool useJson = origin();
    if (!useJson || !active() || mod().config.cfg.mode != "json_switch") return useJson;
    if (mIsAttachable.get()) return useJson; // bow/trident/custom 3D items keep their models
    std::string name  = mFullName.get().getString();
    bool        apply = affected(name);
    logItem(name, apply);
    return apply ? false : useJson;
}

// Depth guard for the first-person pass.
LL_TYPE_INSTANCE_HOOK(
    FirstPersonGuardHook,
    ll::memory::HookPriority::Normal,
    ItemInHandRenderer,
    &ItemInHandRenderer::renderFirstPerson,
    void,
    BaseActorRenderContext& renderContext,
    ItemContextFlags        itemFlags
) {
    DepthGuard g;
    origin(renderContext, itemFlags);
}

// Depth guard for every held item draw (first and third person) + the render_flag mode.
LL_TYPE_INSTANCE_HOOK(
    RenderItemHook,
    ll::memory::HookPriority::Normal,
    ItemInHandRenderer,
    &ItemInHandRenderer::renderItem,
    void,
    BaseActorRenderContext& renderContext,
    Actor&                  entity,
    ItemStack const&        item,
    bool                    posAndRotSetByJSON,
    ItemContextFlags        itemFlags,
    bool                    useMatrixAsIs,
    bool                    renderingMainHand
) {
    DepthGuard g;
    if (posAndRotSetByJSON && active() && mod().config.cfg.mode == "render_flag") {
        Item const* it = item.getItem();
        if (it && it->isComponentBased() && !static_cast<ComponentItem const*>(it)->mIsAttachable.get()) {
            std::string name = item.getTypeName();
            if (affected(name)) {
                logItem(name, true);
                posAndRotSetByJSON = false;
            }
        }
    }
    origin(renderContext, entity, item, posAndRotSetByJSON, itemFlags, useMatrixAsIs, renderingMainHand);
}

#define LIR_HOOK(H)                                                                                                    \
    mod().getSelf().getLogger().info("Hook {} {}", #H, H::hook() == 0 ? "installed" : "FAILED (symbol not found?)")

void installHooks() {
    LIR_HOOK(JsonSwitchHook);
    LIR_HOOK(FirstPersonGuardHook);
    LIR_HOOK(RenderItemHook);
}

void removeHooks() {
    JsonSwitchHook::unhook();
    FirstPersonGuardHook::unhook();
    RenderItemHook::unhook();
}

} // namespace legacy_item_render
