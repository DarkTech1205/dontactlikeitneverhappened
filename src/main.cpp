#include <Geode/Geode.hpp>
#include <Geode/modify/CCLabelBMFont.hpp>

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

        auto children = label->getChildren();
        if (!children) return;

        CCObject* obj;
        CCARRAY_FOREACH(children, obj) {
            if (auto sprite = typeinfo_cast<CCSprite*>(obj)) {
                auto pos = sprite->getPosition();
                sprite->setPosition({ pos.x, pos.y - static_cast<float>(offsetPx) });
            }
        }
    }
};
