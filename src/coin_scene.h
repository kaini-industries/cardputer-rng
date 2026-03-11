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
    uint8_t animFrames = 0;

    static constexpr uint8_t ANIM_TOTAL = 12; // ~400ms at 30 FPS

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

    void onTick(uint32_t dt_ms) override {
        if (animFrames > 0) {
            animFrames--;
            topBar.markDirty(); // force redraw every frame during animation
        }
    }

    bool onInput(const InputEvent& event) override {
        if (!event.isDown()) return false;

        // Block input during flip animation
        if (animFrames > 0) return true;

        if (event.key == Key::ESCAPE) {
            CardGFX::scenes().pop(Transition::SlideRight);
            return true;
        }

        if (event.key == Key::ENTER) {
            byte rnd;
            RNG.rand(&rnd, 1);
            isHeads = (rnd & 1) == 0;
            hasResult = true;
            animFrames = ANIM_TOTAL;
            hintLabel.setText("[ENT] Flip  [G0] Menu");
            hintLabel.markDirty();
            topBar.markDirty();
            return true;
        }

        return false;
    }

    void onDrawOverlay(Canvas& fb, const Theme& theme) override {
        if (!hasResult) return;

        bool animating = (animFrames > 0);

        fb.fillRect(0, 12, SCREEN_W, SCREEN_H - 23, theme.bgPrimary);

        int16_t cx = SCREEN_W / 2;
        int16_t cy = 55;
        int16_t r = 28;

        // During animation: alternate H/T each frame, squish coin horizontally
        bool showHeads;
        if (animating) {
            showHeads = (animFrames % 2 == 0);
            // Squish: width goes full → thin → full (triangle wave)
            // animFrames counts down from 12 to 1
            // halfPoint = 6, distance from center gives squish amount
            int16_t halfPoint = ANIM_TOTAL / 2;
            int16_t dist = (animFrames > halfPoint)
                           ? (ANIM_TOTAL - animFrames) : animFrames;
            // dist ranges 0..6, map to half-width: 3..28
            int16_t halfW = 3 + (dist * 25) / halfPoint;
            // Draw squished coin as a filled rounded rect
            fb.fillRoundRect(cx - halfW, cy - r, halfW * 2, r * 2, halfW < r ? halfW : r,
                             showHeads ? theme.accent : theme.success);
            // Only draw letter when wide enough to read
            if (halfW > 12) {
                const char* letter = showHeads ? "H" : "T";
                fb.drawText(cx - 7, cy - 10, letter, theme.bgPrimary, 3);
            }
        } else {
            showHeads = isHeads;
            uint16_t coinColor = isHeads ? theme.accent : theme.success;
            fb.fillCircle(cx, cy, r, coinColor);
            fb.drawCircle(cx, cy, r, theme.fgPrimary);

            const char* letter = isHeads ? "H" : "T";
            fb.drawText(cx - 7, cy - 10, letter, theme.bgPrimary, 3);

            // Result word below coin
            const char* word = isHeads ? "HEADS" : "TAILS";
            int16_t tw = fb.textWidth(word, 2);
            int16_t tx = (SCREEN_W - tw) / 2;
            fb.drawText(tx, cy + r + 6, word, coinColor, 2);
        }
    }
};

#endif // COIN_SCENE_H
