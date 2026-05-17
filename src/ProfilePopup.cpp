#include <Geode/Geode.hpp>
#include <Geode/binding/CCSpriteGrayscale.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJUserScore.hpp>
#include <Geode/binding/MessagesProfilePage.hpp>
#include <Geode/binding/ProfilePage.hpp>
#include <Geode/binding/CommentCell.hpp>
#include "ProfilePopup.hpp"
#include <fmt/format.h>
#include <cue/ListBorder.hpp>
#include <cue/ListNode.hpp>
#include <cue/PlayerIcon.hpp>
#include "Geode/ui/BasedButtonSprite.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/NineSlice.hpp"
#include "Geode/ui/Button.hpp"
#include "include/ProfileOverhaulConstant.hpp"

using namespace geode::prelude;

ProfilePopup* ProfilePopup::create(int accountId, bool ownProfile) {
    auto ret = new ProfilePopup();
    if (ret && ret->init(accountId, ownProfile)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

ProfilePopup::~ProfilePopup() {
    auto glm = GameLevelManager::get();
    if (glm) {
        if (glm->m_userInfoDelegate == this) {
            glm->m_userInfoDelegate = nullptr;
        }
        if (glm->m_levelCommentDelegate == this) {
            glm->m_levelCommentDelegate = nullptr;
        }
    }
    if (m_profilePopup == this) {
        m_profilePopup = nullptr;
    }
}

bool ProfilePopup::init(int accountId, bool ownProfile) {
    if (!Popup::init(440.f, 290.f)) {
        return false;
    }

    m_profilePopup = Ref<ProfilePopup>(this);
    profile::onVanillaProfilePage = false;

    // check if is actually your own profile
    if (ownProfile) {
        auto currentUserId = GJAccountManager::get()->m_accountID;
        if (currentUserId != accountId) {
            log::debug("ProfilePopup was initialized with ownProfile=true but accountId {} does not match current user id {}", accountId, currentUserId);
            ownProfile = false;
        }
    }

    m_ownProfile = ownProfile;
    m_accountId = accountId;

    // i hate these stuff
    m_noElasticity = true;
    if (m_buttonMenu) m_buttonMenu->removeAllChildren();

    m_score = nullptr;

    // get the user's score info and account comments
    auto glm = GameLevelManager::get();
    if (glm) {
        glm->m_userInfoDelegate = this;
        glm->m_levelCommentDelegate = this;
        glm->getGJUserInfo(accountId);
        glm->getAccountComments(accountId, 0, 10);
        m_score = glm->userInfoForAccountID(accountId);
    }

    // left side panel
    m_closeMenu = CCMenu::create();
    m_closeMenu->setContentSize({35, 35});
    m_closeMenu->setID("close-menu");
    m_closeMenu->m_bIgnoreAnchorPointForPosition = false;
    m_closeMenu->setLayout(ColumnLayout::create());
    m_closeMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_closeMenu, Anchor::TopLeft, {30.f, -30.f}, {0.5, 0.5}, false);

    auto closeMenuBg = NineSlice::create("square02_small.png");
    closeMenuBg->setContentSize(m_closeMenu->getContentSize() + CCSize{5, 5});
    closeMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(closeMenuBg, Anchor::TopLeft, {30.f, -30.f}, {0.5, 0.5}, false);

    m_closeMenu->addChild(m_closeBtn);
    m_closeMenu->updateLayout();

    m_userOptionsMenu = CCMenu::create();
    m_userOptionsMenu->setContentSize({35, 135});
    m_userOptionsMenu->setID("user-options-menu");
    m_userOptionsMenu->m_bIgnoreAnchorPointForPosition = false;
    m_userOptionsMenu->setLayout(ColumnLayout::create()->setCrossAxisOverflow(false));
    m_userOptionsMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_userOptionsMenu, Anchor::Left, {30.f, 20.f}, {0.5, 0.5}, false);

    auto userOptionsMenuBg = NineSlice::create("square02_small.png");
    userOptionsMenuBg->setContentSize(m_userOptionsMenu->getContentSize() + CCSize{5, 5});
    userOptionsMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(userOptionsMenuBg, Anchor::Left, {30.f, 20.f}, {0.5, 0.5}, false);

    m_otherOptionsMenu = CCMenu::create();
    m_otherOptionsMenu->setContentSize({35, 70});
    m_otherOptionsMenu->setID("other-options-menu");
    m_otherOptionsMenu->m_bIgnoreAnchorPointForPosition = false;
    m_otherOptionsMenu->setLayout(ColumnLayout::create()->setCrossAxisOverflow(false));
    m_otherOptionsMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_otherOptionsMenu, Anchor::BottomLeft, {30.f, 50.f}, {0.5, 0.5}, false);

    auto otherOptionsMenuBg = NineSlice::create("square02_small.png");
    otherOptionsMenuBg->setContentSize(m_otherOptionsMenu->getContentSize() + CCSize{5, 5});
    otherOptionsMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(otherOptionsMenuBg, Anchor::BottomLeft, {30.f, 50.f}, {0.5, 0.5}, false);

    // center panel
    m_usernameMenu = CCMenu::create();
    m_usernameMenu->setContentSize({270, 25});
    m_usernameMenu->setID("username-menu");
    m_usernameMenu->m_bIgnoreAnchorPointForPosition = false;
    m_usernameMenu->setZOrder(2);
    m_usernameMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Start)->setCrossAxisOverflow(false)->setAutoScale(false));
    m_mainLayer->addChildAtPosition(m_usernameMenu, Anchor::Top, {-25.f, -25.f}, {0.5, 0.5}, false);

    m_statsMenu = CCMenu::create();
    m_statsMenu->setContentSize({270, 15});
    m_statsMenu->setID("stats-menu");
    m_statsMenu->m_bIgnoreAnchorPointForPosition = false;
    m_statsMenu->setZOrder(2);
    m_statsMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Start)->setGap(2.5f)->setCrossAxisOverflow(false)->setAutoScale(false));
    m_mainLayer->addChildAtPosition(m_statsMenu, Anchor::Top, {-25.f, -45.f}, {0.5, 0.5}, false);

    m_iconsMenu = CCMenu::create();
    m_iconsMenu->setContentSize({315, 45});
    m_iconsMenu->setID("icons-menu");
    m_iconsMenu->setZOrder(1);
    m_iconsMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Center)->setGap(10.f)->setCrossAxisOverflow(false)->setAutoScale(true));
    m_mainLayer->addChildAtPosition(m_iconsMenu, Anchor::Center, {0.f, 65.f}, {0.5, 0.5}, false);

    auto iconsMenuBorder = cue::ListBorder::create(cue::ListBorderStyle::Comments, {325, 45}, ccColor4B{191, 114, 62, 255});
    iconsMenuBorder->m_bIgnoreAnchorPointForPosition = false;
    iconsMenuBorder->setZOrder(2);
    m_mainLayer->addChildAtPosition(iconsMenuBorder, Anchor::Center, {0.f, 65.f}, {0.5, 0.5}, false);

    auto iconsMenuBg = CCLayerColor::create({191, 114, 62, 255}, iconsMenuBorder->getContentSize().width, iconsMenuBorder->getContentSize().height);
    iconsMenuBg->m_bIgnoreAnchorPointForPosition = false;
    iconsMenuBg->setZOrder(0);
    m_mainLayer->addChildAtPosition(iconsMenuBg, Anchor::Center, {0.f, 65.f}, {0.5, 0.5}, false);

    m_commentsList = cue::ListNode::create({325, 85}, {191, 114, 62, 255}, cue::ListBorderStyle::Comments);
    m_commentsList->setID("comments-list");
    m_commentsList->setZOrder(1);
    m_commentsList->setCellHeight(85.f);
    m_commentsList->setAutoUpdate(true);
    m_mainLayer->addChildAtPosition(m_commentsList, Anchor::Center, {0.f, -10.f}, {0.5, 0.5}, false);

    // right side panel
    m_refrshMenu = CCMenu::create();
    m_refrshMenu->setContentSize({35, 35});
    m_refrshMenu->setID("refresh-menu");
    m_refrshMenu->m_bIgnoreAnchorPointForPosition = false;
    m_refrshMenu->setLayout(ColumnLayout::create());
    m_refrshMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_refrshMenu, Anchor::TopRight, {-30.f, -30.f}, {0.5, 0.5}, false);

    auto refrshMenuBg = NineSlice::create("square02_small.png");
    refrshMenuBg->setContentSize(m_refrshMenu->getContentSize() + CCSize{5, 5});
    refrshMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(refrshMenuBg, Anchor::TopRight, {-30.f, -30.f}, {0.5, 0.5}, false);

    m_socialsMenu = CCMenu::create();
    m_socialsMenu->setContentSize({35, 135});
    m_socialsMenu->setID("socials-menu");
    m_socialsMenu->m_bIgnoreAnchorPointForPosition = false;
    m_socialsMenu->setLayout(ColumnLayout::create());
    m_socialsMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_socialsMenu, Anchor::Right, {-30.f, 20.f}, {0.5, 0.5}, false);

    auto socialsMenuBg = NineSlice::create("square02_small.png");
    socialsMenuBg->setContentSize(m_socialsMenu->getContentSize() + CCSize{5, 5});
    socialsMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(socialsMenuBg, Anchor::Right, {-30.f, 20.f}, {0.5, 0.5}, false);

    m_onlineMenu = CCMenu::create();
    m_onlineMenu->setContentSize({35, 70});
    m_onlineMenu->setID("online-menu");
    m_onlineMenu->m_bIgnoreAnchorPointForPosition = false;
    m_onlineMenu->setLayout(ColumnLayout::create());
    m_onlineMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_onlineMenu, Anchor::BottomRight, {-30.f, 50.f}, {0.5, 0.5}, false);

    auto onlineMenuBg = NineSlice::create("square02_small.png");
    onlineMenuBg->setContentSize(m_onlineMenu->getContentSize() + CCSize{5, 5});
    onlineMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(onlineMenuBg, Anchor::BottomRight, {-30.f, 50.f}, {0.5, 0.5}, false);

    return true;
}

