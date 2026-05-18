#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/binding/CCSpriteGrayscale.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/binding/FriendRequestPopup.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJUserScore.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/MessagesProfilePage.hpp>
#include <Geode/binding/ProfilePage.hpp>
#include <Geode/binding/ShareCommentDelegate.hpp>
#include <Geode/binding/ShareCommentLayer.hpp>
#include <Geode/binding/CommentCell.hpp>
#include <Geode/binding/LevelCell.hpp>
#include "ProfilePopup.hpp"
#include <fmt/format.h>
#include <cue/ListBorder.hpp>
#include <cue/ListNode.hpp>
#include <cue/PlayerIcon.hpp>
#include "Geode/c++stl/string.hpp"
#include "Geode/cocos/layers_scenes_transitions_nodes/CCTransition.h"
#include "Geode/ui/BasedButtonSprite.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/NineSlice.hpp"
#include "Geode/ui/Button.hpp"
#include "Geode/ui/Popup.hpp"
#include "Geode/utils/general.hpp"
#include "Geode/utils/web.hpp"
#include "include/ProfileOverhaulConstant.hpp"
#include "FriendNotePopup.hpp"

std::pair<std::string, std::string> loadFriendNoteFromFile(int accountId);

static std::string getRankIconForGlobalRank(int globalRank) {
    if (globalRank <= 0) {
        return "rankIcon_all_001.png";
    }
    if (globalRank == 1) {
        return "rankIcon_1_001.png";
    }
    if (globalRank <= 10) {
        return "rankIcon_top10_001.png";
    }
    if (globalRank <= 50) {
        return "rankIcon_top50_001.png";
    }
    if (globalRank <= 100) {
        return "rankIcon_top100_001.png";
    }
    if (globalRank <= 200) {
        return "rankIcon_top200_001.png";
    }
    if (globalRank <= 500) {
        return "rankIcon_top500_001.png";
    }
    if (globalRank <= 1000) {
        return "rankIcon_top1000_001.png";
    }
    if (globalRank <= 5000) {
        return "rankIcon_top2500_001.png";
    }
    if (globalRank <= 10000) {
        return "rankIcon_top5000_001.png";
    }
    if (globalRank <= 50000) {
        return "rankIcon_top10000_001.png";
    }
    return "rankIcon_all_001.png";
}

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
        if (glm->m_levelManagerDelegate == this) {
            glm->m_levelManagerDelegate = nullptr;
        }
        if (glm->m_leaderboardManagerDelegate == this) {
            glm->m_leaderboardManagerDelegate = nullptr;
        }
    }
    if (m_profilePopup == this) {
        m_profilePopup = nullptr;
    }

    if (m_levelCell) {
        m_levelCell->removeAllChildrenWithCleanup(true);
        m_levelCell = nullptr;
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
        glm->m_levelManagerDelegate = this;
        glm->m_leaderboardManagerDelegate = this;
        glm->getGJUserInfo(accountId);
        m_commentPage = 0;
        m_commentPageSize = 10;
        requestAccountCommentsPage(m_commentPage);
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
    m_userOptionsMenu->setLayout(ColumnLayout::create()->setCrossAxisOverflow(false)->setAxisReverse(true));
    m_userOptionsMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_userOptionsMenu, Anchor::Left, {30.f, 19.f}, {0.5, 0.5}, false);

    auto userOptionsMenuBg = NineSlice::create("square02_small.png");
    userOptionsMenuBg->setContentSize(m_userOptionsMenu->getContentSize() + CCSize{5, 5});
    userOptionsMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(userOptionsMenuBg, Anchor::Left, {30.f, 19.f}, {0.5, 0.5}, false);

    m_otherOptionsMenu = CCMenu::create();
    m_otherOptionsMenu->setContentSize({35, 70});
    m_otherOptionsMenu->setID("other-options-menu");
    m_otherOptionsMenu->m_bIgnoreAnchorPointForPosition = false;
    m_otherOptionsMenu->setLayout(ColumnLayout::create()->setCrossAxisOverflow(false)->setAxisReverse(true));
    m_otherOptionsMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_otherOptionsMenu, Anchor::BottomLeft, {30.f, 50.f}, {0.5, 0.5}, false);

    auto otherOptionsMenuBg = NineSlice::create("square02_small.png");
    otherOptionsMenuBg->setContentSize(m_otherOptionsMenu->getContentSize() + CCSize{5, 5});
    otherOptionsMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(otherOptionsMenuBg, Anchor::BottomLeft, {30.f, 50.f}, {0.5, 0.5}, false);

    // leaderboard menu
    m_leaderboardMenu = CCMenu::create();
    m_leaderboardMenu->setContentSize({60, 65});
    m_leaderboardMenu->setID("leaderboard-menu");
    m_leaderboardMenu->m_bIgnoreAnchorPointForPosition = false;
    m_leaderboardMenu->setLayout(ColumnLayout::create()->setCrossAxisOverflow(false)->setAutoScale(true)->setAxisReverse(true)->setCrossAxisAlignment(AxisAlignment::Start)->setCrossAxisLineAlignment(AxisAlignment::Start)->setGap(10.f));
    m_leaderboardMenu->setZOrder(5);
    m_leaderboardMenu->setScale(0.7f);
    m_mainLayer->addChildAtPosition(m_leaderboardMenu, Anchor::TopRight, {-80.f, -30.f}, {0.5, 0.5}, false);

    // center panel
    m_usernameMenu = CCMenu::create();
    m_usernameMenu->setContentSize({270, 25});
    m_usernameMenu->setID("username-menu");
    m_usernameMenu->m_bIgnoreAnchorPointForPosition = false;
    m_usernameMenu->setZOrder(5);
    m_usernameMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Start)->setCrossAxisOverflow(false)->setAutoScale(true));
    m_mainLayer->addChildAtPosition(m_usernameMenu, Anchor::TopLeft, {60.f, -25.f}, {0, 0.5}, false);

    m_statsMenu = CCMenu::create();
    m_statsMenu->setContentSize({270, 15});
    m_statsMenu->setID("stats-menu");
    m_statsMenu->m_bIgnoreAnchorPointForPosition = false;
    m_statsMenu->setZOrder(5);
    m_statsMenu->setScale(0.8f);
    m_statsMenu->setAnchorPoint({0.f, 0.5f});
    m_statsMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Start)->setGap(2.5f)->setCrossAxisOverflow(false)->setAutoScale(false));
    m_mainLayer->addChildAtPosition(m_statsMenu, Anchor::TopLeft, {60.f, -45.f}, {0, 0.5}, false);

    m_iconsMenu = CCMenu::create();
    m_iconsMenu->setContentSize({315, 40});
    m_iconsMenu->setID("icons-menu");
    m_iconsMenu->setZOrder(1);
    m_iconsMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Center)->setGap(10.f)->setCrossAxisOverflow(false)->setAutoScale(true));
    m_mainLayer->addChildAtPosition(m_iconsMenu, Anchor::Center, {0.f, 68.f}, {0.5, 0.5}, false);

    auto iconsMenuBorder = cue::ListBorder::create(cue::ListBorderStyle::Comments, {325, 40}, ccColor4B{191, 114, 62, 255});
    iconsMenuBorder->m_bIgnoreAnchorPointForPosition = false;
    iconsMenuBorder->setZOrder(2);
    m_mainLayer->addChildAtPosition(iconsMenuBorder, Anchor::Center, {0.f, 68.f}, {0.5, 0.5}, false);

    auto iconsMenuBg = CCLayerColor::create({191, 114, 62, 255}, iconsMenuBorder->getContentSize().width, iconsMenuBorder->getContentSize().height);
    iconsMenuBg->m_bIgnoreAnchorPointForPosition = false;
    iconsMenuBg->setZOrder(0);
    m_mainLayer->addChildAtPosition(iconsMenuBg, Anchor::Center, {0.f, 68.f}, {0.5, 0.5}, false);

    m_commentsList = cue::ListNode::create({325, 85}, {191, 114, 62, 255}, cue::ListBorderStyle::Comments);
    m_commentsList->setID("comments-list");
    m_commentsList->setZOrder(1);
    m_commentsList->setCellHeight(85.f);
    m_commentsList->setAutoUpdate(true);
    m_mainLayer->addChildAtPosition(m_commentsList, Anchor::Center, {0.f, -7.f}, {0.5, 0.5}, false);

    m_ratedLevelCell = cue::ListNode::create({325, 70}, ccColor4B{191, 114, 62, 255}, cue::ListBorderStyle::Comments);
    m_ratedLevelCell->setID("rated-level-cell");
    m_ratedLevelCell->setZOrder(2);
    m_ratedLevelCell->getScrollLayer()->m_disableMovement = true;
    m_mainLayer->addChildAtPosition(m_ratedLevelCell, Anchor::Bottom, {0.f, 50.f}, {0.5, 0.5}, false);

    auto levelCellBg = CCLayerColor::create({191, 114, 62, 255}, m_ratedLevelCell->getContentSize().width, m_ratedLevelCell->getContentSize().height);
    levelCellBg->m_bIgnoreAnchorPointForPosition = false;
    levelCellBg->setZOrder(0);
    m_mainLayer->addChildAtPosition(levelCellBg, Anchor::Bottom, {0.f, 50.f}, {0.5, 0.5}, false);

    // right side panel
    m_swapMenu = CCMenu::create();
    m_swapMenu->setContentSize({35, 35});
    m_swapMenu->setID("swap-menu");
    m_swapMenu->m_bIgnoreAnchorPointForPosition = false;
    m_swapMenu->setLayout(ColumnLayout::create());
    m_swapMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_swapMenu, Anchor::TopRight, {-30.f, -30.f}, {0.5, 0.5}, false);

    auto swapMenuBg = NineSlice::create("square02_small.png");
    swapMenuBg->setContentSize(m_swapMenu->getContentSize() + CCSize{5, 5});
    swapMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(swapMenuBg, Anchor::TopRight, {-30.f, -30.f}, {0.5, 0.5}, false);

    m_socialsMenu = CCMenu::create();
    m_socialsMenu->setContentSize({35, 135});
    m_socialsMenu->setID("socials-menu");
    m_socialsMenu->m_bIgnoreAnchorPointForPosition = false;
    m_socialsMenu->setLayout(ColumnLayout::create()->setCrossAxisOverflow(false)->setAxisReverse(true));
    m_socialsMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_socialsMenu, Anchor::Right, {-30.f, 19.f}, {0.5, 0.5}, false);

    auto socialsMenuBg = NineSlice::create("square02_small.png");
    socialsMenuBg->setContentSize(m_socialsMenu->getContentSize() + CCSize{5, 5});
    socialsMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(socialsMenuBg, Anchor::Right, {-30.f, 19.f}, {0.5, 0.5}, false);

    m_onlineMenu = CCMenu::create();
    m_onlineMenu->setContentSize({35, 70});
    m_onlineMenu->setID("online-menu");
    m_onlineMenu->m_bIgnoreAnchorPointForPosition = false;
    m_onlineMenu->setLayout(ColumnLayout::create()->setCrossAxisOverflow(false)->setAxisReverse(true));
    m_onlineMenu->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_onlineMenu, Anchor::BottomRight, {-30.f, 50.f}, {0.5, 0.5}, false);

    auto onlineMenuBg = NineSlice::create("square02_small.png");
    onlineMenuBg->setContentSize(m_onlineMenu->getContentSize() + CCSize{5, 5});
    onlineMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(onlineMenuBg, Anchor::BottomRight, {-30.f, 50.f}, {0.5, 0.5}, false);

    if (m_score) {
        refreshUserInfoUI();
        requestMoonLeaderboardRank();
    }

    return true;
}

