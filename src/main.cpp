#include <Geode/Geode.hpp>
#include <Geode/modify/CCLabelBMFont.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>

using namespace geode::prelude;

// ---------------------------------------------------------------------
// The 1.4 font vertical-offset glitch
// ---------------------------------------------------------------------
// In old GD builds, CCLabelBMFont glyph placement had a rounding quirk
// that pushed every character a pixel or two lower than the label's
// baseline. Rather than betting on a single entry point, this hooks
// EVERY public function that can rebuild or change a label's glyphs
// (createFontChars, updateLabel, both setString overloads, setCString).
// These call into each other internally (e.g. setString -> updateLabel
// -> createFontChars), so a re-entrancy guard (m_inGlitchHook) makes
// sure only the outermost call actually applies the offset - inner
// nested calls just run the original function and back out. This way
// whichever path your GD build actually uses to update a label's text,
// it's covered, without ever double/triple-stacking the offset.

class $modify(GlitchLabel, CCLabelBMFont) {
    struct Fields {
        bool m_inGlitchHook = false;
    };

    void applyGlitchOffset() {
        int offsetPx = Mod::get()->getSettingValue<int64_t>("font-glitch-offset");
        if (offsetPx <= 0) return;

        for (auto sprite : this->getChildrenExt<CCSprite>()) {
            auto pos = sprite->getPosition();
            sprite->setPosition({ pos.x, pos.y - static_cast<float>(offsetPx) });
        }
    }

    void createFontChars() {
        bool outermost = !m_fields->m_inGlitchHook;
        if (outermost) m_fields->m_inGlitchHook = true;
        CCLabelBMFont::createFontChars();
        if (outermost) {
            applyGlitchOffset();
            m_fields->m_inGlitchHook = false;
        }
    }

    void updateLabel() {
        bool outermost = !m_fields->m_inGlitchHook;
        if (outermost) m_fields->m_inGlitchHook = true;
        CCLabelBMFont::updateLabel();
        if (outermost) {
            applyGlitchOffset();
            m_fields->m_inGlitchHook = false;
        }
    }

    void setString(const char* newString) {
        bool outermost = !m_fields->m_inGlitchHook;
        if (outermost) m_fields->m_inGlitchHook = true;
        CCLabelBMFont::setString(newString);
        if (outermost) {
            applyGlitchOffset();
            m_fields->m_inGlitchHook = false;
        }
    }

    void setString(const char* newString, bool needUpdateLabel) {
        bool outermost = !m_fields->m_inGlitchHook;
        if (outermost) m_fields->m_inGlitchHook = true;
        CCLabelBMFont::setString(newString, needUpdateLabel);
        if (outermost) {
            applyGlitchOffset();
            m_fields->m_inGlitchHook = false;
        }
    }

    void setCString(const char* label) {
        bool outermost = !m_fields->m_inGlitchHook;
        if (outermost) m_fields->m_inGlitchHook = true;
        CCLabelBMFont::setCString(label);
        if (outermost) {
            applyGlitchOffset();
            m_fields->m_inGlitchHook = false;
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

// ---------------------------------------------------------------------
// Live preview popup + settings button
// ---------------------------------------------------------------------
// A popup showing sample text. Because it just creates a normal
// CCLabelBMFont, the createFontChars hook above applies the current
// offset to it automatically - so this is a genuine live preview, not
// a fake mockup.

class GlitchPreviewPopup : public geode::Popup {
protected:
    bool init() {
        if (!Popup::init(240.f, 160.f))
            return false;

        this->setTitle("Font Glitch Preview");

        auto label = CCLabelBMFont::create("GEOMETRY DASH 1.4", "bigFont.fnt");
        m_mainLayer->addChildAtPosition(label, Anchor::Center);

        return true;
    }

public:
    static GlitchPreviewPopup* create() {
        auto ret = new GlitchPreviewPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

// Custom "button" setting type, following Geode's documented pattern,
// so the settings popup gets a real "Preview" button that opens the
// popup above.

class PreviewButtonSettingV3 : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json) {
        auto res = std::make_shared<PreviewButtonSettingV3>();
        auto root = checkJson(json, "PreviewButtonSettingV3");

        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);

        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const& json) override { return true; }
    bool save(matjson::Value& json) const override { return true; }
    bool isDefaultValue() const override { return true; }
    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};

class PreviewButtonSettingNodeV3 : public SettingNodeV3 {
protected:
    ButtonSprite* m_buttonSprite;
    CCMenuItemSpriteExtra* m_button;

    bool init(std::shared_ptr<PreviewButtonSettingV3> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        m_buttonSprite = ButtonSprite::create("Preview", "goldFont.fnt", "GJ_button_01.png", .8f);
        m_buttonSprite->setScale(.5f);
        m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite, this, menu_selector(PreviewButtonSettingNodeV3::onButton)
        );
        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->setContentWidth(80);
        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);

        return true;
    }

    void updateState(CCNode* invoker) override {
        SettingNodeV3::updateState(invoker);

        auto shouldEnable = this->getSetting()->shouldEnable();
        m_button->setEnabled(shouldEnable);
        m_buttonSprite->setCascadeColorEnabled(true);
        m_buttonSprite->setCascadeOpacityEnabled(true);
        m_buttonSprite->setOpacity(shouldEnable ? 255 : 155);
        m_buttonSprite->setColor(shouldEnable ? ccWHITE : ccGRAY);
    }

    void onButton(CCObject*) {
        GlitchPreviewPopup::create()->show();
    }

    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static PreviewButtonSettingNodeV3* create(std::shared_ptr<PreviewButtonSettingV3> setting, float width) {
        auto ret = new PreviewButtonSettingNodeV3();
        if (ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool hasUncommittedChanges() const override { return false; }
    bool hasNonDefaultValue() const override { return false; }

    std::shared_ptr<PreviewButtonSettingV3> getSetting() const {
        return std::static_pointer_cast<PreviewButtonSettingV3>(SettingNodeV3::getSetting());
    }
};

SettingNodeV3* PreviewButtonSettingV3::createNode(float width) {
    return PreviewButtonSettingNodeV3::create(
        std::static_pointer_cast<PreviewButtonSettingV3>(shared_from_this()),
        width
    );
}

$on_mod(Loaded) {
    (void)Mod::get()->registerCustomSettingType("preview-button", &PreviewButtonSettingV3::parse);
}
