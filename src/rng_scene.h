#ifndef RNG_SCENE_H
#define RNG_SCENE_H

#include <cardgfx.h>
#include <Crypto.h>
#include <SHA256.h>
#include <RNG.h>
#include <cstdlib>
#include <cstring>

using namespace CardGFX;

// --- Shared RNG state (defined in main.cpp) ---
extern SHA256 hashMachine;
extern const int KEY_SIZE;
extern bool keyReady;
extern bool keyRngHashReady;
extern bool resetRng;
extern byte rngKey[];
extern bool serialOutput;
extern std::string rngStr;
extern std::string otpStr;
extern std::string bigBinStr;
extern std::string keyHexStr;

// --- Display state ---
enum class DisplayMode { OTP, DIGITS, HEX_VIEW, BINARY };

static constexpr int NORM_THRESHOLD = 250;
static constexpr float NORM_DIVISOR = 250.0f;
static constexpr int NORM_RANGE = 10;

// Matrix rain animation state
static constexpr int RAIN_COLS = 40;
static constexpr int RAIN_ROWS = 14;
static constexpr int RAIN_Y_START = 12;
static constexpr int RAIN_CELL_W = 6;
static constexpr int RAIN_CELL_H = 8;

struct RainColumn {
    int8_t  headRow;
    uint8_t speed;
    uint8_t counter;
    uint8_t length;
};
static RainColumn rainCols[RAIN_COLS];

static void resetRainColumn(RainColumn& col, bool randomStart) {
    col.headRow = randomStart ? -(rand() % RAIN_ROWS) : -(rand() % 6 + 1);
    col.speed = 1 + (rand() % 3);
    col.counter = 0;
    col.length = 4 + (rand() % 7);
}

// --- Draw wrapped text using canvas built-in font ---
static void drawWrappedText(Canvas& canvas, int16_t x, int16_t y,
                            int16_t maxW, int16_t maxH,
                            const char* text, uint16_t color, uint8_t scale) {
    int charW = 6 * scale;
    int lineH = 8 * scale;
    int cols = maxW / charW;
    if (cols <= 0) return;

    size_t len = strlen(text);
    char lineBuf[50];
    int16_t curY = y;

    for (size_t i = 0; i < len && curY + lineH <= y + maxH; ) {
        size_t lineLen = (len - i < (size_t)cols) ? (len - i) : (size_t)cols;
        memcpy(lineBuf, text + i, lineLen);
        lineBuf[lineLen] = '\0';
        canvas.drawText(x, curY, lineBuf, color, scale);
        curY += lineH;
        i += lineLen;
    }
}

// =====================================================================
// RNG Scene
// =====================================================================

class RNGScene : public Scene {
public:
    StatusBar topBar;
    Label entropyLabel;
    Label hintLabel;
    DisplayMode displayMode = DisplayMode::OTP;

    RNGScene() : Scene("rng") {}

    void setup() {
        topBar.setBounds({0, 0, SCREEN_W, 11});
        topBar.setCenter("ENTROPY COLLECTION");
        topBar.setDrawSeparator(true);
        addWidget(&topBar);

        entropyLabel.setBounds({0, 24, SCREEN_W, 14});
        entropyLabel.setText("Shake to gather entropy");
        entropyLabel.setAlign(Label::Align::Center);
        entropyLabel.setColor(CardGFX::theme().fgSecondary);
        addWidget(&entropyLabel);

        hintLabel.setBounds({0, 124, SCREEN_W, 11});
        hintLabel.setText("Jiggle for faster collection");
        hintLabel.setAlign(Label::Align::Center);
        hintLabel.setColor(CardGFX::theme().fgSecondary);
        addWidget(&hintLabel);

        for (int i = 0; i < RAIN_COLS; i++) {
            resetRainColumn(rainCols[i], true);
        }
    }

    void onEnter() override {
        // Reset state when entering from menu
        keyReady = false;
        keyRngHashReady = false;
        resetRng = true;
        displayMode = DisplayMode::OTP;
        setGatheringState();
    }

    void onTick(uint32_t dt_ms) override {
        if (!keyReady) {
            for (int i = 0; i < RAIN_COLS; i++) {
                rainCols[i].counter++;
                if (rainCols[i].counter >= rainCols[i].speed) {
                    rainCols[i].headRow++;
                    rainCols[i].counter = 0;
                    if (rainCols[i].headRow > RAIN_ROWS + rainCols[i].length) {
                        resetRainColumn(rainCols[i], false);
                    }
                }
            }
            if (RNG.available(sizeof(rngKey))) {
                RNG.rand(rngKey, sizeof(rngKey));
                keyReady = true;
            }
            topBar.markDirty();
        }
    }

