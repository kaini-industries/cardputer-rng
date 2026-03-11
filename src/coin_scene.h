#ifndef COIN_SCENE_H
#define COIN_SCENE_H

#include <cardgfx.h>
#include <RNG.h>

using namespace CardGFX;

// =====================================================================
// Coin Flip Scene
// =====================================================================

class CoinScene : public Scene {
public:
    StatusBar topBar;
    Label hintLabel;

    bool hasResult = false;
    bool isHeads = false;

    CoinScene() : Scene("coin") {}

    void setup() {
        topBar.setBounds({0, 0, SCREEN_W, 11});
        topBar.setCenter("COIN FLIP");
        topBar.setDrawSeparator(true);
        addWidget(&topBar);

        hintLabel.setBounds({0, 124, SCREEN_W, 11});
        hintLabel.setText("Press ENTER to flip");
        hintLabel.setAlign(Label::Align::Center);
        hintLabel.setColor(CardGFX::theme().fgSecondary);
        addWidget(&hintLabel);
    }

    void onEnter() override {
        hasResult = false;
        hintLabel.setText("Press ENTER to flip");
        hintLabel.markDirty();
        topBar.markDirty();
    }

    bool onInput(const InputEvent& event) override {
        if (!event.isDown()) return false;

        if (event.key == Key::ESCAPE) {
            CardGFX::scenes().pop(Transition::SlideRight);
            return true;
        }

        if (event.key == Key::ENTER) {
            byte rnd;
            RNG.rand(&rnd, 1);
            isHeads = (rnd & 1) == 0;
            hasResult = true;
            hintLabel.setText("[ENT] Flip  [G0] Menu");
            hintLabel.markDirty();
            topBar.markDirty();
            return true;
        }

        return false;
    }

    void onDrawOverlay(Canvas& fb, const Theme& theme) override {
        if (!hasResult) return;

        // Clear content area (below top bar, above hint)
        fb.fillRect(0, 12, SCREEN_W, SCREEN_H - 23, theme.bgPrimary);

        // Coin circle
        int16_t cx = SCREEN_W / 2;
        int16_t cy = 55;
        int16_t r = 28;
        uint16_t coinColor = isHeads ? theme.accent : theme.success;

        fb.fillCircle(cx, cy, r, coinColor);
        fb.drawCircle(cx, cy, r, theme.fgPrimary);

        // Letter inside coin (scale 3 = 15x21)
        const char* letter = isHeads ? "H" : "T";
        int16_t lx = cx - 7;  // ~half of 15px wide
        int16_t ly = cy - 10; // ~half of 21px tall
        fb.drawText(lx, ly, letter, theme.bgPrimary, 3);

        // Result word below coin (scale 2 = 10x14)
        const char* word = isHeads ? "HEADS" : "TAILS";
        int16_t tw = fb.textWidth(word, 2);
        int16_t tx = (SCREEN_W - tw) / 2;
        fb.drawText(tx, cy + r + 6, word, coinColor, 2);
    }
};

#endif // COIN_SCENE_H