void ProfilePopup::refreshUserInfoUI() {
    if (!Ref<ProfilePopup>(this)) return;

    if (!m_usernameMenu || !m_statsMenu || !m_iconsMenu || !m_swapMenu || !m_userOptionsMenu || !m_onlineMenu || !m_otherOptionsMenu) return;

    m_usernameMenu->removeAllChildren();
    m_statsMenu->removeAllChildren();
    m_iconsMenu->removeAllChildren();
    m_swapMenu->removeAllChildren();
    m_userOptionsMenu->removeAllChildren();
    m_onlineMenu->removeAllChildren();
    m_otherOptionsMenu->removeAllChildren();
    m_socialsMenu->removeAllChildren();
    m_leaderboardMenu->removeAllChildren();

    if (m_buttonMenu) m_buttonMenu->removeAllChildren();
    if (!m_score) {
        m_usernameMenu->updateLayout();
        m_statsMenu->updateLayout();
        m_iconsMenu->updateLayout();
        m_swapMenu->updateLayout();
        m_userOptionsMenu->updateLayout();
        m_onlineMenu->updateLayout();
        m_otherOptionsMenu->updateLayout();
        m_socialsMenu->updateLayout();
        m_leaderboardMenu->updateLayout();
        return;
    }

    if (m_levelCell) {
        m_levelCell->setVisible(false);
        m_levelCell = nullptr;
    }

    m_ratedLevelPage = 0;
    m_ratedLevelSearchKey.clear();
    m_ratedLevelFetchCompleted = false;
    m_ratedLevelFetchFailed = false;

    log::debug("refresh user score UI: username={}, stars={}, demons={}, creator points={}", m_score->m_userName, m_score->m_stars, m_score->m_demons, m_score->m_creatorPoints);
    auto infoSpr = CCSpriteGrayscale::createWithSpriteFrameName("GJ_infoIcon_001.png");
    auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(ProfilePopup::onInfo));
    if (m_buttonMenu) m_buttonMenu->addChildAtPosition(infoBtn, Anchor::TopRight, {-5.f, -5.f}, {0.5f, 0.5f}, false);

    std::string displayName = m_score->m_userName;
    if (m_score->m_friendReqStatus == 1) {
        auto [savedNickname, savedNote] = loadFriendNoteFromFile(m_score->m_accountID);
        if (!savedNickname.empty()) {
            displayName = "*" + savedNickname;
        }
    }

    auto usernameLabel = Button::createWithLabel(displayName.c_str(), "bigFont.fnt", [this](geode::Button* sender) {
        Notification::create("Username Copied to Clipboard", NotificationIcon::Info, 1.5f)->show();
        geode::utils::clipboard::write(m_score->m_userName);
    });
    usernameLabel->setScale(0.8f);
    usernameLabel->setAnchorPoint({0.f, 0.5f});
    usernameLabel->setScaleMultiplier(1.05f);
    m_usernameMenu->addChild(usernameLabel);

    if (m_score->m_modBadge == 1)  // normal moderator
    {
        auto modBadge = Button::createWithSpriteFrameName("modBadge_01_001.png", [this](geode::Button* sender) {
            FLAlertLayer::create(
                "Moderator",
                "<cy>Moderator</c> can suggest levels to <cf>RobTop</c> for rating and hold higher priority when suggesting Demon difficulty ratings.\n"
                "<cy>Moderator</c> comments are highlighted in <cg>light green</c>",
                "OK")
                ->show();
        });
        m_usernameMenu->addChild(modBadge);
    } else if (m_score->m_modBadge == 2)  // super moderator
    {
        auto modBadge = Button::createWithSpriteFrameName("modBadge_02_001.png", [this](geode::Button* sender) {
            if (m_score->m_accountID == 71) {
                FLAlertLayer::create(
                    "Developer",
                    "<cf>Developer</c> holds <cg>all moderator abilities</c>.\n"
                    "<cf>Developer</c> comments are highlighted in <cf>cyan</c>, though the <cg>elder moderator badge remains visible</c>\n"
                    "<cy>This moderation status is only applied to RobTop.</c>",
                    "OK")
                    ->show();
            } else {
                FLAlertLayer::create(
                    "Elder Moderator",
                    "<cg>Elder Moderators</c> hold all <co>moderator abilities</c>, with the additional ability to <cr>delete comments</c>, issue temporary <cr>comment bans</c>, remove <cy>unrated user levels</c> from the server, and <cc>whitelist scouted Newgrounds artists</c>.\n"
                    "<cg>Elder Moderator</c> comments are highlighted in <cg>dark green</c>",
                    "OK")
                    ->show();
            }
        });
        m_usernameMenu->addChild(modBadge);
    } else if (m_score->m_modBadge == 3)  // trial moderator
    {
        auto modBadge = Button::createWithSpriteFrameName("modBadge_03_001.png", [this](geode::Button* sender) {
            FLAlertLayer::create(
                "Leaderboard Moderator",
                "<cl>Leaderboard Moderators</c> monitor <cg>global</c> and <co>level-specific</c> leaderboards for <cr>illegitimate submissions</c> and manage player <cg>whitelisting for the global Top 1,000</c>.\n"
                "They hold a <cg>higher priority</c> when suggesting <cr>Demon difficulty ratings</c>.\n"
                "<cy>The role was first assigned in Update 2.11 and formally included in Update 2.2.</c>",
                "OK")
                ->show();
        });
        m_usernameMenu->addChild(modBadge);
    }

    // friend note
    if (usernameLabel && m_score->m_friendReqStatus == 1) {
        // @geode-ignore(unknown-resource)
        auto friendNoteBtn = Button::createWithNode(EditorButtonSprite::createWithSpriteFrameName("geode.loader/changelog.png", .8f, EditorBaseColor::LightBlue), [this](geode::Button* sender) {
            // Handle friend request button press
            FriendNotePopup::create(m_profilePopup, m_score)->show();
        });
        m_usernameMenu->addChild(friendNoteBtn);
    }
    m_usernameMenu->updateLayout();

    auto starsLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_stars).c_str(), "bigFont.fnt");
    starsLabel->setScale(0.4f);
    starsLabel->setColor({233, 253, 113});
    m_statsMenu->addChild(starsLabel);

    auto starsIcon = Button::createWithSpriteFrameName("GJ_starsIcon_001.png", [this](auto btn) {
        StarInfoPopup::createFromString(m_score->m_starsInfo)->show();
    });
    starsIcon->setScale(0.5f);
    m_statsMenu->addChild(starsIcon);

    auto moonLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_moons).c_str(), "bigFont.fnt");
    moonLabel->setScale(0.4f);
    moonLabel->setColor({109, 215, 249});
    m_statsMenu->addChild(moonLabel);

    auto moonIcon = Button::createWithSpriteFrameName("GJ_moonsIcon_001.png", [this](auto btn) {
        StarInfoPopup::createFromStringMoons(m_score->m_platformerInfo)->show();
    });
    moonIcon->setScale(0.5f);
    m_statsMenu->addChild(moonIcon);

    auto demonLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_demons).c_str(), "bigFont.fnt");
    demonLabel->setScale(0.4f);
    demonLabel->setColor({240, 140, 140});
    m_statsMenu->addChild(demonLabel);

    auto demonIcon = Button::createWithSpriteFrameName("GJ_demonIcon_001.png", [this](auto btn) {
        DemonInfoPopup::createFromString(m_score->m_demonInfo)->show();
    });
    demonIcon->setScale(0.5f);
    m_statsMenu->addChild(demonIcon);

    if (m_score->m_creatorPoints > 0) {
        auto cpLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_creatorPoints).c_str(), "bigFont.fnt");
        cpLabel->setScale(0.4f);
        cpLabel->setColor({182, 186, 186});
        m_statsMenu->addChild(cpLabel);

        auto cpIcon = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");
        cpIcon->setScale(0.5f);
        m_statsMenu->addChild(cpIcon);
    }

    auto userCoinsLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_userCoins).c_str(), "bigFont.fnt");
    userCoinsLabel->setScale(0.4f);
    userCoinsLabel->setColor({255, 255, 255});
    m_statsMenu->addChild(userCoinsLabel);

    auto userCoinsIcon = CCSprite::createWithSpriteFrameName("GJ_coinsIcon2_001.png");
    userCoinsIcon->setScale(0.5f);
    m_statsMenu->addChild(userCoinsIcon);

    auto secretCoinsLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_secretCoins).c_str(), "bigFont.fnt");
    secretCoinsLabel->setScale(0.4f);
    secretCoinsLabel->setColor({248, 138, 0});
    m_statsMenu->addChild(secretCoinsLabel);

    auto secretCoinsIcon = CCSprite::createWithSpriteFrameName("GJ_coinsIcon_001.png");
    secretCoinsIcon->setScale(0.5f);
    m_statsMenu->addChild(secretCoinsIcon);

    if (Mod::get()->getSettingValue<bool>("showDiamondStats")) {
        auto diamondsLabel = CCLabelBMFont::create(GameToolbox::pointsToString(m_score->m_diamonds).c_str(), "bigFont.fnt");
        diamondsLabel->setScale(0.4f);
        diamondsLabel->setColor({0, 255, 255});
        m_statsMenu->addChild(diamondsLabel);

        auto diamondsIcon = CCSprite::createWithSpriteFrameName("GJ_diamondsIcon_001.png");
        diamondsIcon->setScale(0.5f);
        m_statsMenu->addChild(diamondsIcon);
    }

    m_statsMenu->updateLayout();

    auto playerObject = [&](IconType iconType, int playerType) {
        auto wrapper = CCNode::create();
        wrapper->setContentSize({35, 35});
        wrapper->setAnchorPoint({0.5f, 0.5f});

        auto player = cue::PlayerIcon::create(iconType, playerType, m_score->m_color1, m_score->m_color2, m_score->m_color3);
        player->setAnchorPoint({0.5f, 0.5f});
        player->setPosition({wrapper->getContentSize().width / 2.f, wrapper->getContentSize().height / 2.f});
        wrapper->addChild(player);

        if (iconType == IconType::Ufo) {
            player->setPositionY(player->getPositionY() - 7);
        }
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

    auto oldProfileBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("PO-icon-person.png"_spr), [this](geode::Button* sender) {
        onClose(sender);
        profile::onVanillaProfilePage = true;
        ProfilePage::create(m_score->m_accountID, m_ownProfile)->show();
    });
    m_swapMenu->addChild(oldProfileBtn);
    m_swapMenu->updateLayout();

    if (m_score->isCurrentUser()) {
        auto accountSettingsBtn = Button::createWithSpriteFrameName("accountBtn_settings_001.png", [this](geode::Button* sender) {
            GJAccountSettingsLayer::create(m_score->m_accountID)->show();
        });
        m_userOptionsMenu->addChild(accountSettingsBtn);

        auto friendListBtn = Button::createWithSpriteFrameName("accountBtn_friends_001.png", [this](geode::Button* sender) {
            FriendsProfilePage::create(UserListType::Friends)->show();
        });
        m_userOptionsMenu->addChild(friendListBtn);

        auto friendRequestsBtn = Button::createWithSpriteFrameName("accountBtn_requests_001.png", [this](geode::Button* sender) {
            FRequestProfilePage::create(false)->show();
        });
        m_userOptionsMenu->addChild(friendRequestsBtn);

        // number of friend request
        if (m_score->m_friendReqCount > 0 && m_score->isCurrentUser()) {
            // @geode-ignore(unknown-resource)
            auto reqBadge = CCSprite::createWithSpriteFrameName("geode.loader/updates-failed.png");
            reqBadge->setScale(0.8f);
            reqBadge->setPosition({friendRequestsBtn->getContentSize().width - 5.f, friendRequestsBtn->getContentSize().height - 5.f});
            friendRequestsBtn->addChild(reqBadge);
            auto reqCountLabel = CCLabelBMFont::create(numToString(m_score->m_friendReqCount).c_str(), "bigFont.fnt");
            reqCountLabel->limitLabelWidth(reqBadge->getContentWidth(), .6f, 0.1f);
            reqCountLabel->setPosition({reqBadge->getContentSize().width / 2.f, reqBadge->getContentSize().height / 2.f});
            reqBadge->addChild(reqCountLabel);
        }

        // new friends!
        if (m_score->m_newFriendCount > 0 && m_score->isCurrentUser()) {
            // @geode-ignore(unknown-resource)
            auto newFriendBadge = CCSprite::createWithSpriteFrameName("geode.loader/updates-installed.png");
            newFriendBadge->setPosition({friendListBtn->getContentSize().width - 5.f, friendListBtn->getContentSize().height - 5.f});
            newFriendBadge->setScale(0.8f);
            friendListBtn->addChild(newFriendBadge);
            auto newFriendCountLabel = CCLabelBMFont::create(numToString(m_score->m_newFriendCount).c_str(), "bigFont.fnt");
            newFriendCountLabel->limitLabelWidth(newFriendBadge->getContentWidth(), .6f, 0.1f);
            newFriendCountLabel->setPosition({newFriendBadge->getContentSize().width / 2.f, newFriendBadge->getContentSize().height / 2.f});
            newFriendBadge->addChild(newFriendCountLabel);
        }
    }

    if ((m_score->m_messageState == 0) || (m_score->m_messageState == 1 && m_score->m_friendReqStatus == 1) || (m_score->isCurrentUser())) {
        auto messageBtn = Button::createWithSpriteFrameName("accountBtn_messages_001.png", [this](geode::Button* sender) {
            if (m_score) {
                if (m_ownProfile) {
                    MessagesProfilePage::create(false)->show();
                } else {
                    GJWriteMessagePopup::create(m_score->m_accountID, 0)->show();
                }
            }
        });
        m_userOptionsMenu->addChild(messageBtn);

        // unread messages
        if (m_score->m_newMsgCount > 0 && m_score->isCurrentUser()) {
            // @geode-ignore(unknown-resource)
            auto msgBadge = CCSprite::createWithSpriteFrameName("geode.loader/updates-available.png");
            msgBadge->setScale(0.8f);
            msgBadge->setPosition({messageBtn->getContentSize().width - 5.f, messageBtn->getContentSize().height - 5.f});
            messageBtn->addChild(msgBadge);
            auto msgCountLabel = CCLabelBMFont::create(numToString(m_score->m_newMsgCount).c_str(), "bigFont.fnt");
            msgCountLabel->limitLabelWidth(msgBadge->getContentWidth(), .6f, 0.1f);
            msgCountLabel->setPosition({msgBadge->getContentSize().width / 2.f, msgBadge->getContentSize().height / 2.f});
            msgBadge->addChild(msgCountLabel);
        }
    }

    if (!m_score->isCurrentUser()) {
        auto blockBtn = Button::createWithSpriteFrameName("accountBtn_blocked_001.png", [this](geode::Button* sender) {
            if (m_score) {
                createQuickPopup(
                    "Block user",
                    fmt::format("Are you sure you want to block <cg>{}</c>?\n"
                                "<cg>{}</c> will no longer be able to:\n"
                                "- <cy>View your profile</c>\n"
                                "- <cl>Send messages</c>\n"
                                "- <cp>Send friend requests</c>\n"
                                "- <cr>Messages from this user will be removed</c>",
                        m_score->m_userName,
                        m_score->m_userName),
                    "Back",
                    "Block",
                    [this](auto layer, auto block) {
                        if (block) {
                            auto upopup = UploadActionPopup::create(nullptr, "Blocking user...");
                            if (GameLevelManager::get()->blockUser(m_score->m_accountID)) {
                                upopup->showSuccessMessage("User blocked!");
                            } else {
                                upopup->showFailMessage("Failed to block user");
                            }
                        }
                    });
            }
        });
        m_userOptionsMenu->addChild(blockBtn);
    }

    log::debug("friend status for account {} is {}", m_score->m_accountID, m_score->m_friendStatus);             // 0 = All, 1 = None
    log::debug("friend request status for account {} is {}", m_score->m_accountID, m_score->m_friendReqStatus);  // 0 = Unfriended, 1 = Friended, 3 = Friend request sent to the player, 4 = Friend request received from the player

    if (m_score->m_friendReqStatus == 0 && !m_score->isCurrentUser() && m_score->m_friendStatus == 0) {
        auto addFriendBtn = Button::createWithSpriteFrameName("accountBtn_requests_001.png", [this](geode::Button* sender) {
            if (m_score) {
                if (auto layer = ShareCommentLayer::create("Friend Request", 140, CommentType::FriendRequest, m_score->m_accountID, "")) {
                    layer->show();
                }
            }
        });
        m_userOptionsMenu->addChild(addFriendBtn);
    }

    if (m_score->m_friendReqStatus == 1 && !m_score->isCurrentUser()) {
        auto friendBtn = Button::createWithSpriteFrameName("accountBtn_removeFriend_001.png", [this](geode::Button* sender) {
            if (m_score) {
                createQuickPopup(
                    "Unfriend",
                    fmt::format("Are you sure you want to unfriend <cg>{}</c>?", m_score->m_userName),
                    "Back",
                    "Unfriend",
                    [this](auto layer, auto unfriend) {
                        if (unfriend) {
                            auto upopup = UploadActionPopup::create(nullptr, "Removing friend...");
                            if (GameLevelManager::get()->removeFriend(m_score->m_accountID)) {
                                upopup->showSuccessMessage("Friend removed!");
                            } else {
                                upopup->showFailMessage("Failed to remove friend");
                            }
                        }
                    });
            }
        });
        m_userOptionsMenu->addChild(friendBtn);
    }

    if (m_score->m_friendReqStatus == 3 && !m_score->isCurrentUser() && m_score->m_friendStatus == 0) {
        auto cancelFriendBtn = Button::createWithSpriteFrameName("accountBtn_pendingRequest_001.png", [this](geode::Button* sender) {
            if (m_score) {
                GJFriendRequest* friendObj = GameLevelManager::get()->friendRequestFromAccountID(m_score->m_accountID);
                if (friendObj) FriendRequestPopup::create(friendObj)->show();
            }
        });
        m_userOptionsMenu->addChild(cancelFriendBtn);
    }

    if (m_score->m_friendReqStatus == 4 && !m_score->isCurrentUser() && m_score->m_friendStatus == 0) {
        auto acceptFriendBtn = Button::createWithSpriteFrameName("accountBtn_pending_001.png", [this](geode::Button* sender) {
            if (m_score) {
                createQuickPopup(
                    "Cancel friend request",
                    "Are you sure you want to cancel this friend request?",
                    "Back",
                    "Remove",
                    [this](auto layer, auto remove) {
                        if (remove) {
                            auto glm = GameLevelManager::get();
                            if (glm) {
                                auto upopup = UploadActionPopup::create(nullptr, "Removing friend request...");
                                upopup->show();
                                if (glm->deleteSentFriendRequest(m_score->m_accountID)) {
                                    upopup->showSuccessMessage("Request removed");
                                } else {
                                    upopup->showFailMessage("Failed to remove friend request");
                                }
                            }
                        }
                    });
            }
        });
        m_userOptionsMenu->addChild(acceptFriendBtn);
    }

    if (m_score->isCurrentUser() && m_ownProfile) {
        // @geode-ignore(unknown-resource)
        auto shareCommentBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("geode.loader/message.png"), [this](geode::Button* sender) {
            if (m_score) {
                if (auto layer = ShareCommentLayer::create("Post Account Update", 140, CommentType::Account, m_score->m_accountID, "")) {
                    layer->m_delegate = this;
                    layer->show();
                }
            }
        });
        m_userOptionsMenu->addChild(shareCommentBtn);
    }

    m_userOptionsMenu->updateLayout();

    // bottom left menu
    if ((m_score->m_commentHistoryStatus == 0) || (m_score->m_commentHistoryStatus == 1 && m_score->m_friendReqStatus == 1)) {
        auto commentHistoryBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("particle_206_001.png"), [this](geode::Button* sender) {
            if (m_score) {
                onCommentHistory(sender);
            }
        });
        m_otherOptionsMenu->addChild(commentHistoryBtn);
        m_otherOptionsMenu->updateLayout();
    }

    if (!m_score->isCurrentUser()) {
        auto glm = GameLevelManager::get();
        bool isFollowing = glm ? glm->isFollowingUser(m_score->m_accountID) : false;
        auto followSprite = isFollowing ? AccountButtonSprite::createWithSpriteFrameName("gj_heartOn_001.png", 1.f, AccountBaseColor::Blue) : AccountButtonSprite::createWithSpriteFrameName("gj_heartOff_001.png", 1.f, AccountBaseColor::Gray);
        auto followUserBtn = Button::createWithNode(followSprite, [this](geode::Button* sender) {
            onFollowUser(sender);
        });
        m_otherOptionsMenu->addChild(followUserBtn);
        m_otherOptionsMenu->updateLayout();
    }

    // @geode-ignore(unknown-resource)
    auto refreshBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("geode.loader/reload.png", 1.f, AccountBaseColor::Gray), [this](geode::Button* sender) {
        this->refreshUserInfoUI();
        this->refreshComments();
    });
    refreshBtn->setScale(0.6f);
    m_buttonMenu->addChildAtPosition(refreshBtn, Anchor::BottomLeft, {0.f, 0.f});

    // @geode-ignore(unknown-resource)
    auto modSettingsBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("geode.loader/settings.png", 1.f, AccountBaseColor::Purple), [this](geode::Button* sender) {
        openSettingsPopup(getMod());
    });
    modSettingsBtn->setScale(0.6f);
    m_buttonMenu->addChildAtPosition(modSettingsBtn, Anchor::BottomLeft, {30.f, 0.f});

    // middle right menu
    if (m_score->m_youtubeURL.size() > 0) {
        auto youtubeBtn = Button::createWithSpriteFrameName("gj_ytIcon_001.png", [this](geode::Button* sender) {
            utils::web::openLinkInBrowser(std::string("https://youtube.com/channel/") + m_score->m_youtubeURL.c_str());
        });
        m_socialsMenu->addChild(youtubeBtn);
    }

    if (m_score->m_twitterURL.size() > 0) {
        auto twitterBtn = Button::createWithSpriteFrameName("gj_twIcon_001.png", [this](geode::Button* sender) {
            utils::web::openLinkInBrowser(std::string("https://twitter.com/") + m_score->m_twitterURL.c_str());
        });
        m_socialsMenu->addChild(twitterBtn);
    }

    if (m_score->m_twitchURL.size() > 0) {
        auto twitchBtn = Button::createWithSpriteFrameName("gj_twitchIcon_001.png", [this](geode::Button* sender) {
            utils::web::openLinkInBrowser(std::string("https://twitch.tv/") + m_score->m_twitchURL.c_str());
        });
        m_socialsMenu->addChild(twitchBtn);
    }

    if (m_score->m_instagramURL.size() > 0) {
        auto instagramBtn = Button::createWithSpriteFrameName("gj_instaIcon_001.png", [this](geode::Button* sender) {
            utils::web::openLinkInBrowser(std::string("https://instagram.com/") + m_score->m_instagramURL.c_str());
        });
        m_socialsMenu->addChild(instagramBtn);
    }

    if (m_score->m_tiktokURL.size() > 0) {
        auto tiktokBtn = Button::createWithSpriteFrameName("gj_tiktokIcon_001.png", [this](geode::Button* sender) {
            utils::web::openLinkInBrowser(std::string("https://tiktok.com/@") + m_score->m_tiktokURL.c_str());
        });
        m_socialsMenu->addChild(tiktokBtn);
    }

    m_socialsMenu->updateLayout();

    // bottom right menu
    auto listAccountBtn = Button::createWithSpriteFrameName("accountBtn_myLists_001.png", [this](geode::Button* sender) {
        onList(sender);
    });
    m_onlineMenu->addChild(listAccountBtn);

    auto myLevelBtn = Button::createWithSpriteFrameName("accountBtn_myLevels_001.png", [this](geode::Button* sender) {
        onLevel(sender);
    });
    m_onlineMenu->addChild(myLevelBtn);

    m_onlineMenu->updateLayout();

    // leaderboard stats
    log::debug("player rank is {}", m_score->m_playerRank);
    log::debug("stars rank is {}", m_score->m_globalRank);
    //log::debug("moon stuff {}", GameLevelManager::get()->getLeaderboardScore(LeaderboardType::Global, LeaderboardStat::Moons));
    if (m_score->m_globalRank > 0) {
        auto starsLeaderboardBtn = Button::createWithLabel(fmt::format("Stars Rank:\n#{}", m_score->m_globalRank), "chatFont.fnt", [this](geode::Button* sender) {
            if (!m_score) return;
            if (!m_score->isCurrentUser()) return;
            m_score->m_leaderboardStat = LeaderboardStat::Stars;
            auto scene = CCScene::create();
            auto layer = LeaderboardsLayer::create(LeaderboardType::Global, m_score->m_leaderboardStat);
            scene->addChild(layer);
            CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
        });
        if (!m_score->isCurrentUser()) starsLeaderboardBtn->setEnabled(false);
        starsLeaderboardBtn->setColor({233, 253, 113});
        auto starsRankSprite = CCSprite::createWithSpriteFrameName(getRankIconForGlobalRank(m_score->m_globalRank).c_str());
        starsRankSprite->setPosition({-5, starsLeaderboardBtn->getContentSize().height / 2.f});
        starsRankSprite->setAnchorPoint({1.f, 0.5f});
        starsLeaderboardBtn->addChild(starsRankSprite);
        m_leaderboardMenu->addChild(starsLeaderboardBtn);

        if (m_score->isCurrentUser() && m_moonsRankLoaded && m_moonsRank > 0) {
            auto moonsLeaderboardBtn = Button::createWithLabel(fmt::format("Moons Rank:\n#{}", m_moonsRankLoaded ? m_moonsRank : 0), "chatFont.fnt", [this](geode::Button* sender) {
                if (!m_score) return;
                if (!m_score->isCurrentUser()) return;
                m_score->m_leaderboardStat = LeaderboardStat::Moons;
                auto scene = CCScene::create();
                auto layer = LeaderboardsLayer::create(LeaderboardType::Global, m_score->m_leaderboardStat);
                scene->addChild(layer);
                CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
            });

            if (!m_score->isCurrentUser()) moonsLeaderboardBtn->setEnabled(false);
            moonsLeaderboardBtn->setColor({109, 215, 249});
            auto moonsRankSprite = CCSprite::createWithSpriteFrameName(getRankIconForGlobalRank(m_moonsRank).c_str());
            moonsRankSprite->setPosition({-5, moonsLeaderboardBtn->getContentSize().height / 2.f});
            moonsRankSprite->setAnchorPoint({1.f, 0.5f});
            moonsLeaderboardBtn->addChild(moonsRankSprite);

            m_leaderboardMenu->addChild(moonsLeaderboardBtn);
        }
    }
    m_leaderboardMenu->updateLayout();

    refreshRatedLevelCell();
}

