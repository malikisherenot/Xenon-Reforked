#include "floatingButton.hpp"
#include "gui.hpp"
#include "../bot/frameStepper.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>

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

// In-level frame stepper controls (bottom center): STEPPER on/off + STEP (hold to repeat)
class XenonStepperPanel : public CCLayer
{
    static constexpr float H = 28.f;
    static constexpr float W_TOGGLE = 100.f;
    static constexpr float W_STEP = 56.f;
    static constexpr float GAP = 8.f;

    CCNode* m_root = nullptr;
    CCNode* m_toggleNode = nullptr;
    CCNode* m_stepNode = nullptr;
    CCScale9Sprite* m_stepBody = nullptr;
    CCLabelBMFont* m_toggleLabel = nullptr;
    int m_pressed = 0; // 0 none, 1 toggle, 2 step
    float m_held = 0.f;
    int m_lastState = -1;

    CCNode* makeButton(float w, const char* text, CCScale9Sprite** bodyOut, CCLabelBMFont** labelOut)
    {
        auto node = CCNode::create();
        node->setContentSize({ w, H });
        node->setAnchorPoint({ 0.5f, 0.5f });

        auto border = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        border->setContentSize({ w + 3.f, H + 3.f });
        border->setColor({ 128, 128, 230 });
        border->setOpacity(150);
        border->setPosition({ w / 2, H / 2 });
        node->addChild(border);

        auto body = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        body->setContentSize({ w, H });
        body->setColor({ 13, 13, 13 });
        body->setOpacity(215);
        body->setPosition({ w / 2, H / 2 });
        node->addChild(body);

        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setColor({ 128, 230, 255 });
        label->setPosition({ w / 2, H / 2 + 1.f });
        label->setScale(std::min(0.3f, (w - 10.f) / label->getContentSize().width));
        node->addChild(label);

        if (bodyOut) { *bodyOut = body; }
        if (labelOut) { *labelOut = label; }
        return node;
    }

    bool init() override
    {
        if (!CCLayer::init()) { return false; }

        this->setID("stepper-panel"_spr);
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-600);
        this->setTouchEnabled(true);

        auto winSize = CCDirector::get()->getWinSize();
        float total = W_TOGGLE + GAP + W_STEP;
        float left = winSize.width / 2 - total / 2;
        float y = 26.f;

        m_root = CCNode::create();
        m_toggleNode = this->makeButton(W_TOGGLE, "STEPPER OFF", nullptr, &m_toggleLabel);
        m_toggleNode->setPosition({ left + W_TOGGLE / 2, y });
        m_root->addChild(m_toggleNode);

        m_stepNode = this->makeButton(W_STEP, "STEP", &m_stepBody, nullptr);
        m_stepNode->setPosition({ left + W_TOGGLE + GAP + W_STEP / 2, y });
        m_root->addChild(m_stepNode);

        m_root->setVisible(false);
        this->addChild(m_root);
        this->scheduleUpdate();
        return true;
    }

    bool shouldShow()
    {
        if (!stepperButtons || replayStatus == 2 || !PlayLayer::get()) { return false; }
        auto scene = CCDirector::get()->getRunningScene();
        return !(scene && scene->getChildByType<PauseLayer>(0));
    }

    bool hit(CCNode* node, CCPoint p)
    {
        auto size = node->getContentSize();
        auto pos = node->getPosition();
        return CCRect(pos.x - size.width / 2 - 4.f, pos.y - size.height / 2 - 4.f, size.width + 8.f, size.height + 8.f).containsPoint(p);
    }

    void update(float dt) override
    {
        bool show = this->shouldShow();
        m_root->setVisible(show);
        if (!show) { m_pressed = 0; return; }

        int state = frameStepper ? 1 : 0;
        if (state != m_lastState)
        {
            m_lastState = state;
            m_toggleLabel->setString(state ? "STEPPER ON" : "STEPPER OFF");
            m_toggleLabel->setScale(std::min(0.3f, (W_TOGGLE - 10.f) / m_toggleLabel->getContentSize().width));
            m_toggleLabel->setColor(state ? ccColor3B{ 128, 230, 255 } : ccColor3B{ 150, 150, 150 });
            m_stepBody->setOpacity(state ? 215 : 90);
        }

        if (m_pressed == 2 && frameStepper)
        {
            m_held += dt;
            if (m_held > 0.35f) { shouldStep = true; } // hold = keep stepping
        }
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override
    {
        if (!m_root->isVisible()) { return false; }
        auto p = touch->getLocation();
        if (this->hit(m_toggleNode, p)) { m_pressed = 1; m_toggleNode->setScale(0.92f); }
        else if (this->hit(m_stepNode, p))
        {
            m_pressed = 2;
            m_held = 0.f;
            m_stepNode->setScale(0.92f);
            if (frameStepper) { shouldStep = true; }
        }
        else { return false; }
        return true;
    }

    void release()
    {
        m_toggleNode->setScale(1.f);
        m_stepNode->setScale(1.f);
        m_pressed = 0;
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override
    {
        if (m_pressed == 1 && replayStatus != 2) { frameStepper = !frameStepper; }
        this->release();
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override { this->release(); }

public:
    static XenonStepperPanel* create()
    {
        auto ret = new XenonStepperPanel();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }
};

// keep one button alive and move it into whatever scene is about to run
static XenonFloatingButton* g_button = nullptr;
static XenonStepperPanel* g_panel = nullptr;

static void attachToScene(CCScene* scene)
{
    if (!scene) { return; }
    if (!g_button)
    {
        g_button = XenonFloatingButton::create();
        if (!g_button) { return; }
        g_button->retain();
    }
    if (!g_panel)
    {
        stepperButtons = Mod::get()->getSavedValue<bool>("xenon-stepper-buttons", true);
        g_panel = XenonStepperPanel::create();
        if (g_panel) { g_panel->retain(); }
    }
    if (g_button->getParent() != scene)
    {
        g_button->removeFromParentAndCleanup(false);
        scene->addChild(g_button, 99999);
    }
    if (g_panel && g_panel->getParent() != scene)
    {
        g_panel->removeFromParentAndCleanup(false);
        scene->addChild(g_panel, 99998);
    }
}

class $modify(XenonButtonDirector, CCDirector)
{
    void willSwitchToScene(CCScene* newScene)
    {
        CCDirector::willSwitchToScene(newScene);
        attachToScene(newScene);
    }
};

namespace floatingButton
{
    void create() { attachToScene(CCDirector::get()->getRunningScene()); }
}

$on_game(Loaded) { floatingButton::create(); }

#else

namespace floatingButton { void create() {} }

#endif
