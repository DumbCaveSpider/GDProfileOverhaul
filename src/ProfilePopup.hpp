#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/ShareCommentDelegate.hpp>
#include <Geode/binding/ShareCommentLayer.hpp>
#include <cue/ListBorder.hpp>
#include <cue/ListNode.hpp>
#include "Geode/cocos/cocoa/CCObject.h"

using namespace geode::prelude;

class ProfilePopup : public geode::Popup, public UserInfoDelegate, public LevelCommentDelegate, public CommentUploadDelegate, public UploadActionDelegate, public UploadPopupDelegate, public ShareCommentDelegate, public LevelManagerDelegate, public LeaderboardManagerDelegate {
    friend ProfilePage;

public:
    static ProfilePopup* create(int accountId, bool ownProfile);
    bool init(int accountId, bool ownProfile);
    ~ProfilePopup() override;

    void getUserInfoFinished(GJUserScore* score) override;
    void getUserInfoFailed(int id) override;
    void userInfoChanged(GJUserScore* score) override;
    void refreshUserInfoUI();
    void loadCommentsFinished(cocos2d::CCArray* comments, char const* key) override;
    void loadCommentsFailed(char const* key) override;
    void loadLeaderboardFinished(cocos2d::CCArray* scores, char const* key) override;
    void loadLeaderboardFailed(char const* key) override;
    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key) override;
    void loadLevelsFailed(char const* key) override;
    void shareCommentClosed(gd::string text, ShareCommentLayer* layer) override;

    bool m_hasSwitched = false;

private:
    GJUserScore* m_score;
    ProfilePopup* m_profilePopup = nullptr;

    // left side panel
    CCMenu* m_closeMenu;
    CCMenu* m_userOptionsMenu;
    CCMenu* m_otherOptionsMenu;

    // center panel
    CCMenu* m_usernameMenu;
    CCMenu* m_statsMenu;
    CCMenu* m_iconsMenu;
    cue::ListNode* m_commentsList;
    cue::ListBorder* m_levelCellBorder;
    cocos2d::CCNode* m_levelContainer = nullptr;

    // comments UI
    LoadingSpinner* m_commentsSpinner = nullptr;
    CCLabelBMFont* m_noCommentsLabel = nullptr;

    // leaderboard menu
    CCMenu* m_leaderboardMenu;

    // right side panel
    CCMenu* m_swapMenu;
    CCMenu* m_socialsMenu;
    CCMenu* m_onlineMenu;

    // level cell
    LoadingSpinner* m_spinner = nullptr;
    LevelCell* m_levelCell = nullptr;
    CCLabelBMFont* m_noneLabel = nullptr;
    int m_ratedLevelPage = 0;
    bool m_ratedLevelFetchCompleted = false;
    bool m_ratedLevelFetchFailed = false;
    gd::string m_ratedLevelSearchKey;

    // leaderboard state
    int m_moonsRank = 0;
    bool m_moonsRankLoaded = false;

    // members
    int m_accountId;
    bool m_ownProfile;
    int m_commentPage = 0;
    int m_commentPageSize = 10;
    bool m_refreshScheduled = false;

    void requestAccountCommentsPage(int page);
    void refreshRatedLevelCell();
    void showNoRatedLevelLabel(bool showLatestLevel);
    void refreshComments();
    void requestMoonLeaderboardRank();

    void onList(CCObject* sender);
    void onLevel(CCObject* sender);

    void onCommentHistory(CCObject* sender);
    void onFollowUser(CCObject* sender);

protected:
    void onInfo(CCObject* sender);
};