#include <Geode/Bindings.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "ProfilePopup.hpp"

using namespace geode::prelude;

class $modify(MenuLayer) {
    void onMyProfile(CCObject*) {
        ProfilePopup::create(7689052, true)->show();
    }
};