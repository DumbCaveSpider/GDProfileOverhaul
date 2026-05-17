#include <Geode/modify/MessagesProfilePage.hpp>
#include <Geode/modify/FRequestProfilePage.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/Geode.hpp>
#include "../include/ProfileOverhaulConstant.hpp"
#include "../ProfilePopup.hpp"
#include "Geode/ui/Button.hpp"

using namespace geode::prelude;

class $modify(MessagesProfilePage) {
    void onClose(cocos2d::CCObject* sender) {
        if (!profile::onVanillaProfilePage) {
            setKeypadEnabled(false);
            removeFromParentAndCleanup(true);
        } else {
            MessagesProfilePage::onClose(sender);
        }
    }
};

class $modify(FRequestProfilePage) {
    void onClose(CCObject* sender) {
        if (!profile::onVanillaProfilePage) {
            setKeypadEnabled(false);
            removeFromParentAndCleanup(true);
        } else {
            FRequestProfilePage::onClose(sender);
        }
    }
};

class $modify(ProfilePage) {
    void loadPageFromUserInfo(GJUserScore* score) {
        auto bottomMenu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByIDRecursive("bottom-menu"));
        if (bottomMenu) {
            auto profileBtn = Button::createWithNode(AccountButtonSprite::createWithSpriteFrameName("PO-icon-person.png"_spr), [this, score](geode::Button* sender) {
                profile::onVanillaProfilePage = false;
                ProfilePopup::create(score->m_accountID, score->isCurrentUser())->show();
            });
            bottomMenu->addChild(profileBtn);
            bottomMenu->updateLayout();
        }
        ProfilePage::loadPageFromUserInfo(score);
    }
};