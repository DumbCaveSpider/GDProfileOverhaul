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

// temp solution for missing dtor for ccindexpath in 2.2091 bindings
template <class T>
static T* createListCell(CustomListView* list, char const* identifier, float height) {
    auto cell = static_cast<T*>(list->getListCell(identifier));
    cell->autorelease();
    cell->m_height = height;
    cell->setContentHeight(height);
    if (cell->m_backgroundLayer) cell->m_backgroundLayer->setContentHeight(height);
    return cell->init() ? cell : nullptr;
}

static bool hasAccount(char const* action) {
    if (GJAccountManager::get()->m_accountID > 0) return true;
    FLAlertLayer::create("Account needed", fmt::format("You need an <cg>account</c> to {}.", action), "OK")->show();
    return false;
}

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
        if (glm->m_uploadActionDelegate == this) {
            glm->m_uploadActionDelegate = nullptr;
        }
        if (glm->m_commentUploadDelegate == this) {
            glm->m_commentUploadDelegate = nullptr;
        }
    }
    if (m_profilePopup == this) {
        m_profilePopup = nullptr;
    }
    if (m_uploadPopup) {
        m_uploadPopup->m_delegate = nullptr;
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
        if (currentUserId != accountId) ownProfile = false;
    }

    m_ownProfile = ownProfile;
    m_accountId = accountId;

    // i hate these stuff
    //m_noElasticity = true;
    if (m_buttonMenu) m_buttonMenu->removeAllChildren();

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

    Ref<geode::Button> closeBtn = m_closeBtn;
    closeBtn->removeFromParentAndCleanup(false);
    m_closeMenu->addChild(closeBtn);
    m_closeMenu->updateLayout();

    m_userOptionsMenu = CCMenu::create();
    m_userOptionsMenu->setContentSize({50, 180});
    m_userOptionsMenu->setID("user-options-menu");
    m_userOptionsMenu->m_bIgnoreAnchorPointForPosition = false;
    m_userOptionsMenu->setLayout(ColumnLayout::create()
            ->setCrossAxisOverflow(false)
            ->setAxisReverse(true)
            ->setGrowCrossAxis(true)
            ->setGap(0.f));
    m_userOptionsMenu->setZOrder(1);
    m_userOptionsMenu->setScale(0.75f);
    m_mainLayer->addChildAtPosition(m_userOptionsMenu, Anchor::Left, {30.f, 19.f}, {0.5, 0.5}, false);

    auto userOptionsMenuBg = NineSlice::create("square02_small.png");
    userOptionsMenuBg->setContentSize(m_userOptionsMenu->getContentSize() - CCSize{10, 40});
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

    m_statusLabel = geode::Label::create("", "goldFont.fnt");
    m_statusLabel->setID("status-label");
    m_statusLabel->setScale(0.5f);
    m_statusLabel->setAnchorPoint({0.5f, 0.5f});
    m_statusLabel->setPosition(m_commentsList->getContentSize() / 2.f);
    m_statusLabel->setVisible(false);
    m_commentsList->addChild(m_statusLabel);

    m_levelCellBorder = cue::ListBorder::create(cue::ListBorderStyle::Comments, {325, 70}, ccColor4B{191, 114, 62, 255});
    m_levelCellBorder->setID("level-cell-border");
    m_levelCellBorder->setZOrder(2);
    m_mainLayer->addChildAtPosition(m_levelCellBorder, Anchor::Bottom, {0.f, 50.f}, {0.5, 0.5}, false);

    m_levelContainer = CCNode::create();
    m_levelContainer->setContentSize(m_levelCellBorder->getContentSize());
    m_levelContainer->setAnchorPoint({0.5f, 0.5f});
    m_levelContainer->setPosition({0, 0});
    m_levelCellBorder->addChild(m_levelContainer, -1);

    auto levelCellBg = CCLayerColor::create({191, 114, 62, 255}, m_levelCellBorder->getContentSize().width, m_levelCellBorder->getContentSize().height);
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

    m_score = nullptr;

    // get the user's score info and account comments
    auto glm = GameLevelManager::get();
    if (glm) {
        glm->m_userInfoDelegate = this;
        glm->m_levelCommentDelegate = this;
        glm->m_levelManagerDelegate = this;
        glm->m_leaderboardManagerDelegate = this;
        glm->getGJUserInfo(accountId);
        loadCommentPage(0);
        m_score = glm->userInfoForAccountID(accountId);
    }

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

    const bool isBlocked = m_score->m_friendReqStatus == 2;
    m_statsMenu->setVisible(!isBlocked);
    if (isBlocked) {
        if (m_commentsList) m_commentsList->clear();
        showCommentsStatus();
        m_commentsHiddenByBlock = true;
    } else if (m_commentsHiddenByBlock) {
        // nothing was loaded while blocked, so the posts have to be fetched now
        m_commentsHiddenByBlock = false;
        refreshComments();
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
    auto infoBtn = geode::Button::createWithNode(infoSpr, [this](geode::Button* sender) {
        this->onInfo(sender);
    });
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

    auto starsLabel = geode::Label::create(GameToolbox::pointsToString(m_score->m_stars), "bigFont.fnt");
    starsLabel->setScale(0.4f);
    starsLabel->setColor({233, 253, 113});
    m_statsMenu->addChild(starsLabel);

    auto starsIcon = Button::createWithSpriteFrameName("GJ_starsIcon_001.png", [this](auto btn) {
        StarInfoPopup::createFromString(m_score->m_starsInfo)->show();
    });
    starsIcon->setScale(0.5f);
    m_statsMenu->addChild(starsIcon);

    auto moonLabel = geode::Label::create(GameToolbox::pointsToString(m_score->m_moons), "bigFont.fnt");
    moonLabel->setScale(0.4f);
    moonLabel->setColor({109, 215, 249});
    m_statsMenu->addChild(moonLabel);

    auto moonIcon = Button::createWithSpriteFrameName("GJ_moonsIcon_001.png", [this](auto btn) {
        StarInfoPopup::createFromStringMoons(m_score->m_platformerInfo)->show();
    });
    moonIcon->setScale(0.5f);
    m_statsMenu->addChild(moonIcon);

    auto demonLabel = geode::Label::create(GameToolbox::pointsToString(m_score->m_demons), "bigFont.fnt");
    demonLabel->setScale(0.4f);
    demonLabel->setColor({240, 140, 140});
    m_statsMenu->addChild(demonLabel);

    auto demonIcon = Button::createWithSpriteFrameName("GJ_demonIcon_001.png", [this](auto btn) {
        DemonInfoPopup::createFromString(m_score->m_demonInfo)->show();
    });
    demonIcon->setScale(0.5f);
    m_statsMenu->addChild(demonIcon);

    if (m_score->m_creatorPoints > 0) {
        auto cpLabel = geode::Label::create(GameToolbox::pointsToString(m_score->m_creatorPoints), "bigFont.fnt");
        cpLabel->setScale(0.4f);
        cpLabel->setColor({182, 186, 186});
        m_statsMenu->addChild(cpLabel);

        auto cpIcon = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");
        cpIcon->setScale(0.5f);
        m_statsMenu->addChild(cpIcon);
    }

    auto userCoinsLabel = geode::Label::create(GameToolbox::pointsToString(m_score->m_userCoins), "bigFont.fnt");
    userCoinsLabel->setScale(0.4f);
    userCoinsLabel->setColor({255, 255, 255});
    m_statsMenu->addChild(userCoinsLabel);

    auto userCoinsIcon = CCSprite::createWithSpriteFrameName("GJ_coinsIcon2_001.png");
    userCoinsIcon->setScale(0.5f);
    m_statsMenu->addChild(userCoinsIcon);

    auto secretCoinsLabel = geode::Label::create(GameToolbox::pointsToString(m_score->m_secretCoins), "bigFont.fnt");
    secretCoinsLabel->setScale(0.4f);
    secretCoinsLabel->setColor({248, 138, 0});
    m_statsMenu->addChild(secretCoinsLabel);

    auto secretCoinsIcon = CCSprite::createWithSpriteFrameName("GJ_coinsIcon_001.png");
    secretCoinsIcon->setScale(0.5f);
    m_statsMenu->addChild(secretCoinsIcon);

    if (Mod::get()->getSettingValue<bool>("showDiamondStats")) {
        auto diamondsLabel = geode::Label::create(GameToolbox::pointsToString(m_score->m_diamonds), "bigFont.fnt");
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

    // same rules vanilla ProfilePage uses: 0218 / 0219 are the parental safe and offline modes, 0243 disables messaging
    auto gm = GameManager::get();
    const bool safeOrOffline = gm->getGameVariable("0218") || gm->getGameVariable("0219");
    const bool noMessaging = safeOrOffline || gm->getGameVariable("0243");

    log::debug("friend status for account {} is {}", m_score->m_accountID, m_score->m_friendStatus);             // 1 = Not accepting requests, 2 = Accepting requests with messages
    log::debug("friend request status for account {} is {}", m_score->m_accountID, m_score->m_friendReqStatus);  // 0 = Not friends, 1 = Friends, 2 = Blocked by you, 3 = They sent you a request, 4 = You sent them a request

    if (m_score->isCurrentUser()) {
        if (!safeOrOffline) {
            auto accountSettingsBtn = Button::createWithSpriteFrameName("accountBtn_settings_001.png", [this](geode::Button* sender) {
                GJAccountSettingsLayer::create(m_score->m_accountID)->show();
            });
            m_userOptionsMenu->addChild(accountSettingsBtn);
        }

        auto notificationBtn = Button::createWithSpriteFrameName("accountBtn_notifications_001.png", [this](geode::Button* sender) {
            //NotificationsPage::create()->show();
        });
        m_userOptionsMenu->addChild(notificationBtn);

        auto friendListBtn = Button::createWithSpriteFrameName("accountBtn_friends_001.png", [this](geode::Button* sender) {
            FriendsProfilePage::create(UserListType::Friends)->show();
        });
        m_userOptionsMenu->addChild(friendListBtn);

        // new friends!
        if (m_score->m_newFriendCount > 0) {
            // @geode-ignore(unknown-resource)
            auto newFriendBadge = CCSprite::createWithSpriteFrameName("geode.loader/updates-installed.png");
            newFriendBadge->setPosition({friendListBtn->getContentSize().width - 5.f, friendListBtn->getContentSize().height - 5.f});
            newFriendBadge->setScale(0.8f);
            friendListBtn->addChild(newFriendBadge);
            auto newFriendCountLabel = geode::Label::create(numToString(m_score->m_newFriendCount), "bigFont.fnt");
            newFriendCountLabel->setLimitLabelWidth(newFriendBadge->getContentWidth(), .6f, 0.1f);
            newFriendCountLabel->setPosition({newFriendBadge->getContentSize().width / 2.f, newFriendBadge->getContentSize().height / 2.f});
            newFriendBadge->addChild(newFriendCountLabel);
        }

        if (!noMessaging) {
            auto friendRequestsBtn = Button::createWithSpriteFrameName("accountBtn_requests_001.png", [this](geode::Button* sender) {
                //FRequestProfilePage::create(false)->show(); // not added in bindings yet
            });
            m_userOptionsMenu->addChild(friendRequestsBtn);

            // number of friend request
            if (m_score->m_friendReqCount > 0) {
                // @geode-ignore(unknown-resource)
                auto reqBadge = CCSprite::createWithSpriteFrameName("geode.loader/updates-failed.png");
                reqBadge->setScale(0.8f);
                reqBadge->setPosition({friendRequestsBtn->getContentSize().width - 5.f, friendRequestsBtn->getContentSize().height - 5.f});
                friendRequestsBtn->addChild(reqBadge);
                auto reqCountLabel = geode::Label::create(numToString(m_score->m_friendReqCount), "bigFont.fnt");
                reqCountLabel->setLimitLabelWidth(reqBadge->getContentWidth(), .6f, 0.1f);
                reqCountLabel->setPosition({reqBadge->getContentSize().width / 2.f, reqBadge->getContentSize().height / 2.f});
                reqBadge->addChild(reqCountLabel);
            }
        }
    }

    const bool blocked = m_score->m_friendReqStatus == 2;
    const int modLevel = gm->m_hasRP.value();
    const bool canMessageUser = m_score->m_messageState != 2 && (m_score->m_messageState != 1 || m_score->m_friendReqStatus == 1 || modLevel == 1 || modLevel == 2);

    if (!noMessaging && (m_score->isCurrentUser() || (!blocked && canMessageUser))) {
        auto messageBtn = Button::createWithSpriteFrameName("accountBtn_messages_001.png", [this](geode::Button* sender) {
            if (!m_score) return;
#if defined(GEODE_IS_INTEL_MAC) || defined(GEODE_IS_IOS)
            FLAlertLayer::create("Messages", "Opening messages from here isn't available on this platform yet.", "OK")->show();
#else
            if (m_score->isCurrentUser()) {
                MessageThreadsPage::create()->show();
                return;
            }
            if (!hasAccount("send <cg>messages</c>")) return;
            NewThreadPopup::create(m_score->m_accountID, "", m_score->m_userName)->show();
#endif
        });
        m_userOptionsMenu->addChild(messageBtn);

        // unread messages
        if (m_score->m_newMsgCount > 0 && m_score->isCurrentUser()) {
            // @geode-ignore(unknown-resource)
            auto msgBadge = CCSprite::createWithSpriteFrameName("geode.loader/updates-available.png");
            msgBadge->setScale(0.8f);
            msgBadge->setPosition({messageBtn->getContentSize().width - 5.f, messageBtn->getContentSize().height - 5.f});
            messageBtn->addChild(msgBadge);
            auto msgCountLabel = geode::Label::create(numToString(m_score->m_newMsgCount), "bigFont.fnt");
            msgCountLabel->setLimitLabelWidth(msgBadge->getContentWidth(), .6f, 0.1f);
            msgCountLabel->setPosition({msgBadge->getContentSize().width / 2.f, msgBadge->getContentSize().height / 2.f});
            msgBadge->addChild(msgCountLabel);
        }
    }

    if (blocked && !m_score->isCurrentUser()) {
        auto unblockBtn = Button::createWithSpriteFrameName("accountBtn_unblock_001.png", [this](geode::Button* sender) {
            if (!m_score || !hasAccount("block <cg>users</c>")) return;
            createQuickPopup(
                "Unblock user",
                fmt::format("Are you sure you want to <cl>unblock</c> <cy>{}</c>?", m_score->m_userName),
                "Back",
                "Unblock",
                [this](auto layer, auto unblock) {
                    if (!unblock) return;
                    prepareUploadAction();
                    if (GameLevelManager::get()->unblockUser(m_score->m_accountID)) showUploadPopup("Unblocking user...");
                });
        });
        m_userOptionsMenu->addChild(unblockBtn);
    }

    // moderators can't be blocked
    if (!blocked && !m_score->isCurrentUser() && m_score->m_modBadge != 1 && m_score->m_modBadge != 2) {
        auto blockBtn = Button::createWithSpriteFrameName("accountBtn_blocked_001.png", [this](geode::Button* sender) {
            if (!m_score || !hasAccount("block <cg>users</c>")) return;
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
                    if (!block) return;
                    prepareUploadAction();
                    if (GameLevelManager::get()->blockUser(m_score->m_accountID)) showUploadPopup("Blocking user...");
                });
        });
        m_userOptionsMenu->addChild(blockBtn);
    }

    const bool showFriendButtons = !noMessaging && !blocked && !m_score->isCurrentUser();

    if (showFriendButtons && m_score->m_friendReqStatus == 0 && m_score->m_friendStatus != 1) {
        auto addFriendBtn = Button::createWithSpriteFrameName("accountBtn_requests_001.png", [this](geode::Button* sender) {
            if (!m_score || !hasAccount("add <cg>friends</c>")) return;
            if (auto layer = ShareCommentLayer::create("Friend Request", 140, CommentType::FriendRequest, m_score->m_accountID, "")) {
                layer->show();
            }
        });
        m_userOptionsMenu->addChild(addFriendBtn);
    }

    if (showFriendButtons && m_score->m_friendReqStatus == 1) {
        auto friendBtn = Button::createWithSpriteFrameName("accountBtn_removeFriend_001.png", [this](geode::Button* sender) {
            if (!m_score || !hasAccount("add <cg>friends</c>")) return;
            createQuickPopup(
                "Unfriend",
                fmt::format("Are you sure you want to unfriend <cg>{}</c>?", m_score->m_userName),
                "Back",
                "Unfriend",
                [this](auto layer, auto unfriend) {
                    if (!unfriend) return;
                    prepareUploadAction();
                    if (GameLevelManager::get()->removeFriend(m_score->m_accountID)) showUploadPopup("Removing friend...");
                });
        });
        m_userOptionsMenu->addChild(friendBtn);
    }

    // they sent you a friend request
    if (showFriendButtons && m_score->m_friendReqStatus == 3) {
        auto answerRequestBtn = Button::createWithSpriteFrameName("accountBtn_pendingRequest_001.png", [this](geode::Button* sender) {
            if (!m_score || !hasAccount("add <cg>friends</c>")) return;
            GJFriendRequest* friendObj = GameLevelManager::get()->friendRequestFromAccountID(m_score->m_accountID);
            m_score->m_newFriendRequest = false;
            if (friendObj) FriendRequestPopup::create(friendObj, false)->show();
        });
        m_userOptionsMenu->addChild(answerRequestBtn);
    }

    // you sent them a friend request
    if (showFriendButtons && m_score->m_friendReqStatus == 4) {
        auto sentRequestBtn = Button::createWithSpriteFrameName("accountBtn_pending_001.png", [this](geode::Button* sender) {
            if (!m_score || !hasAccount("add <cg>friends</c>")) return;
            if (auto friendObj = GameLevelManager::get()->friendRequestFromAccountID(m_score->m_accountID)) {
                FriendRequestPopup::create(friendObj, true)->show();
                return;
            }
            createQuickPopup(
                "Cancel friend request",
                "Are you sure you want to remove this friend request?",
                "Back",
                "Remove",
                [this](auto layer, auto remove) {
                    if (!remove) return;
                    prepareUploadAction();
                    if (GameLevelManager::get()->deleteFriendRequests(m_score->m_accountID, nullptr, false)) showUploadPopup("Removing friend request...");
                });
        });
        m_userOptionsMenu->addChild(sentRequestBtn);
    }

    if (m_score->isCurrentUser() && m_ownProfile) {
        // @geode-ignore(unknown-resource)
        auto shareCommentBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("geode.loader/message.png"), [this](geode::Button* sender) {
            if (m_score) {
                if (auto layer = ShareCommentLayer::create("Post Account Update", 140, CommentType::Account, m_score->m_accountID, "")) {
                    GameLevelManager::get()->m_commentUploadDelegate = this;
                    layer->show();
                }
            }
        });
        m_userOptionsMenu->addChild(shareCommentBtn);
    }

    m_userOptionsMenu->updateLayout();

    // bottom left menu
    if ((m_score->m_commentHistoryStatus == 0) || (m_score->m_commentHistoryStatus == 1 && m_score->m_friendReqStatus == 1) || (m_score->isCurrentUser())) {
        auto commentHistoryBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("particle_206_001.png"), [this](geode::Button* sender) {
            if (m_score) {
                InfoLayer::create(nullptr, m_score, nullptr, nullptr)->show();
            }
        });
        m_otherOptionsMenu->addChild(commentHistoryBtn);
        m_otherOptionsMenu->updateLayout();
    }

    if (!m_score->isCurrentUser() && m_score->m_friendReqStatus != 2) {
        auto glm = GameLevelManager::get();
        bool isFollowing = glm ? glm->isFollowingUser(m_score->m_accountID) : false;
        auto followSprite = isFollowing ? AccountButtonSprite::createWithSpriteFrameName("gj_heartOn_001.png", 1.f, AccountBaseColor::Blue) : AccountButtonSprite::createWithSpriteFrameName("gj_heartOff_001.png", 1.f, AccountBaseColor::Gray);
        auto followUserBtn = Button::createWithNode(followSprite, [this](geode::Button* sender) {
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

    if (Mod::get()->getSettingValue<bool>("showModSettingsButton")) {
        // @geode-ignore(unknown-resource)
        auto modSettingsBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("geode.loader/settings.png", 1.2f, AccountBaseColor::Purple), [this](geode::Button* sender) {
            openSettingsPopup(getMod());
        });
        modSettingsBtn->setScale(0.6f);
        m_buttonMenu->addChildAtPosition(modSettingsBtn, Anchor::BottomRight, {0.f, 0.f});
    }

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
        if (!m_score) return;
        GJSearchObject* searchObj = GJSearchObject::create(SearchType::UsersLevels, numToString(m_score->m_accountID));
        searchObj->m_searchMode = 1;  // 1 = lists, 0 = levels
        auto scene = CCScene::create();
        auto levelLayer = LevelBrowserLayer::create(searchObj);
        scene->addChild(levelLayer);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
    });
    m_onlineMenu->addChild(listAccountBtn);

    auto myLevelBtn = Button::createWithSpriteFrameName("accountBtn_myLevels_001.png", [this](geode::Button* sender) {
        if (!m_score) return;
        GJSearchObject* searchObj = GJSearchObject::create(SearchType::UsersLevels, numToString(m_score->m_userID));
        auto scene = CCScene::create();
        auto levelLayer = LevelBrowserLayer::create(searchObj);
        scene->addChild(levelLayer);
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
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

        if (m_moonsRankLoaded && m_moonsRank > 0) {
            auto moonsLeaderboardBtn = Button::createWithLabel(fmt::format("Moons Rank:\n#{}", m_moonsRankLoaded ? m_moonsRank : 0), "chatFont.fnt", [this](geode::Button* sender) {
                if (!m_score) return;
                m_score->m_leaderboardStat = LeaderboardStat::Moons;
                auto scene = CCScene::create();
                auto layer = LeaderboardsLayer::create(LeaderboardType::Global, m_score->m_leaderboardStat);
                scene->addChild(layer);
                CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
            });

            moonsLeaderboardBtn->setColor({109, 215, 249});
            auto moonsRankSprite = CCSprite::createWithSpriteFrameName(getRankIconForGlobalRank(m_moonsRank).c_str());
            moonsRankSprite->setPosition({-5, moonsLeaderboardBtn->getContentSize().height / 2.f});
            moonsRankSprite->setAnchorPoint({1.f, 0.5f});
            moonsLeaderboardBtn->addChild(moonsRankSprite);

            m_leaderboardMenu->addChild(moonsLeaderboardBtn);
        }
    }
    m_leaderboardMenu->updateLayout();

    // if (Mod::get()->getSettingValue<bool>("showGradientBackground")) showGradientBackground(GameManager::get()->colorForIdx(m_score->m_color1), GameManager::get()->colorForIdx(m_score->m_color2));

    refreshRatedLevelCell();
}