void ProfilePopup::refreshRatedLevelCell() {
    if (!m_ratedLevelCell) {
        return;
    }

    m_refreshScheduled = false;

    if (!m_score) {
        return;
    }

    if (m_levelCell) {
        return;
    }

    if (m_spinner) {
        m_spinner->removeFromParent();
        m_spinner = nullptr;
    }

    if (m_noneLabel) {
        m_noneLabel->removeFromParent();
        m_noneLabel = nullptr;
    }
    m_ratedLevelCell->clear();

    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    auto searchObj = GJSearchObject::create(SearchType::UsersLevels, numToString(m_score->m_userID));
    searchObj->m_searchMode = 0;
    searchObj->m_page = m_ratedLevelPage;
    auto key = searchObj->getKey();
    CCArray* levels = nullptr;
    const bool showLatestLevel = m_score->m_creatorPoints == 0;

    if (key && key[0] && m_ratedLevelSearchKey != key) {
        m_ratedLevelFetchCompleted = false;
        m_ratedLevelFetchFailed = false;
        m_ratedLevelSearchKey = key;
    }

    if (key && key[0]) {
        levels = glm->getStoredOnlineLevels(key);
    }

    if (!levels || levels->count() == 0) {
        if (m_ratedLevelSearchKey == key && (m_ratedLevelFetchCompleted || m_ratedLevelFetchFailed)) {
            showNoRatedLevelLabel(showLatestLevel);
            return;
        }

        if (key && key[0]) {
            glm->getOnlineLevels(searchObj);
        }

        m_spinner = LoadingSpinner::create(35.f);
        if (m_spinner) {
            m_spinner->setAnchorPoint({0.5f, 0.5f});
            m_spinner->setPosition({m_ratedLevelCell->getContentSize().width / 2.f, m_ratedLevelCell->getContentSize().height / 2.f});
            m_ratedLevelCell->addChild(m_spinner);
        }

        if (!m_refreshScheduled) {
            m_refreshScheduled = true;
            auto delay = CCDelayTime::create(1.0f);
            auto callback = CCCallFunc::create(this, callfunc_selector(ProfilePopup::refreshRatedLevelCell));
            auto sequence = CCSequence::create(delay, callback, nullptr);
            this->runAction(sequence);
        }
        return;
    }

    GJGameLevel* ratedLevel = nullptr;
    for (int i = 0; i < levels->count(); ++i) {
        auto level = typeinfo_cast<GJGameLevel*>(levels->objectAtIndex(i));
        if (!level) {
            continue;
        }
        if (!showLatestLevel && level->m_stars.value() <= 0) {
            continue;
        }

        ratedLevel = level;
        break;
    }

    if (!ratedLevel) {
        if (m_score->m_creatorPoints > 0) {
            ++m_ratedLevelPage;
            m_ratedLevelSearchKey.clear();
            m_ratedLevelFetchCompleted = false;
            m_ratedLevelFetchFailed = false;
            refreshRatedLevelCell();
            return;
        }

        showNoRatedLevelLabel(showLatestLevel);
        return;
    }

    m_levelCell = LevelCell::create(m_ratedLevelCell->getContentSize().width, m_ratedLevelCell->getContentSize().height);
    if (!m_levelCell) {
        return;
    }

    m_levelCell->setUserFlag("profile-overhaul-level-cell");
    m_levelCell->setVisible(true);

    m_levelCell->setContentSize(m_ratedLevelCell->getContentSize());
    m_levelCell->setAnchorPoint({0.5f, 0.5f});
    m_levelCell->loadFromLevel(ratedLevel);
    if (m_levelCell->m_mainMenu) {
        auto creatorName = m_levelCell->m_mainMenu->getChildByID("creator-name");
        if (creatorName) creatorName->setVisible(false);
    }
    if (m_levelCell->m_mainLayer) {
        auto levelName = m_levelCell->m_mainLayer->getChildByID("level-name");
        auto songName = m_levelCell->m_mainLayer->getChildByID("song-name");
        auto difficultyContainer = m_levelCell->m_mainLayer->getChildByID("difficulty-container");
        auto completedIcon = m_levelCell->m_mainLayer->getChildByID("completed-icon");
        auto copyIndicator = m_levelCell->m_mainLayer->getChildByID("copy-indicator");
        auto highObjIndicator = m_levelCell->m_mainLayer->getChildByID("high-object-indicator");
        auto percentageLabel = m_levelCell->m_mainLayer->getChildByID("percentage-label");
        auto ncsLogo = m_levelCell->m_mainLayer->getChildByID("ncs-icon");
        auto chompoLogo = m_levelCell->m_mainLayer->getChildByID("chompo-icon");
        if (levelName) levelName->setPositionY(levelName->getPositionY() - 10.f);
        if (songName) songName->setPosition({songName->getPositionX() - 2.f, songName->getPositionY() + 11.f});
        if (difficultyContainer) difficultyContainer->setScale(difficultyContainer->getScale() - 0.2f);
        if (completedIcon) completedIcon->setVisible(false);
        if (copyIndicator) copyIndicator->setVisible(false);
        if (highObjIndicator) highObjIndicator->setVisible(false);
        if (percentageLabel) percentageLabel->setPositionY(percentageLabel->getPositionY() - 10.f);
        if (ncsLogo) ncsLogo->setPositionY(ncsLogo->getPositionY() + 10.f);
        if (chompoLogo) chompoLogo->setPositionY(chompoLogo->getPositionY() + 10.f);
    }
    if (m_spinner) {
        m_spinner->removeFromParent();
        m_spinner = nullptr;
    }
    m_ratedLevelCell->addCell(m_levelCell);
    m_ratedLevelCell->updateLayout();
}