void ProfilePopup::getUserInfoFinished(GJUserScore* score) {
    m_score = score;
    if (!Ref<ProfilePopup>(this)) return;
    if (!m_usernameMenu || !m_statsMenu || !m_iconsMenu) return;

    if (m_score) {
        log::info("async user score received: username={}, stars={}, demons={}, creator points={}", m_score->m_userName, m_score->m_stars, m_score->m_demons, m_score->m_creatorPoints);
        // TODO: refresh UI elements that depend on m_score here
        auto usernameLabel = CCLabelBMFont::create(m_score->m_userName.c_str(), "bigFont.fnt");
        usernameLabel->setScale(0.8f);
        m_usernameMenu->addChild(usernameLabel);

        auto infoSpr = CCSpriteGrayscale::createWithSpriteFrameName("GJ_infoIcon_001.png");
        auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(ProfilePopup::onInfo));
        m_buttonMenu->addChildAtPosition(infoBtn, Anchor::TopRight, {-5.f, -5.f}, {0.5f, 0.5f}, false);

        m_usernameMenu->updateLayout();

        // stats
        auto starsLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_stars).c_str(), "bigFont.fnt");
        starsLabel->setScale(0.4f);
        starsLabel->setColor({233, 253, 113});
        m_statsMenu->addChild(starsLabel);

        auto starsIcon = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        starsIcon->setScale(0.5f);
        m_statsMenu->addChild(starsIcon);

        // moons
        auto moonLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_moons).c_str(), "bigFont.fnt");
        moonLabel->setScale(0.4f);
        moonLabel->setColor({109, 215, 249});
        m_statsMenu->addChild(moonLabel);

        auto moonIcon = CCSprite::createWithSpriteFrameName("GJ_moonsIcon_001.png");
        moonIcon->setScale(0.5f);
        m_statsMenu->addChild(moonIcon);

        // demons
        auto demonLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_demons).c_str(), "bigFont.fnt");
        demonLabel->setScale(0.4f);
        demonLabel->setColor({240, 140, 140});
        m_statsMenu->addChild(demonLabel);

        auto demonIcon = CCSprite::createWithSpriteFrameName("GJ_demonIcon_001.png");
        demonIcon->setScale(0.5f);
        m_statsMenu->addChild(demonIcon);

        // creator points
        if (m_score->m_creatorPoints > 0) {
            auto cpLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_creatorPoints).c_str(), "bigFont.fnt");
            cpLabel->setScale(0.4f);
            cpLabel->setColor({182, 186, 186});
            m_statsMenu->addChild(cpLabel);

            auto cpIcon = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");
            cpIcon->setScale(0.5f);
            m_statsMenu->addChild(cpIcon);
        }

        // user coints
        auto userCoinsLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_userCoins).c_str(), "bigFont.fnt");
        userCoinsLabel->setScale(0.4f);
        userCoinsLabel->setColor({255, 255, 255});
        m_statsMenu->addChild(userCoinsLabel);

        auto userCoinsIcon = CCSprite::createWithSpriteFrameName("GJ_coinsIcon2_001.png");
        userCoinsIcon->setScale(0.5f);
        m_statsMenu->addChild(userCoinsIcon);

        // secret coins
        auto secretCoinsLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_secretCoins).c_str(), "bigFont.fnt");
        secretCoinsLabel->setScale(0.4f);
        secretCoinsLabel->setColor({248, 138, 0});
        m_statsMenu->addChild(secretCoinsLabel);

        auto secretCoinsIcon = CCSprite::createWithSpriteFrameName("GJ_coinsIcon_001.png");
        secretCoinsIcon->setScale(0.5f);
        m_statsMenu->addChild(secretCoinsIcon);

        m_statsMenu->updateLayout();

        // icons
        auto playerObject = [&](IconType iconType, int playerType) {
            auto wrapper = CCNode::create();
            wrapper->setContentSize({35, 35});
            wrapper->setAnchorPoint({0.5f, 0.5f});

            auto player = cue::PlayerIcon::create(iconType, playerType, m_score->m_color1, m_score->m_color2, m_score->m_color3);
            player->setAnchorPoint({0.5f, 0.5f});
            player->setPosition({wrapper->getContentSize().width / 2.f, wrapper->getContentSize().height / 2.f});
            wrapper->addChild(player);

            if (iconType == IconType::Ufo) player->setPositionY(player->getPositionY() - 7);  // very hacky way to make the ufo aligned
            return wrapper;
        };

        m_iconsMenu->addChild(playerObject(IconType::Cube, m_score->m_playerCube));
        m_iconsMenu->addChild(playerObject(IconType::Ship, m_score->m_playerShip));
        m_iconsMenu->addChild(playerObject(IconType::Ball, m_score->m_playerBall));
        m_iconsMenu->addChild(playerObject(IconType::Ufo, m_score->m_playerUfo));
        m_iconsMenu->addChild(playerObject(IconType::Wave, m_score->m_playerWave));
        m_iconsMenu->addChild(playerObject(IconType::Robot, m_score->m_playerRobot));
        m_iconsMenu->addChild(playerObject(IconType::Spider, m_score->m_playerSpider));
        m_iconsMenu->addChild(playerObject(IconType::Swing, m_score->m_playerSwing));
        m_iconsMenu->addChild(playerObject(IconType::Jetpack, m_score->m_playerJetpack));

        m_iconsMenu->updateLayout();

        // left panel
        if (m_ownProfile) {
            auto accountSettingsBtn = Button::createWithSpriteFrameName("accountBtn_settings_001.png", [this](geode::Button* sender) {
                GJAccountSettingsLayer::create(m_score->m_accountID)->show();
            });
            m_userOptionsMenu->addChild(accountSettingsBtn);

            auto frienRequestsBtn = Button::createWithSpriteFrameName("accountBtn_requests_001.png", [this](geode::Button* sender) {
                FRequestProfilePage::create(false)->show();
            });
            m_userOptionsMenu->addChild(frienRequestsBtn);

            // @geode-ignore(unknown-resource)
            auto shareCommentBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("geode.loader/message.png"), [this](geode::Button* sender) {
                if (m_score) {
                    ShareCommentLayer::create("Post Account Update", 140, CommentType::Account, m_score->m_accountID, "")->show();
                }
            });
            m_userOptionsMenu->addChild(shareCommentBtn);
        }

        auto oldProfileBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("PO-icon-person.png"_spr), [this](geode::Button* sender) {
            onClose(sender);
            profile::onVanillaProfilePage = true;
            ProfilePage::create(m_score->m_accountID, m_ownProfile)->show();
        });
        m_userOptionsMenu->addChild(oldProfileBtn);

        auto messageBtn = Button::createWithSpriteFrameName("accountBtn_messages_001.png", [this](geode::Button* sender) {
            if (m_score) {
                MessagesProfilePage::create(false)->show();
            }
        });

        m_userOptionsMenu->addChild(messageBtn);
        m_userOptionsMenu->updateLayout();
    }
}

