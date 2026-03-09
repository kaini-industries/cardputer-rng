#include <cardgfx.h>
#include <M5Cardputer.h>
#include <Crypto.h>
#include <SHA256.h>
#include <RNG.h>
#include <TransistorNoiseSource.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <bitset>
#include <string>
#include <cstdlib>

using namespace CardGFX;

// --- Crypto / RNG ---
SHA256 hashMachine;
TransistorNoiseSource noise1(G3);
TransistorNoiseSource noise2(G4);
TransistorNoiseSource noise3(G5);
TransistorNoiseSource noise4(G6);
TransistorNoiseSource noise5(G13);
m5::imu_data_t imuData;

const int KEY_SIZE = 256;
bool keyReady = false;
bool keyRngHashReady = false;
bool resetRng = false;
byte rngKey[KEY_SIZE];
bool serialOutput = false;

// Persistent strings (built once per key generation)
std::string rngStr;
std::string otpStr;
std::string bigBinStr;
std::string keyHexStr;

// --- Display state ---
enum class DisplayMode { OTP, DIGITS, HEX_VIEW, BINARY };
DisplayMode displayMode = DisplayMode::OTP;
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

// --- Helper: hash and stir a float into RNG ---
static void stirFloat(float value) {
    uint8_t hash[SHA256::HASH_SIZE];
    hashMachine.update((uint8_t*)&value, sizeof(float));
    hashMachine.finalize(hash, sizeof(hash));
    RNG.stir(hash, sizeof(hash));
    hashMachine.clear();
    hashMachine.reset();
}

// --- Draw wrapped text using canvas built-in font ---
static void drawWrappedText(Canvas& canvas, int16_t x, int16_t y,
                            int16_t maxW, int16_t maxH,
                            const char* text, uint16_t color, uint8_t scale) {
    int charW = 6 * scale;  // 5px char + 1px spacing
    int lineH = 8 * scale;  // 7px char + 1px spacing
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

        // Initialize rain columns once (persists across key resets)
        for (int i = 0; i < RAIN_COLS; i++) {
            resetRainColumn(rainCols[i], true);
        }
    }

    void onEnter() override {
        setGatheringState();
    }

    void onTick(uint32_t dt_ms) override {
        if (!keyReady) {
            // Advance rain columns
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
            // Check for actual key readiness
            if (RNG.available(sizeof(rngKey))) {
                RNG.rand(rngKey, sizeof(rngKey));
                keyReady = true;
            }
            topBar.markDirty();
        }
    }

    bool onInput(const InputEvent& event) override {
        if (!event.isDown()) return false;

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

            // Matrix rain
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
            // Clear content area to remove stale gathering-phase pixels
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
        hintLabel.setText("[ENT]Reset [SPC]View");
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

// =====================================================================
// Globals
// =====================================================================

RNGScene rngScene;

// =====================================================================
// Setup & Loop
// =====================================================================

void setup() {
    if (!CardGFX::init(1, 128)) {
        while (true) delay(1000);  // Can't print — serial not ready yet
    }

    // Initialize IMU
    M5.Imu.update();
    imuData = M5.Imu.getImuData();

    // Initialize RNG
    RNG.begin("CardputerRNG v1");
    RNG.addNoiseSource(noise1);
    RNG.addNoiseSource(noise2);
    RNG.addNoiseSource(noise3);
    RNG.addNoiseSource(noise4);
    RNG.addNoiseSource(noise5);

    // Setup and push scene, render first frame so UI appears immediately
    rngScene.setup();
    CardGFX::scenes().registerScene(&rngScene);
    CardGFX::scenes().push(&rngScene);
    CardGFX::tick();

    // Wait up to 3s for USB CDC host to connect (after UI is visible)
    uint32_t serialWait = millis();
    while (!Serial && (millis() - serialWait < 3000)) {
        delay(10);
    }

    Serial.println("BOOT OK");
}

void loop() {
    // --- IMU + RNG (runs every frame regardless of scene) ---
    M5.Imu.update();
    imuData = M5.Imu.getImuData();

    if (resetRng) {
        RNG.begin("CardputerRNG v1");
        resetRng = false;
    }

    RNG.loop();

    // Stir IMU data into entropy pool
    stirFloat(imuData.accel.x);
    stirFloat(imuData.accel.y);
    stirFloat(imuData.accel.z);
    stirFloat(imuData.gyro.x);
    stirFloat(imuData.gyro.y);
    stirFloat(imuData.gyro.z);

    // --- Process key (runs once per generation) ---
    if (keyReady && !keyRngHashReady) {
        rngStr.clear();
        otpStr.clear();
        bigBinStr.clear();
        keyHexStr.clear();
        keyHexStr.reserve(256 * 3);
        rngStr.reserve(256);
        bigBinStr.reserve(256 * 8);
        otpStr.reserve(256 * 2);

        int digitCount = 0;
        char hexBuf[4];
        for (size_t i = 0; i < sizeof(rngKey); ++i) {
            int keyItemInt = static_cast<int>(rngKey[i]);
            // Build hex string
            snprintf(hexBuf, sizeof(hexBuf), "%02X ", rngKey[i]);
            keyHexStr += hexBuf;
            // Build normalized digit string + OTP in single pass
            if (keyItemInt > NORM_THRESHOLD - 1) continue;
            int normalized = static_cast<int>(floorf((static_cast<float>(keyItemInt) / NORM_DIVISOR) * NORM_RANGE));
            char digitChar = '0' + normalized;
            rngStr += digitChar;
            bigBinStr += std::bitset<8>(rngKey[i]).to_string();
            if (digitCount > 0 && digitCount % 2 == 0) otpStr += ' ';
            otpStr += digitChar;
            digitCount++;
        }

        // Compute SHA256 fingerprint for status bar
        uint8_t rngHash[SHA256::HASH_SIZE];
        hashMachine.clear();
        hashMachine.reset();
        hashMachine.update((const uint8_t*)rngKey, sizeof(rngKey));
        hashMachine.finalize(rngHash, sizeof(rngHash));

        keyRngHashReady = true;
        serialOutput = true;
        rngScene.setKeyReadyState(rngHash);
    }

    // --- CardGFX frame (input, tick, draw, push to screen) ---
    CardGFX::tick();

    // --- Serial output (once per key generation) ---
    if (keyRngHashReady && serialOutput) {
        Serial.println("--- KEY (OTP) ---");
        Serial.println(otpStr.c_str());
        Serial.println("--- KEY (normalized digits) ---");
        Serial.println(rngStr.c_str());
        Serial.println("--- KEY (binary) ---");
        Serial.println(bigBinStr.c_str());
        Serial.println("--- KEY (raw hex) ---");
        for (size_t i = 0; i < sizeof(rngKey); ++i) {
            Serial.printf("%02X", rngKey[i]);
            if ((i + 1) % 32 == 0) Serial.println();
        }
        Serial.println();
        serialOutput = false;
    }
}