void ProfilePopup::showNoRatedLevelLabel(bool showLatestLevel) {
    if (m_spinner) {
        m_spinner->removeFromParent();
        m_spinner = nullptr;
    }
    if (m_noneLabel) {
        m_noneLabel->removeFromParent();
        m_noneLabel = nullptr;
    }

    m_noneLabel = CCLabelBMFont::create(showLatestLevel ? "No level found" : "No rated level", "goldFont.fnt");
    m_noneLabel->setScale(0.5f);
    m_noneLabel->setAnchorPoint({0.5f, 0.5f});
    m_noneLabel->setPosition({m_ratedLevelCell->getContentSize().width / 2.f, m_ratedLevelCell->getContentSize().height / 2.f});
    m_ratedLevelCell->addChild(m_noneLabel);
    m_ratedLevelCell->updateLayout();
}

void ProfilePopup::requestMoonLeaderboardRank() {
    if (!m_score) {
        return;
    }

    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    m_moonsRankLoaded = false;
    m_moonsRank = 0;
    glm->getLeaderboardScores(LeaderboardType::Global, LeaderboardStat::Moons);
}

void ProfilePopup::getUserInfoFinished(GJUserScore* score) {
    m_score = score;
    refreshUserInfoUI();
    requestMoonLeaderboardRank();
}

