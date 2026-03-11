#ifndef DICE_SCENE_H
#define DICE_SCENE_H

#include <cardgfx.h>
#include <esp_random.h>
#include <cstdio>

using namespace CardGFX;

// =====================================================================
// Dice Roller Scene
// =====================================================================

class DiceScene : public Scene {
public:
    StatusBar topBar;
    Label hintLabel;
    List diceList;

    bool hasResult = false;
    bool selectingDie = true;
    uint8_t selectedDie = 0;
    uint16_t lastResult = 0;

    static constexpr uint16_t DICE_SIDES[] = {4, 6, 8, 10, 12, 20, 100};
    static constexpr uint8_t NUM_DICE = 7;

    DiceScene() : Scene("dice") {}

    void setup() {
        topBar.setBounds({0, 0, SCREEN_W, 11});
        topBar.setCenter("DICE ROLLER");
        topBar.setDrawSeparator(true);
        addWidget(&topBar);

        diceList.setBounds({0, 12, SCREEN_W, SCREEN_H - 23});
        diceList.addItem("d4");
        diceList.addItem("d6");
        diceList.addItem("d8");
        diceList.addItem("d10");
        diceList.addItem("d12");
        diceList.addItem("d20");
        diceList.addItem("d100");
        diceList.setOnSelect([this](uint8_t index, const char* text) {
            selectedDie = index;
            rollCurrentDie();
        });
        addWidget(&diceList, true);

        hintLabel.setBounds({0, 124, SCREEN_W, 11});
        hintLabel.setText("Pick a die  [G0]Menu");
        hintLabel.setAlign(Label::Align::Center);
        hintLabel.setColor(CardGFX::theme().fgSecondary);
        addWidget(&hintLabel);
    }

    void onEnter() override {
        hasResult = false;
        selectingDie = true;
        diceList.setVisible(true);
        hintLabel.setText("Pick a die  [G0]Menu");
        topBar.setRight("");
        topBar.markDirty();
        diceList.markDirty();
        hintLabel.markDirty();
    }

    bool onInput(const InputEvent& event) override {
        if (!event.isDown()) return false;

        if (event.key == Key::ESCAPE) {
            if (!selectingDie) {
                // Go back to die selection
                showDieSelection();
                return true;
            }
            CardGFX::scenes().pop(Transition::SlideRight);
            return true;
        }

        if (!selectingDie && hasResult) {
            if (event.key == Key::ENTER) {
                rollCurrentDie();
                return true;
            }
            if (event.key == Key::SPACE) {
                showDieSelection();
                return true;
            }
        }

        return false;
    }

    void onDrawOverlay(Canvas& fb, const Theme& theme) override {
        if (!hasResult || selectingDie) return;

        // Clear content area (below top bar, above hint)
        fb.fillRect(0, 12, SCREEN_W, SCREEN_H - 23, theme.bgPrimary);

        // Draw result number large and centered
        char resultStr[8];
        snprintf(resultStr, sizeof(resultStr), "%u", lastResult);

        uint8_t scale = (lastResult >= 100) ? 3 : 4;
        int16_t tw = fb.textWidth(resultStr, scale);
        int16_t th = 7 * scale; // FONT_CHAR_H * scale
        int16_t tx = (SCREEN_W - tw) / 2;
        int16_t ty = 40;
        fb.drawText(tx, ty, resultStr, theme.accent, scale);

        // Die type label below the number
        char dieLabel[8];
        snprintf(dieLabel, sizeof(dieLabel), "d%u", DICE_SIDES[selectedDie]);
        int16_t dlw = fb.textWidth(dieLabel, 2);
        int16_t dlx = (SCREEN_W - dlw) / 2;
        fb.drawText(dlx, ty + th + 10, dieLabel, theme.fgSecondary, 2);
    }

private:
    void rollCurrentDie() {
        uint16_t sides = DICE_SIDES[selectedDie];
        lastResult = rollUnbiased(sides);
        hasResult = true;
        selectingDie = false;
        diceList.setVisible(false);

        char dieLabel[8];
        snprintf(dieLabel, sizeof(dieLabel), "d%u", sides);
        topBar.setRight(dieLabel);
        hintLabel.setText("[ENT]Roll [SPC]Change [G0]Menu");
        topBar.markDirty();
        hintLabel.markDirty();
    }

    void showDieSelection() {
        selectingDie = true;
        hasResult = false;
        diceList.setVisible(true);
        topBar.setRight("");
        hintLabel.setText("Pick a die  [G0]Menu");
        topBar.markDirty();
        diceList.markDirty();
        hintLabel.markDirty();
    }

    // Unbiased random number in [1, sides] using rejection sampling
    static uint16_t rollUnbiased(uint16_t sides) {
        uint32_t limit = (UINT32_MAX / sides) * sides;
        uint32_t rnd;
        do {
            rnd = esp_random();
        } while (rnd >= limit);
        return (rnd % sides) + 1;
    }
};

constexpr uint16_t DiceScene::DICE_SIDES[];

#endif // DICE_SCENE_H
