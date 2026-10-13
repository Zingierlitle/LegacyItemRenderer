// Shared helpers (each mod keeps its own copy under src/mod/ so projects stay standalone).
#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_set>

#include "ll/api/Config.h"
#include "ll/api/io/Logger.h"
#include "ll/api/mod/NativeMod.h"

namespace modbase {

// Live-reloading config: call maybeReload() from hot code; it checks the file time at most once per second.
template <class T>
class LiveConfig {
public:
    T cfg;

    void init(ll::mod::NativeMod& self) {
        mSelf = &self;
        mPath = self.getConfigDir() / "config.json";
        load();
    }

    // Returns true when the config was reloaded this call.
    bool maybeReload() {
        auto now = std::chrono::steady_clock::now();
        if (now - mLastCheck < std::chrono::seconds(1)) return false;
        mLastCheck = now;
        std::error_code ec;
        auto t = std::filesystem::last_write_time(mPath, ec);
        if (ec || t == mStamp) return false;
        load();
        return true;
    }

private:
    void load() {
        T fresh{};
        try {
            std::filesystem::create_directories(mPath.parent_path());
            if (!ll::config::loadConfig(fresh, mPath)) ll::config::saveConfig(fresh, mPath);
            cfg = fresh;
            mSelf->getLogger().info("Config loaded from {}", mPath.string());
        } catch (...) {
            // Broken JSON: keep the previous values and never overwrite the user's file.
            mSelf->getLogger().warn("Config file is invalid, keeping previous settings: {}", mPath.string());
        }
        std::error_code ec;
        mStamp = std::filesystem::last_write_time(mPath, ec);
    }

    ll::mod::NativeMod*                         mSelf{};
    std::filesystem::path                       mPath;
    std::filesystem::file_time_type             mStamp{};
    std::chrono::steady_clock::time_point       mLastCheck{};
};

// Logs a given key only the first time it is seen (cleared on config reload).
class LogOnce {
public:
    bool first(std::string const& key) {
        std::scoped_lock lk(mMutex);
        return mSeen.insert(key).second;
    }
    void clear() {
        std::scoped_lock lk(mMutex);
        mSeen.clear();
    }

private:
    std::mutex                      mMutex;
    std::unordered_set<std::string> mSeen;
};

} // namespace modbase