// profile page own implemetation
void ProfilePopup::onInfo(CCObject* sender) {
    if (!m_score) {
        return;
    }

    auto message = fmt::format(
        "<cl>Account ID:</c> {}\n<cy>Stars:</c> {}\n<cb>Moons:</c> {}\n<cf>Diamonds:</c> {}\n<co>Secret Coins:</c> {}\n<cc>User Coins:</c> {}\n<cr>Demons:</c> {}",
        m_score->m_accountID,
        m_score->m_stars,
        m_score->m_moons,
        m_score->m_diamonds,
        m_score->m_secretCoins,
        m_score->m_userCoins,
        m_score->m_demons);

    if (m_score->m_creatorPoints > 0) {
        message += fmt::format("\n<cg>Creator Points:</c> {}", m_score->m_creatorPoints);
    }

    FLAlertLayer::create(m_score->m_userName.c_str(), std::string(message), "OK")->show();
}

// gjuserinfo delegate
void ProfilePopup::getUserInfoFailed(int id) {
    log::error("user info request failed for account id {}", id);
}

void ProfilePopup::loadCommentsFinished(cocos2d::CCArray* comments, char const* key) {
    if (!comments || !m_commentsList || !key) {
        return;
    }

    log::debug("comments {}", comments);

    // auto glm = GameLevelManager::get();
    // if (!glm || glm->typeFromCommentKey(key) != CommentType::Account) {
    //     return;
    // }

    m_commentsList->clear();
    const auto width = m_commentsList->getListSize().width;

    for (auto i = 0u; i < comments->count(); ++i) {
        auto commentObj = comments->objectAtIndex(i);
        if (!commentObj) {
            continue;
        }

        GJComment* comment = nullptr;
        if (auto dict = typeinfo_cast<cocos2d::CCDictionary*>(commentObj)) {
            comment = GJComment::create(dict);
        } else {
            comment = static_cast<GJComment*>(commentObj);
        }

        if (!comment) {
            continue;
        }

        auto cell = new CommentCell("commentCell", width, 85.f);
        if (!cell) {
            continue;
        }
        cell->autorelease();
        cell->setContentHeight(85);
        cell->loadFromComment(comment);
        cell->m_backgroundLayer->setOpacity(0);
        cell->m_accountComment = true;
        m_commentsList->addCell(cell);
    }

    m_commentsList->updateLayout();
}

void ProfilePopup::loadCommentsFailed(char const* key) {
    log::error("account comments load failed for key {}", key ? key : "<null>");
}

void ProfilePopup::userInfoChanged(GJUserScore* score) {
    if (score && m_score && score == m_score) {
        m_score = score;
        log::info("user score changed for account id {}", m_score->m_accountID);
        // TODO: refresh UI elements if needed
    }
}
