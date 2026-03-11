#ifndef MENU_SCENE_H
#define MENU_SCENE_H

#include <cardgfx.h>

using namespace CardGFX;

// =====================================================================
// Main Menu Scene
// =====================================================================

class MenuScene : public Scene {
public:
    StatusBar topBar;
    List menuList;

    MenuScene() : Scene("menu") {}

    void setup() {
        topBar.setBounds({0, 0, SCREEN_W, 11});
        topBar.setCenter("CARDPUTER RNG");
        topBar.setDrawSeparator(true);
        addWidget(&topBar);

        menuList.setBounds({0, 12, SCREEN_W, SCREEN_H - 12});
        menuList.addItem("Generate Key");
        menuList.addItem("Flip a Coin");
        menuList.setOnSelect([](uint8_t index, const char* text) {
            switch (index) {
                case 0:
                    CardGFX::scenes().pushByName("rng", Transition::SlideLeft);
                    break;
                case 1:
                    CardGFX::scenes().pushByName("coin", Transition::SlideLeft);
                    break;
            }
        });
        addWidget(&menuList, true);  // focusable
    }

    void onEnter() override {
        topBar.markDirty();
        menuList.markDirty();
    }
};

#endif // MENU_SCENE_H
