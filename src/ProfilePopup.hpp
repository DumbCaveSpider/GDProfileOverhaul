#include <Geode/Geode.hpp>
#include <cue/ListBorder.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class ProfilePopup : public geode::Popup, public UserInfoDelegate, public LevelCommentDelegate, public CommentUploadDelegate, public UploadActionDelegate, public UploadPopupDelegate, public LeaderboardManagerDelegate {
    friend ProfilePage;

public:
    static ProfilePopup* create(int accountId, bool ownProfile);
    bool init(int accountId, bool ownProfile);
    ~ProfilePopup() override;

    void getUserInfoFinished(GJUserScore* score) override;
    void getUserInfoFailed(int id) override;
    void userInfoChanged(GJUserScore* score) override;
    void loadCommentsFinished(cocos2d::CCArray* comments, char const* key) override;
    void loadCommentsFailed(char const* key) override;

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
    cue::ListBorder* m_ratedLevelCell;

    // right side panel
    CCMenu* m_refrshMenu;
    CCMenu* m_socialsMenu;
    CCMenu* m_onlineMenu;

    // members
    int m_accountId;
    bool m_ownProfile;

protected:
    void onInfo(CCObject* sender);
};