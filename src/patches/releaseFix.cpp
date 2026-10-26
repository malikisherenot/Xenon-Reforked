#include "releaseFix.hpp"

using namespace geode::prelude;

bool patchesEnabled = false;

void applyPatches()
{
    for (auto* p : Mod::get()->getPatches()) 
    {
        if (!p->isEnabled()) { (void)p->enable(); }
    }
    geode::log::info("Enabled patches");
    patchesEnabled = true;
}

void disablePatches()
{
    for (auto* p : Mod::get()->getPatches()) 
    {
        if (p->isEnabled()) { (void)p->disable(); }
    }
    geode::log::info("Disabled patches");
    patchesEnabled = false;
}

#ifdef GEODE_IS_ANDROID
// Android: no byte patches. Same effect through function hooks:
// releaseAllButtons is skipped while inside the 4 functions the Windows patches NOP'd.
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

namespace
{
    int g_skipRelease = 0;
    struct SkipReleaseScope
    {
        bool active;
        SkipReleaseScope() : active(patchesEnabled) { if (active) { ++g_skipRelease; } }
        ~SkipReleaseScope() { if (active) { --g_skipRelease; } }
    };
}

class $modify(XenonRelPlayerObject, PlayerObject)
{
    void releaseAllButtons()
    {
        if (g_skipRelease > 0) { return; }
        PlayerObject::releaseAllButtons();
    }
};

class $modify(XenonRelPlayLayer, PlayLayer)
{
    void pauseGame(bool p0) { SkipReleaseScope s; PlayLayer::pauseGame(p0); }
    void resume() { SkipReleaseScope s; PlayLayer::resume(); }
    void resumeAndRestart() { SkipReleaseScope s; PlayLayer::resumeAndRestart(); }
};

class $modify(XenonRelBaseGameLayer, GJBaseGameLayer)
{
    void resetLevelVariables() { SkipReleaseScope s; GJBaseGameLayer::resetLevelVariables(); }
};
#endif
