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

// --- Helper: hash and stir a float into RNG ---
static void stirFloat(float value) {
    uint8_t hash[SHA256::HASH_SIZE];
    hashMachine.update((uint8_t*)&value, sizeof(float));
    hashMachine.finalize(hash, sizeof(hash));
    RNG.stir(hash, sizeof(hash));
    hashMachine.clear();
    hashMachine.reset();
}

// --- Scenes ---
#include "rng_scene.h"
#include "menu_scene.h"

RNGScene rngScene;
MenuScene menuScene;

// =====================================================================
// Setup & Loop
// =====================================================================

void setup() {
    if (!CardGFX::init(1, 128)) {
        while (true) delay(1000);
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

    // Seed rand() with hardware entropy so rain animation isn't deterministic
    srand((unsigned)esp_random());

    // Setup and register scenes
    rngScene.setup();
    menuScene.setup();
    CardGFX::scenes().registerScene(&rngScene);
    CardGFX::scenes().registerScene(&menuScene);

    // Push menu as the initial scene
    CardGFX::scenes().push(&menuScene);
    CardGFX::tick();

    // Wait up to 3s for USB CDC host to connect, keeping animation alive
    uint32_t serialWait = millis();
    while (!Serial && (millis() - serialWait < 3000)) {
        RNG.loop();
        CardGFX::tick();
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