    bool onInput(const InputEvent& event) override {
        if (!event.isDown()) return false;

        // ESC returns to menu
        if (event.key == Key::ESCAPE) {
            CardGFX::scenes().pop(Transition::SlideRight);
            return true;
        }

        if (keyRngHashReady) {
            if (event.key == Key::ENTER) {
                keyReady = false;
                keyRngHashReady = false;
                resetRng = true;
                displayMode = DisplayMode::OTP;
                RNG.destroy();
                setGatheringState();
                return true;
            }
            if (event.key == Key::SPACE) {
                switch (displayMode) {
                    case DisplayMode::OTP:      displayMode = DisplayMode::DIGITS;   break;
                    case DisplayMode::DIGITS:   displayMode = DisplayMode::HEX_VIEW; break;
                    case DisplayMode::HEX_VIEW: displayMode = DisplayMode::BINARY;   break;
                    case DisplayMode::BINARY:   displayMode = DisplayMode::OTP;      break;
                }
                updateKeyDisplayWidgets();
                return true;
            }
        }
        return false;
    }

    void onDrawOverlay(Canvas& fb, const Theme& theme) override {
        if (!keyReady) {
            fb.fillRect(0, 12, SCREEN_W, SCREEN_H - 12, theme.bgPrimary);

            char glyph[2] = {0, 0};
            static const char hexChars[] = "0123456789ABCDEF";

            for (int col = 0; col < RAIN_COLS; col++) {
                int16_t x = col * RAIN_CELL_W;
                int head = rainCols[col].headRow;
                int len = rainCols[col].length;

                for (int d = 0; d <= len; d++) {
                    int row = head - d;
                    if (row < 0 || row >= RAIN_ROWS) continue;

                    int16_t y = RAIN_Y_START + row * RAIN_CELL_H;
                    glyph[0] = hexChars[rand() % 16];

                    uint8_t brightness;
                    if (d == 0) {
                        brightness = 255;
                    } else {
                        brightness = 200 - (d * 180 / len);
                    }
                    uint16_t color = HAL::rgb565(d == 0 ? brightness / 2 : 0, brightness, 0);
                    fb.drawText(x, y, glyph, color, 1);
                }
            }
        }

        if (keyRngHashReady) {
            fb.fillRect(0, 12, SCREEN_W, SCREEN_H - 23, theme.bgPrimary);

            switch (displayMode) {
                case DisplayMode::OTP:
                    drawWrappedText(fb, 4, 14, SCREEN_W - 8, 108,
                                    otpStr.c_str(), theme.accent, 1);
                    break;
                case DisplayMode::DIGITS:
                    drawWrappedText(fb, 4, 14, SCREEN_W - 8, 108,
                                    rngStr.c_str(), theme.accent, 1);
                    break;
                case DisplayMode::HEX_VIEW:
                    drawWrappedText(fb, 4, 14, SCREEN_W - 8, 108,
                                    keyHexStr.c_str(), theme.fgPrimary, 1);
                    break;
                case DisplayMode::BINARY:
                    drawWrappedText(fb, 2, 14, SCREEN_W - 4, 108,
                                    bigBinStr.c_str(), theme.success, 1);
                    break;
            }
        }
    }

    void setGatheringState() {
        topBar.setLeft("");
        topBar.setCenter("ENTROPY COLLECTION");
        topBar.setRight("");
        entropyLabel.setVisible(true);
        hintLabel.setVisible(false);
    }

    void setKeyReadyState(const uint8_t* hash = nullptr) {
        topBar.setCenter("KEY GENERATED");
        if (hash) {
            char fp[10];
            snprintf(fp, sizeof(fp), "%02X%02X%02X%02X", hash[0], hash[1], hash[2], hash[3]);
            topBar.setLeft(fp);
        }
        entropyLabel.setVisible(false);
        hintLabel.setText("[ENT]Reset [SPC]View [ESC]Menu");
        hintLabel.setVisible(true);
        updateKeyDisplayWidgets();
    }

    void updateKeyDisplayWidgets() {
        switch (displayMode) {
            case DisplayMode::OTP:      topBar.setRight("OTP");    break;
            case DisplayMode::DIGITS:   topBar.setRight("DIGITS"); break;
            case DisplayMode::HEX_VIEW: topBar.setRight("HEX");    break;
            case DisplayMode::BINARY:   topBar.setRight("BINARY"); break;
        }
        topBar.markDirty();
        hintLabel.markDirty();
    }
};

#endif // RNG_SCENE_H
