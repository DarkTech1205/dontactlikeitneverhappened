#include <Geode/Geode.hpp>
#include <Geode/modify/CCLabelBMFont.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;

// ---------------------------------------------------------------------
// The 1.4 font vertical-offset glitch
// ---------------------------------------------------------------------
// In old GD builds, CCLabelBMFont glyph placement had a rounding quirk
// that pushed every character a pixel or two lower than the label's
// baseline. We recreate it by hooking the point where the label
// rebuilds its child glyph sprites and nudging every child down after
// the original layout runs.
//
// NOTE: the exact bound method name/signature can shift between GD
// versions and Geode SDK releases. If `setString` isn't the right hook
// point for your bindings, check your local Geode bindings for
// CCLabelBMFont (createFontChars / updateLabel / setCString are the
// other usual candidates) and swap the modify target below.

class $modify(GlitchLabel, CCLabelBMFont) {
    void setString(const char* newString) {
        CCLabelBMFont::setString(newString);
        applyGlitchOffset(this);
    }

    static void applyGlitchOffset(CCLabelBMFont* label) {
        int offsetPx = Mod::get()->getSettingValue<int64_t>("font-glitch-offset");
        if (offsetPx <= 0) return;

        for (auto sprite : label->getChildrenExt<CCSprite>()) {
            auto pos = sprite->getPosition();
            sprite->setPosition({ pos.x, pos.y - static_cast<float>(offsetPx) });
        }
    }
};

// ---------------------------------------------------------------------
// Cosmetic-only moderator/owner buttons on the level info page
// ---------------------------------------------------------------------
// This reveals the moderator-only rate buttons (star rate, demon rate)
// and the owner-delete action on LevelInfoLayer regardless of actual
// account permissions, so you can see/click into them like old builds
// did. IMPORTANT: none of these actually talk to the server - each
// handler below is fully replaced (not just extended), so the real
// game code that would send a rating/deletion request never runs.
// Clicking them just shows a popup confirming nothing happened.
//
// Based on Geode's published LevelInfoLayer bindings (m_starRateBtn,
// m_demonRateBtn, onRateStarsMod, onRateDemon, confirmOwnerDelete).
// If your build's rate button ends up calling a different handler
// than expected (e.g. onRateStars instead of onRateStarsMod), let me
// know which button behaves wrong and I'll adjust the hook target.

class $modify(ModOwnerButtons, LevelInfoLayer) {
    void updateSideButtons() {
        LevelInfoLayer::updateSideButtons();

        // Force these visible/enabled even if the account isn't
        // actually a moderator - purely cosmetic navigation.
        if (m_starRateBtn) {
            m_starRateBtn->setVisible(true);
            m_starRateBtn->setEnabled(true);
        }
        if (m_demonRateBtn) {
            m_demonRateBtn->setVisible(true);
            m_demonRateBtn->setEnabled(true);
        }
    }

    // Replaces the real moderator star-rate action - no server call.
    void onRateStarsMod(CCObject* sender) {
        FLAlertLayer::create(
            "Rate Stars",
            "This button is cosmetic only in this mod - no rating was sent.",
            "OK"
        )->show();
    }

    // Replaces the real moderator demon-rate action - no server call.
    void onRateDemon(CCObject* sender) {
        FLAlertLayer::create(
            "Rate Demon",
            "This button is cosmetic only in this mod - no rating was sent.",
            "OK"
        )->show();
    }

    // Replaces the real owner-delete action - no level is deleted.
    void confirmOwnerDelete(CCObject* sender) {
        FLAlertLayer::create(
            "Delete Level",
            "This button is cosmetic only in this mod - nothing was deleted.",
            "OK"
        )->show();
    }
};
