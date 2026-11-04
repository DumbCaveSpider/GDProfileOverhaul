#include <Geode/binding/CommentCell.hpp>
#include <Geode/binding/FriendsProfilePage.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJLevelScoreCell.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/MessageThreadsPage.hpp>
#include <Geode/modify/FRequestProfilePage.hpp>
#include <Geode/modify/FriendsProfilePage.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/GJRequestCell.hpp>
#include <Geode/modify/GJScoreCell.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelListLayer.hpp>
#include <Geode/modify/LevelListCell.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/modify/GJMessageCell.hpp>
#include <Geode/modify/GJLevelScoreCell.hpp>
#include <Geode/modify/GJCompactUserCell.hpp>
#include <Geode/modify/InfoLayer.hpp>
#include <Geode/Geode.hpp>
#include "../include/ProfileOverhaulConstant.hpp"
#include "../ProfilePopup.hpp"
#include "Geode/ui/Button.hpp"

using namespace geode::prelude;


#if !defined(GEODE_IS_IOS)
class $modify(MenuLayer) {
    void onMyProfile(CCObject*) {
        ProfilePopup::create(GJAccountManager::get()->m_accountID, true)->show();
    }
};
#endif
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_ANDROID)
class $modify(MessageThreadsPage) {
    void onClose(CCObject* sender) {
        profile::suppressProfilePageReopen = !profile::onVanillaProfilePage;
        MessageThreadsPage::onClose(sender);
        profile::suppressProfilePageReopen = false;
    }
};
#endif

// temp no address outside Android in the 2.2091 bindings
#if defined(GEODE_IS_ANDROID)
class $modify(FRequestProfilePage) {
    void onClose(CCObject* sender) {
        profile::suppressProfilePageReopen = !profile::onVanillaProfilePage && m_sent == false;
        FRequestProfilePage::onClose(sender);
        profile::suppressProfilePageReopen = false;
    }
};
#endif

class $modify(FriendsProfilePage) {
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_ANDROID)
    void onClose(CCObject* sender) {
        profile::suppressProfilePageReopen = !profile::onVanillaProfilePage && m_type == UserListType::Friends;
        FriendsProfilePage::onClose(sender);
        profile::suppressProfilePageReopen = false;
    }
#endif

#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_ANDROID)
    // friend list rows are virtual in 2.2091, the row's button holds the score as its user object
    void onViewProfile(CCObject* sender) {
        auto node = typeinfo_cast<CCNode*>(sender);
        auto score = node ? typeinfo_cast<GJUserScore*>(node->getUserObject()) : nullptr;
        if (!score || score->m_accountID <= 0) {
            FriendsProfilePage::onViewProfile(sender);
            return;
        }
        ProfilePopup::create(score->m_accountID, false)->show();
    }
#endif
};


class $modify(ProfilePage) {
    bool init(int accountID, bool ownProfile) {
        if (profile::suppressProfilePageReopen) return FLAlertLayer::init(150);
        if (!ProfilePage::init(accountID, ownProfile)) return false;
        return true;
    }

    void show() {
        if (profile::suppressProfilePageReopen) return;
        ProfilePage::show();
    }

    CCNode* getOrCreateBottomMenu() {
        if (auto menu = m_mainLayer->getChildByIDRecursive("bottom-menu")) return menu;

        auto menu = CCMenu::create();
        auto winSize = CCDirector::get()->getWinSize();
        menu->setID("bottom-menu");
        menu->setContentSize({270.f, 41.5f});
        menu->m_bIgnoreAnchorPointForPosition = false;
        menu->setScale(0.7f);
        menu->setLayout(RowLayout::create()->setAutoScale(false));
        menu->setPosition({winSize.width / 2.f, 37.f});
        m_mainLayer->addChild(menu, 10);
        return menu;
    }

    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);
        auto bottomMenu = getOrCreateBottomMenu();
        if (bottomMenu) {
            // this runs again on every reload, and vanilla only clears its own buttons
            bottomMenu->removeChildByID("profile-overhaul-button");
            if (!score) return;
            auto profileBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("PO-icon-person.png"_spr), [this, score](geode::Button* sender) {
                profile::onVanillaProfilePage = false;
                this->onClose(sender);
                ProfilePopup::create(score->m_accountID, m_ownProfile)->show();
            });
            profileBtn->setID("profile-overhaul-button");
            bottomMenu->addChild(profileBtn);
            bottomMenu->updateLayout();
        }
    }
};

#if !defined(GEODE_IS_IOS)
class $modify(LevelCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(this->m_level->m_accountID, false)->show();
    }
};
#endif

// on Windows this should be the same function as GJLevelScoreCell::onViewProfile, which is hooked below
#if defined(GEODE_IS_INTEL_MAC) || defined(GEODE_IS_ANDROID)
class $modify(GJRequestCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_score->m_accountID, false)->show();
    }
};
#endif

#if !defined(GEODE_IS_IOS)
class $modify(GJScoreCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_score->m_accountID, false)->show();
    }
};
#endif

#if !defined(GEODE_IS_IOS)
class $modify(LevelInfoLayer) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_level->m_accountID, false)->show();
    }
};
#endif

#if !defined(GEODE_IS_IOS)
class $modify(LevelListLayer) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_levelList->m_accountID, false)->show();
    }
};
#endif

#if !defined(GEODE_IS_IOS)
class $modify(LevelListCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_levelList->m_accountID, false)->show();
    }
};
#endif

#if !defined(GEODE_IS_IOS)
class $modify(CommentCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_comment->m_accountID, false)->show();
    }
};
#endif

#if !defined(GEODE_IS_IOS)
class $modify(GJMessageCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_message->m_accountID, false)->show();
    }
};
#endif

#if !defined(GEODE_IS_IOS)
class $modify(GJLevelScoreCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_userScore->m_accountID, false)->show();
    }
};
#endif

// on Windows this is the same function as GJLevelScoreCell::onViewProfile, which is hooked below
#if defined(GEODE_IS_ANDROID)
class $modify(GJCompactUserCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_userScore->m_accountID, false)->show();
    }
};
#endif

#if !defined(GEODE_IS_IOS)
class $modify(InfoLayer) {
    void onMore(CCObject* sender) {
        if (m_level) ProfilePopup::create(m_level->m_accountID, false)->show();
        if (m_levelList) ProfilePopup::create(m_levelList->m_accountID, false)->show();
        if (m_score) ProfilePopup::create(m_score->m_accountID, false)->show();
    }
};
#endif