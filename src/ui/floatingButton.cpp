#include "floatingButton.hpp"
#include "gui.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

#ifdef GEODE_IS_MOBILE

// Floating "X" button: tap = open/close Xenon, drag = move it (position is saved)
class XenonFloatingButton : public CCLayer
{
    static constexpr float SIZE = 36.f;

    CCNode* m_button = nullptr;
    CCPoint m_touchStart;
    CCPoint m_grabOffset;
    bool m_dragging = false;

    bool init() override
    {
        if (!CCLayer::init()) { return false; }

        this->setID("floating-button"_spr);
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-600);
        this->setTouchEnabled(true);

        auto winSize = CCDirector::get()->getWinSize();

        m_button = CCNode::create();
        m_button->setContentSize({ SIZE, SIZE });
        m_button->setAnchorPoint({ 0.5f, 0.5f });

        // same look as the Glass theme: dark glass body, soft blue border, cyan accent text
        auto border = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        border->setContentSize({ SIZE + 3.f, SIZE + 3.f });
        border->setColor({ 128, 128, 230 });
        border->setOpacity(150);
        border->setPosition({ SIZE / 2, SIZE / 2 });
        m_button->addChild(border);

        auto body = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        body->setContentSize({ SIZE, SIZE });
        body->setColor({ 13, 13, 13 });
        body->setOpacity(215);
        body->setPosition({ SIZE / 2, SIZE / 2 });
        m_button->addChild(body);

        auto label = CCLabelBMFont::create("X", "bigFont.fnt");
        label->setScale(0.55f);
        label->setColor({ 128, 230, 255 });
        label->setPosition({ SIZE / 2, SIZE / 2 + 1.f });
        m_button->addChild(label);

        float x = Mod::get()->getSavedValue<float>("xenon-btn-x", 30.f);
        float y = Mod::get()->getSavedValue<float>("xenon-btn-y", winSize.height * 0.62f);
        m_button->setPosition(this->clampToScreen({ x, y }));
        this->addChild(m_button);

        return true;
    }

    CCPoint clampToScreen(CCPoint p)
    {
        auto winSize = CCDirector::get()->getWinSize();
        float half = SIZE / 2 + 2.f;
        p.x = std::clamp(p.x, half, winSize.width - half);
        p.y = std::clamp(p.y, half, winSize.height - half);
        return p;
    }

    void setPressed(bool pressed)
    {
        m_button->stopAllActions();
        m_button->runAction(CCEaseSineOut::create(CCScaleTo::create(0.08f, pressed ? 0.9f : 1.f)));
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override
    {
        if (!m_button || !m_button->isVisible()) { return false; }

        auto p = touch->getLocation();
        if (ccpDistance(p, m_button->getPosition()) > SIZE / 2 + 8.f) { return false; }

        m_touchStart = p;
        m_grabOffset = m_button->getPosition() - p;
        m_dragging = false;
        this->setPressed(true);
        return true; // swallow so the game doesn't get this touch
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override
    {
        auto p = touch->getLocation();
        if (!m_dragging && ccpDistance(p, m_touchStart) > 8.f) { m_dragging = true; }
        if (m_dragging) { m_button->setPosition(this->clampToScreen(p + m_grabOffset)); }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override
    {
        this->setPressed(false);
        if (m_dragging)
        {
            auto pos = m_button->getPosition();
            Mod::get()->setSavedValue<float>("xenon-btn-x", pos.x);
            Mod::get()->setSavedValue<float>("xenon-btn-y", pos.y);
        }
        else { gui::toggleGui(); }
        m_dragging = false;
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override
    {
        this->setPressed(false);
        m_dragging = false;
    }

public:
    static XenonFloatingButton* create()
    {
        auto ret = new XenonFloatingButton();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }
};

namespace floatingButton
{
    void create()
    {
        if (auto btn = XenonFloatingButton::create()) { SceneManager::get()->keepAcrossScenes(btn); }
    }
}

$on_game(Loaded) { floatingButton::create(); }

#else

namespace floatingButton { void create() {} }

#endif
