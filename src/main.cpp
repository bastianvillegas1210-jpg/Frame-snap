#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <set>
#include <sstream>

using namespace geode::prelude;

struct Pending {
    bool active = false;
    bool releaseQueued = false;
    int fireAt = 0;
    int releaseAt = 0;
    int button = 1;
    bool p1 = true;
};
static Pending g_pending;

static std::set<int> parseTargets() {
    std::set<int> out;
    std::stringstream ss(Mod::get()->getSettingValue<std::string>("targets"));
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        try { out.insert(std::stoi(tok)); } catch (...) {}
    }
    return out;
}

class $modify(SnapGL, GJBaseGameLayer) {
    int nowFrame() { return m_gameState.m_currentProgress; }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (!Mod::get()->getSettingValue<bool>("enabled") || !PlayLayer::get())
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);

        int now = nowFrame();

        if (down) {
            int window = Mod::get()->getSettingValue<int64_t>("window");
            for (int t : parseTargets()) {
                if (t > now && t - now <= window) {
                    g_pending = {true, false, t, 0, button, isPlayer1};
                    return;
                }
            }
        } else if (g_pending.active && g_pending.button == button && g_pending.p1 == isPlayer1) {
            g_pending.releaseQueued = true;
            g_pending.releaseAt = g_pending.fireAt + 1;
            return;
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        int now = nowFrame();
        if (g_pending.active && now >= g_pending.fireAt) {
            g_pending.active = false;
            GJBaseGameLayer::handleButton(true, g_pending.button, g_pending.p1);
            if (!g_pending.releaseQueued) g_pending.releaseAt = -1;
        }
        if (g_pending.releaseQueued && !g_pending.active && now >= g_pending.releaseAt) {
            g_pending.releaseQueued = false;
            GJBaseGameLayer::handleButton(false, g_pending.button, g_pending.p1);
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

class $modify(SnapPL, PlayLayer) {
    void resetLevel() {
        g_pending = Pending{};
        PlayLayer::resetLevel();
    }
};
