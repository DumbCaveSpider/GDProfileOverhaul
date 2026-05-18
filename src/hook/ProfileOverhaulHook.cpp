#include <Geode/binding/CommentCell.hpp>
#include <Geode/binding/FriendsProfilePage.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/binding/GJLevelScoreCell.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/MessagesProfilePage.hpp>
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
#include <Geode/modify/GJUserCell.hpp>
#include <Geode/Geode.hpp>
#include "../include/ProfileOverhaulConstant.hpp"
#include "../ProfilePopup.hpp"
#include "Geode/ui/Button.hpp"

using namespace geode::prelude;

class $modify(MenuLayer) {
    void onMyProfile(CCObject*) {
        ProfilePopup::create(GJAccountManager::get()->m_accountID, true)->show();
    }
};
class $modify(MessagesProfilePage) {
    void onClose(cocos2d::CCObject* sender) {
        if (!profile::onVanillaProfilePage && m_sentMessages == false) {
            setKeypadEnabled(false);
            removeFromParentAndCleanup(true);
        } else {
            MessagesProfilePage::onClose(sender);
        }
    }
};

class $modify(FRequestProfilePage) {
    void onClose(CCObject* sender) {
        if (!profile::onVanillaProfilePage && m_sent == false) {
            setKeypadEnabled(false);
            removeFromParentAndCleanup(true);
        } else {
            FRequestProfilePage::onClose(sender);
        }
    }
};

class $modify(FriendsProfilePage) {
    void onClose(CCObject* sender) {
        if (!profile::onVanillaProfilePage && m_type == UserListType::Friends) {
            setKeypadEnabled(false);
            removeFromParentAndCleanup(true);
        } else {
            FriendsProfilePage::onClose(sender);
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

class $modify(LevelCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(this->m_level->m_accountID, false)->show();
    }
};

class $modify(GJRequestCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_score->m_accountID, false)->show();
    }
};

class $modify(GJScoreCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_score->m_accountID, false)->show();
    }
};

class $modify(LevelInfoLayer) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_level->m_accountID, false)->show();
    }
};

class $modify(LevelListLayer) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_levelList->m_accountID, false)->show();
    }
};

class $modify(LevelListCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_levelList->m_accountID, false)->show();
    }
};

class $modify(CommentCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_comment->m_accountID, false)->show();
    }
};

class $modify(GJMessageCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_message->m_accountID, false)->show();
    }
};

class $modify(GJLevelScoreCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_userScore->m_accountID, false)->show();
    }
};

class $modify(GJUserCell) {
    void onViewProfile(CCObject* sender) {
        ProfilePopup::create(m_userScore->m_accountID, false)->show();
    }
};