#include <Geode/Geode.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/utils/file.hpp>
#include <matjson.hpp>
#include <asp/fs.hpp>
#include "FriendNotePopup.hpp"
#include "Geode/ui/General.hpp"

using namespace geode::prelude;

static asp::fs::Path getFriendNoteSaveFilePath() {
    return geode::Mod::get()->getSaveDir() / "savedFriendNote.json";
}

std::pair<std::string, std::string> loadFriendNoteFromFile(int accountId) {
    auto path = getFriendNoteSaveFilePath();
    if (auto parsed = geode::utils::file::readJson(path); parsed) {
        auto root = *parsed;
        if (auto accountValueRes = root.get<matjson::Value>(numToString(accountId)); accountValueRes) {
            auto accountValue = *accountValueRes;
            std::string nickname;
            std::string note;
            if (auto nicknameValue = accountValue.get<std::string>("nickname"); nicknameValue) {
                nickname = *nicknameValue;
            }
            if (auto noteValue = accountValue.get<std::string>("note"); noteValue) {
                note = *noteValue;
            }
            return {nickname, note};
        }
    }
    return {std::string(), std::string()};
}

static void saveFriendNoteToFile(int accountId, std::string const& nickname, std::string const& note) {
    auto path = getFriendNoteSaveFilePath();
    matjson::Value root = matjson::Value::object();

    if (auto parsed = geode::utils::file::readJson(path); parsed) {
        root = *parsed;
    }

    root.set(numToString(accountId), matjson::makeObject({
                                         {"nickname", nickname},
                                         {"note", note},
                                     }));

    auto result = geode::utils::file::writeStringSafe(path, root.dump(4));
    if (result.isErr()) {
        log::warn("Failed to write savedFriendNote.json: {}", result.unwrapErr());
    }
}

FriendNotePopup* FriendNotePopup::create(ProfilePopup* profilePopup, GJUserScore* score) {
    auto ret = new FriendNotePopup();
    if (ret && ret->init(profilePopup, score)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool FriendNotePopup::init(ProfilePopup* profilePopup, GJUserScore* score) {
    if (!Popup::init(330.f, 220.f)) {
        return false;
    }

    if (score->m_friendReqStatus != 1) {
        FLAlertLayer::create("Error", "You can only add <co>Friend Notes</c> to users you <cg>friended</c>.", "OK")->show();
        return false;
    }

    m_noElasticity = true;

    m_closeBtn->removeFromParent();
    m_closeBtn = nullptr;

    std::string title = std::string("Friend Note - ") + score->m_userName.c_str();
    setTitle(title);

    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    // nickname input
    m_nickNameInput = TextInput::create(m_mainLayer->getContentSize().width - 40.f, score->m_userName, "bigFont.fnt");
    m_nickNameInput->setMaxCharCount(20);
    m_nickNameInput->setLabel("Nickname");
    m_nickNameInput->setCommonFilter(CommonFilter::Name);
    m_mainLayer->addChildAtPosition(m_nickNameInput, Anchor::Center, {0.f, 25.f}, {0.5f, 0.5f}, false);

    // friend note input
    m_friendNoteInput = TextInput::create(m_mainLayer->getContentSize().width - 40.f, "Friend Note", "bigFont.fnt");
    m_friendNoteInput->setMaxCharCount(100);
    m_friendNoteInput->setLabel("Friend Note");
    m_friendNoteInput->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChildAtPosition(m_friendNoteInput, Anchor::Center, {0.f, -25.f}, {0.5f, 0.5f}, false);

    // populate saved note/nickname if available
    if (auto [savedNickname, savedNote] = loadFriendNoteFromFile(score->m_accountID); !savedNickname.empty() || !savedNote.empty()) {
        if (!savedNickname.empty()) {
            m_nickNameInput->setString(savedNickname.c_str());
        }
        if (!savedNote.empty()) {
            m_friendNoteInput->setString(savedNote.c_str());
        }
    }

    // apply
    auto applyBtn = Button::createWithNode(ButtonSprite::create("Apply", "goldFont.fnt", "GJ_button_01.png"), [this, profilePopup, score](geode::Button* sender) {
        auto note = m_friendNoteInput->getString();
        auto nickname = m_nickNameInput->getString();
        auto upopup = UploadActionPopup::create(nullptr, "Saving friend note...");
        upopup->show();
        if (nickname.size() > 20) {
            upopup->showFailMessage("Nickname cannot be longer than 20 characters.");
            return;
        }
        saveFriendNoteToFile(score->m_accountID, nickname, note);
        upopup->showSuccessMessage("Friend note saved!");
        profilePopup->refreshUserInfoUI();
        this->removeFromParentAndCleanup(true);
    });
    m_mainLayer->addChildAtPosition(applyBtn, Anchor::Bottom, {-60.f, 25.f}, {0.5f, 0.5f}, false);

    auto cancelBtn = Button::createWithNode(ButtonSprite::create("Cancel", "goldFont.fnt", "GJ_button_06.png"), [this, profilePopup](geode::Button* sender) {
        this->onClose(sender);
    });
    m_mainLayer->addChildAtPosition(cancelBtn, Anchor::Bottom, {60.f, 25.f}, {0.5f, 0.5f}, false);

    return true;
}