void ProfilePopup::loadLeaderboardFinished(cocos2d::CCArray* scores, char const* key) {
    if (!scores || !key || !m_score) {
        return;
    }

    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    if (std::string(key) != glm->getLeaderboardKey(LeaderboardType::Global, LeaderboardStat::Moons)) {
        return;
    }

    log::debug("moon leaderboard finished: scores={}, key={}", scores->count(), key);
    int foundRank = 0;
    for (auto i = 0u; i < scores->count(); ++i) {
        auto object = scores->objectAtIndex(i);
        if (!object) {
            continue;
        }

        GJUserScore* score = nullptr;
        if (auto userScore = typeinfo_cast<GJUserScore*>(object)) {
            score = GJUserScore::create();
            if (score) {
                score->m_userID = userScore->m_userID;
                score->m_accountID = userScore->m_accountID;
                score->m_globalRank = userScore->m_globalRank;
                score->m_playerRank = userScore->m_playerRank;
            }
        } else if (auto dict = typeinfo_cast<CCDictionary*>(object)) {
            score = GJUserScore::create(dict);
        }

        if (!score) {
            continue;
        }

        if (score->m_userID == m_score->m_userID || score->m_accountID == m_score->m_accountID) {
            foundRank = score->m_globalRank > 0 ? score->m_globalRank : score->m_playerRank;
            break;
        }
    }

    m_moonsRankLoaded = true;
    m_moonsRank = foundRank;
    refreshUserInfoUI();
}