void ProfilePopup::refreshRatedLevelCell() {
    if (!m_levelCellBorder) {
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
    if (m_levelContainer) {
        m_levelContainer->removeAllChildrenWithCleanup(true);
    }

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
            m_spinner->setPosition({m_levelCellBorder->getContentSize().width, m_levelCellBorder->getContentSize().height});
            if (m_levelContainer) m_levelContainer->addChild(m_spinner);
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

    auto cellSize = m_levelCellBorder->getContentSize();
    auto cellFactory = CustomListView::create(CCArray::create(), BoomListType::Level, cellSize.height, cellSize.width);
    m_levelCell = createListCell<LevelCell>(cellFactory, " ", cellSize.height);
    if (!m_levelCell) {
        return;
    }

    m_levelCell->setUserFlag("profile-overhaul-level-cell");
    m_levelCell->setVisible(true);

    m_levelCell->setContentSize(m_levelCellBorder->getContentSize());
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
    if (m_levelCell) {
        auto levelCellClippingNode = CCClippingNode::create();
        levelCellClippingNode->setAlphaThreshold(0.1f);
        if (!levelCellClippingNode) {
            return;
        }

        auto levelCellStencil = CCSprite::createWithSpriteFrameName("square02b_001.png");
        if (!levelCellStencil) {
            return;
        }

        levelCellStencil->setAnchorPoint({0.5f, 0.5f});
        levelCellStencil->setPosition({m_levelContainer->getContentSize().width, m_levelContainer->getContentSize().height});
        levelCellStencil->setScaleX(m_levelContainer->getContentSize().width / levelCellStencil->getContentSize().width);
        levelCellStencil->setScaleY(m_levelContainer->getContentSize().height / levelCellStencil->getContentSize().height);

        levelCellClippingNode->setStencil(levelCellStencil);
        levelCellClippingNode->setContentSize(m_levelContainer->getContentSize());
        levelCellClippingNode->setAnchorPoint({0.5f, 0.5f});
        levelCellClippingNode->setPosition({m_levelContainer->getContentSize().width / 2.f, m_levelContainer->getContentSize().height / 2.f});
        m_levelCell->setPosition({levelCellClippingNode->getContentSize().width / 2.f, levelCellClippingNode->getContentSize().height / 2.f});
        levelCellClippingNode->addChild(m_levelCell);

        if (m_levelContainer) m_levelContainer->addChild(levelCellClippingNode);
    }
    m_levelCellBorder->updateLayout();
}

void ProfilePopup::showGradientBackground(ccColor3B startColor, ccColor3B endColor) {
    if (!m_score || !m_mainLayer) {
        return;
    }

    auto gameManager = GameManager::get();
    if (!gameManager) {
        return;
    }

    if (m_gradient) {
        m_gradient->removeFromParent();
        m_gradient = nullptr;
    }

    if (m_mainLayer) {
        auto gradientLayer = CCLayer::create();
        gradientLayer->setContentSize(m_mainLayer->getContentSize());
        m_mainLayer->addChild(gradientLayer, 2);

        m_gradient = CCLayerGradient::create();
        m_gradient->setID("profile-gradient");

        m_gradient->setStartOpacity(255);
        m_gradient->setEndOpacity(255);
        m_gradient->setStartColor(startColor);
        m_gradient->setEndColor(endColor);
        m_gradient->setContentSize(gradientLayer->getContentSize());

        gradientLayer->addChild(m_gradient);
    }
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

    m_noneLabel = geode::Label::create(showLatestLevel ? "No level found" : "No rated level", "goldFont.fnt");
    m_noneLabel->setScale(0.5f);
    m_noneLabel->setAnchorPoint({0.5f, 0.5f});
    m_noneLabel->setPosition({m_levelCellBorder->getContentSize().width, m_levelCellBorder->getContentSize().height});
    if (m_levelContainer)
        m_levelContainer->addChild(m_noneLabel);
    else
        m_levelCellBorder->addChild(m_noneLabel);
    m_levelCellBorder->updateLayout();
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

// the delegates may have been taken by another page since the popup opened
void ProfilePopup::prepareUploadAction() {
    auto glm = GameLevelManager::get();
    glm->m_userInfoDelegate = this;
    glm->m_uploadActionDelegate = this;
}

void ProfilePopup::showUploadPopup(char const* text) {
    if (m_uploadPopup) m_uploadPopup->m_delegate = nullptr;
    m_uploadPopup = UploadActionPopup::create(this, text);
    m_uploadPopup->show();
}

void ProfilePopup::uploadActionFinished(int id, int response) {
    if (id != m_accountId) return;

    if (m_uploadPopup) {
        char const* text = "Done";
        switch (response) {
            case 36: text = "Request removed"; break;
            case 39: text = "Friend removed"; break;
            case 40: text = "User blocked"; break;
            case 41: text = "User unblocked"; break;
            default: break;
        }
        m_uploadPopup->showSuccessMessage(text);
    }

    // an unblocked profile has its buttons and comments back, so reload it
    if (response == 41) {
        if (m_score) m_score->m_friendReqStatus = 0;
        refreshUserInfoUI();
        auto glm = GameLevelManager::get();
        glm->m_userInfoDelegate = this;
        glm->getGJUserInfo(m_accountId);
    }
}

void ProfilePopup::uploadActionFailed(int id, int response) {
    if (id != m_accountId || !m_uploadPopup) return;
    m_uploadPopup->showFailMessage("Failed. Please try again later.");
}

void ProfilePopup::onClosePopup(UploadActionPopup* popup) {
    popup->setKeypadEnabled(false);
    popup->removeFromParentAndCleanup(true);
    if (popup == m_uploadPopup) m_uploadPopup = nullptr;
}

// gjuserinfo delegate
void ProfilePopup::getUserInfoFailed(int id) {
    log::error("user info request failed for account id {}", id);
}

void ProfilePopup::clearCommentsStatus() {
    if (m_commentsSpinner) {
        m_commentsSpinner->removeFromParent();
        m_commentsSpinner = nullptr;
    }
    if (m_statusLabel) m_statusLabel->setVisible(false);
}

void ProfilePopup::showCommentsSpinner() {
    clearCommentsStatus();
    m_commentsSpinner = LoadingSpinner::create(35.f);
    if (!m_commentsSpinner) return;
    m_commentsSpinner->setAnchorPoint({0.5f, 0.5f});
    m_commentsSpinner->setPosition(m_commentsList->getContentSize() / 2.f);
    m_commentsList->addChild(m_commentsSpinner);
}

void ProfilePopup::showCommentsStatus() {
    clearCommentsStatus();
    if (!m_statusLabel) return;
    const bool blocked = m_score && m_score->m_friendReqStatus == 2;
    m_statusLabel->setString(blocked ? "You blocked this user" : "No Account Posts");
    m_statusLabel->setVisible(true);
}

// same flow as ProfilePage::loadPage: a stored page is reused, otherwise it gets requested
void ProfilePopup::loadCommentPage(int page) {
    auto glm = GameLevelManager::get();
    if (!glm || !m_commentsList) return;

    m_commentPage = page;
    m_commentKey = fmt::format("a_{}_{}", m_accountId, page);
    glm->m_levelCommentDelegate = this;

    if (auto stored = glm->getStoredOnlineLevels(m_commentKey.c_str())) {
        auto info = glm->getPageInfo(m_commentKey.c_str());
        // copied because the callbacks can move on to the next page and change m_commentKey
        auto key = m_commentKey;
        setupPageInfo(info ? info : "", key.c_str());
        loadCommentsFinished(stored, key.c_str());
        return;
    }

    if (page == 0) showCommentsSpinner();
    log::debug("requesting account comments page {} for account {}", page, m_accountId);
    glm->getAccountComments(m_accountId, page, m_commentItemCount);
}

// "count:start:end"
void ProfilePopup::setupPageInfo(std::string info, char const* key) {
    if (!key || m_commentKey != key) return;

    auto parts = geode::utils::string::splitView(info, ":");
    if (parts.size() < 3) return;

    auto count = geode::utils::numFromString<int>(parts[0]);
    auto start = geode::utils::numFromString<int>(parts[1]);
    auto end = geode::utils::numFromString<int>(parts[2]);
    if (!count || !start || !end) return;

    m_commentItemCount = *count;
    m_commentPageStart = *start;
    m_commentPageEnd = *end;
}

void ProfilePopup::loadCommentsFinished(cocos2d::CCArray* comments, char const* key) {
    // the delegate is shared, so answers for an older request or another page are ignored
    if (!comments || !m_commentsList || !key || m_commentKey != key) {
        return;
    }

    if (m_commentPage == 0) {
        m_commentsList->clear();
    }
    clearCommentsStatus();

    if (m_score && m_score->m_friendReqStatus == 2) {
        showCommentsStatus();
        return;
    }

    log::debug("account comments page {} returned {} items", m_commentPage, comments->count());

    if (comments->count() == 0) {
        if (m_commentPage == 0) showCommentsStatus();
        m_commentsList->updateLayout();
        return;
    }

    const auto width = m_commentsList->getListSize().width;
    auto cellFactory = CustomListView::create(CCArray::create(), BoomListType::Comment4, 85.f, width);

    for (auto i = 0u; i < comments->count(); ++i) {
        auto comment = typeinfo_cast<GJComment*>(comments->objectAtIndex(i));
        if (!comment) {
            continue;
        }

        auto cell = createListCell<CommentCell>(cellFactory, "commentCell", 85.f);
        if (!cell) {
            continue;
        }

        cell->setUserFlag("profile-overhaul-comment-cell");
        cell->setVisible(true);
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

    m_commentsList->updateLayout();

    // the list shows every post, so keep going until the server's total is reached
    if (m_commentPageStart + m_commentPageEnd < m_commentItemCount) {
        loadCommentPage(m_commentPage + 1);
    } else {
        log::debug("account comments finished after page {}", m_commentPage);
    }
}

void ProfilePopup::loadCommentsFailed(char const* key) {
    if (!key || m_commentKey != key) {
        return;
    }
    log::error("account comments load failed for key {}", key);

    if (!m_commentsList) {
        return;
    }

    clearCommentsStatus();
    if (m_commentPage == 0) {
        m_commentsList->clear();
        showCommentsStatus();
        m_commentsList->updateLayout();
    }
}

// same as ProfilePage::onUpdate on your own profile: stored pages go first, otherwise
// loadCommentPage would just get the old ones back
void ProfilePopup::refreshComments() {
    if (!m_commentsList) {
        return;
    }

    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    for (int page = 0; page <= 100; ++page) {
        auto key = fmt::format("a_{}_{}", m_accountId, page);
        if (!glm->getStoredOnlineLevels(key.c_str())) break;
        glm->m_storedLevels->removeObjectForKey(key);
    }

    m_commentItemCount = 0;
    m_commentPageStart = 0;
    m_commentPageEnd = 0;
    m_commentsList->clear();
    clearCommentsStatus();
    m_commentsList->updateLayout();

    if (m_score && m_score->m_friendReqStatus == 2) {
        showCommentsStatus();
        return;
    }

    loadCommentPage(0);
}

void ProfilePopup::commentUploadFinished(int parentID) {
    if (parentID != GJAccountManager::get()->m_accountID) return;
    refreshComments();
}

void ProfilePopup::commentUploadFailed(int parentID, CommentError errorType) {
    // only the server's plain "-1" shows the alert
    if (parentID != GJAccountManager::get()->m_accountID || static_cast<int>(errorType) != -1) return;
    FLAlertLayer::create("Failed", "Comment upload failed. Please try again later.", "OK")->show();
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