void ProfilePopup::loadLeaderboardFailed(char const* key) {
    if (!key) {
        return;
    }

    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    if (std::string(key) != glm->getLeaderboardKey(LeaderboardType::Global, LeaderboardStat::Moons)) {
        return;
    }

    m_moonsRankLoaded = true;
    m_moonsRank = 0;
    refreshUserInfoUI();
}

void ProfilePopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* key) {
    if (!key || key[0] == '\0') {
        return;
    }

    if (m_ratedLevelSearchKey != key) {
        return;
    }

    m_ratedLevelFetchCompleted = true;
    m_ratedLevelFetchFailed = false;
    refreshRatedLevelCell();
}

void ProfilePopup::loadLevelsFailed(char const* key) {
    if (!key || key[0] == '\0') {
        return;
    }

    if (m_ratedLevelSearchKey != key) {
        return;
    }

    m_ratedLevelFetchCompleted = true;
    m_ratedLevelFetchFailed = true;
    refreshRatedLevelCell();
}

// profile page own implemetation
void ProfilePopup::onInfo(CCObject* sender) {
    if (!m_score) {
        return;
    }

    auto message = fmt::format(
        "<cl>AccountID:</c> {}\n<cd>UserID:</c> {}\n<cy>Stars:</c> {}\n<cb>Moons:</c> {}\n<cf>Diamonds:</c> {}\n<co>Secret Coins:</c> {}\n<cc>User Coins:</c> {}\n<cr>Demons:</c> {}",
        m_score->m_accountID,
        m_score->m_userID,
        GameToolbox::pointsToString(m_score->m_stars),
        GameToolbox::pointsToString(m_score->m_moons),
        GameToolbox::pointsToString(m_score->m_diamonds),
        GameToolbox::pointsToString(m_score->m_secretCoins),
        GameToolbox::pointsToString(m_score->m_userCoins),
        GameToolbox::pointsToString(m_score->m_demons));

    if (m_score->m_creatorPoints > 0) {
        message += fmt::format("\n<cg>Creator Points:</c> {}", m_score->m_creatorPoints);
    }

    if (m_score->m_discordUsername.size() > 0) {
        message += fmt::format("\n<cj>Discord:</c> @{}", std::string(m_score->m_discordUsername));
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

    log::debug("account comments page {} returned {} items", m_commentPage, comments->count());

    if (this->m_commentPage == 0) {
        this->m_commentsList->clear();
        if (this->m_commentsSpinner) {
            this->m_commentsSpinner->removeFromParent();
            this->m_commentsSpinner = nullptr;
        }
        if (this->m_noCommentsLabel) {
            this->m_noCommentsLabel->removeFromParent();
            this->m_noCommentsLabel = nullptr;
        }
    }

    const auto width = m_commentsList->getListSize().width;

    for (auto i = 0u; i < comments->count(); ++i) {
        auto commentObj = comments->objectAtIndex(i);
        if (!commentObj) {
            log::debug("comment[{}] is null", i);
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
        if (!cell->init()) {
            continue;
        }

        cell->setUserFlag("profile-overhaul-comment-cell");
        cell->setVisible(true);

        cell->setContentHeight(85);
        cell->loadFromComment(comment);
        cell->m_backgroundLayer->setVisible(false);
        cell->m_accountComment = true;
        m_commentsList->addCell(cell);
        if (auto usernameLabel = cell->m_mainLayer->getChildByID("username-label")) usernameLabel->setVisible(false);
        if (auto dateLabel = cell->m_mainLayer->getChildByID("date-label")) {
            dateLabel->setAnchorPoint({0.f, 0.5f});
            dateLabel->setPosition({10.f, cell->getContentSize().height - 15.f});
            dateLabel->setScale(.7f);
        }
    }

    this->m_commentsList->updateLayout();

    if (comments->count() == 0 && this->m_commentPage == 0) {
        if (!this->m_noCommentsLabel) {
            this->m_noCommentsLabel = CCLabelBMFont::create("No Account Posts", "goldFont.fnt");
            this->m_noCommentsLabel->setScale(0.5f);
            this->m_noCommentsLabel->setAnchorPoint({0.5f, 0.5f});
            this->m_noCommentsLabel->setPosition({this->m_commentsList->getContentSize().width / 2.f, this->m_commentsList->getContentSize().height / 2.f});
            this->m_commentsList->addChild(this->m_noCommentsLabel);
        }
        if (this->m_commentsSpinner) {
            this->m_commentsSpinner->removeFromParent();
            this->m_commentsSpinner = nullptr;
        }
        this->m_commentsList->updateLayout();
        return;
    }

    if (this->m_commentsSpinner) {
        this->m_commentsSpinner->removeFromParent();
        this->m_commentsSpinner = nullptr;
    }

    if (comments->count() >= static_cast<size_t>(m_commentPageSize)) {
        ++m_commentPage;
        requestAccountCommentsPage(m_commentPage);
    } else {
        log::debug("account comments finished after page {}", m_commentPage);
    }
}

void ProfilePopup::refreshComments() {
    if (!this->m_commentsList) {
        return;
    }

    this->m_commentPage = 0;
    this->m_commentsList->clear();
    if (this->m_commentsSpinner) {
        this->m_commentsSpinner->removeFromParent();
        this->m_commentsSpinner = nullptr;
    }
    if (this->m_noCommentsLabel) {
        this->m_noCommentsLabel->removeFromParent();
        this->m_noCommentsLabel = nullptr;
    }

    this->m_commentsSpinner = LoadingSpinner::create(35.f);
    if (this->m_commentsSpinner) {
        this->m_commentsSpinner->setAnchorPoint({0.5f, 0.5f});
        this->m_commentsSpinner->setPosition({this->m_commentsList->getContentSize().width / 2.f, this->m_commentsList->getContentSize().height / 2.f});
        this->m_commentsList->addChild(this->m_commentsSpinner);
    }
    this->m_commentsList->updateLayout();
    this->requestAccountCommentsPage(0);
}

void ProfilePopup::requestAccountCommentsPage(int page) {
    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }
    log::debug("requesting account comments page {} for account {}", page, m_accountId);
    glm->getAccountComments(m_accountId, page, m_commentPageSize);
}

void ProfilePopup::loadCommentsFailed(char const* key) {
    log::error("account comments load failed for key {}", key ? key : "<null>");

    if (this->m_commentsSpinner) {
        this->m_commentsSpinner->removeFromParent();
        this->m_commentsSpinner = nullptr;
    }

    if (!this->m_commentsList) {
        return;
    }

    if (this->m_commentPage == 0) {
        if (!this->m_noCommentsLabel) {
            this->m_noCommentsLabel = CCLabelBMFont::create("No Account Posts", "goldFont.fnt");
            this->m_noCommentsLabel->setScale(0.5f);
            this->m_noCommentsLabel->setAnchorPoint({0.5f, 0.5f});
            this->m_noCommentsLabel->setPosition({this->m_commentsList->getContentSize().width / 2.f, this->m_commentsList->getContentSize().height / 2.f});
            this->m_commentsList->addChild(this->m_noCommentsLabel);
        }
        this->m_commentsList->updateLayout();
    }
}

void ProfilePopup::shareCommentClosed(gd::string text, ShareCommentLayer* layer) {
    if (!Ref<ProfilePopup>(this) || !m_commentsList || !layer) {
        return;
    }
    // if (layer->m_commentType != CommentType::Account) {
    //     return;
    // }

    log::info("share comment closed, refreshing comment list");
    refreshComments();
}

void ProfilePopup::userInfoChanged(GJUserScore* score) {
    if (!score) {
        return;
    }
    if (score != m_score && score->m_accountID != m_accountId) {
        return;
    }

    m_score = score;
    log::info("user score changed for account id {}", m_score->m_accountID);
    refreshUserInfoUI();
}

void ProfilePopup::onList(CCObject* sender) {
    if (!m_score) return;
    GJSearchObject* searchObj = GJSearchObject::create(SearchType::UsersLevels, numToString(m_score->m_accountID));
    searchObj->m_searchMode = 1;  // 1 = lists, 0 = levels
    auto scene = CCScene::create();
    auto levelLayer = LevelBrowserLayer::create(searchObj);
    scene->addChild(levelLayer);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void ProfilePopup::onLevel(CCObject* sender) {
    if (!m_score) return;
    GJSearchObject* searchObj = GJSearchObject::create(SearchType::UsersLevels, numToString(m_score->m_userID));
    auto scene = CCScene::create();
    auto levelLayer = LevelBrowserLayer::create(searchObj);
    scene->addChild(levelLayer);
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void ProfilePopup::onCommentHistory(CCObject* sender) {
    if (!m_score) return;
    // show the user comment history
    InfoLayer::create(nullptr, m_score, nullptr)->show();
}

void ProfilePopup::onFollowUser(CCObject* sender) {
    if (!m_score) return;
    if (m_score->isCurrentUser()) {
        log::error("cannot follow/unfollow yourself");
        return;
    }
    auto glm = GameLevelManager::get();
    if (!glm) return;
    // follow/unfollow the user
    if (glm->isFollowingUser(m_score->m_accountID)) {
        glm->unfollowUser(m_score->m_accountID);
    } else {
        glm->followUser(m_score->m_accountID);
    }

    refreshUserInfoUI();
}