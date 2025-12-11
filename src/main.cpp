#include <M5Cardputer.h>
#include <Crypto.h>
#include <SHA256.h>
#include <RNG.h>
#include <TransistorNoiseSource.h>
#include <SPI.h>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cstring>
#include <vector>
#include <bitset>

SHA256 hashMachine = SHA256();
SHA256 rngHashMachine = SHA256();
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

struct FloatBytes {
  float f_val;
  byte b_array[sizeof(float)];
};

struct DoubleBytes {
  double d_val;
  byte d_array[sizeof(double)];
};

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  RNG.begin("CardputerRNG v1");

  RNG.addNoiseSource(noise1);
  RNG.addNoiseSource(noise2);
  RNG.addNoiseSource(noise3);
  RNG.addNoiseSource(noise4);
  RNG.addNoiseSource(noise5);

  M5Cardputer.Display.setFont(&fonts::FreeMonoBold9pt7b);
  M5Cardputer.Display.setCursor(0, 0);

  Serial.begin(115200);
  delay(10);
  Serial.println("Cardputer ADV Ready...");
}

void loop() {
  M5Cardputer.update();

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  uint8_t rngHash[KEY_SIZE];
  std::string rngStr = "";
  std::string bigBinStr = "";
  std::string rngHashStr = "";

  hashMachine.clear();
  hashMachine.reset();
  rngHashMachine.clear();
  rngHashMachine.reset();

  if (resetRng) {
    RNG.begin("CardputerRNG v1");
    resetRng = false;
  }

  RNG.loop();

  FloatBytes accelX;
  accelX.f_val = imuData.accel.x;
  std::string accelXString = std::to_string(accelX.f_val);
  uint8_t accelXHash[KEY_SIZE];
  hashMachine.update((uint8_t*)&accelX.f_val, sizeof(float));
  hashMachine.finalize(accelXHash, sizeof(accelXHash));
  RNG.stir(accelXHash, sizeof(accelXHash));
  hashMachine.clear(); hashMachine.reset();

  FloatBytes gyroX;
  gyroX.f_val = imuData.gyro.x;
  std::string gyroXString = std::to_string(gyroX.f_val);
  uint8_t gyroXHash[KEY_SIZE];
  hashMachine.update((uint8_t*)&gyroX.f_val, sizeof(float));
  hashMachine.finalize(gyroXHash, sizeof(gyroXHash));
  RNG.stir(gyroXHash, sizeof(gyroXHash));
  hashMachine.clear(); hashMachine.reset();

  FloatBytes accelY;
  accelY.f_val = imuData.gyro.x;
  std::string accelYString = std::to_string(accelY.f_val);
  uint8_t accelYHash[KEY_SIZE];
  hashMachine.update((uint8_t*)&accelY.f_val, sizeof(float));
  hashMachine.finalize(accelYHash, sizeof(accelYHash));
  RNG.stir(accelYHash, sizeof(accelYHash));
  hashMachine.clear(); hashMachine.reset();

  FloatBytes gyroY;
  gyroY.f_val = imuData.gyro.y;
  std::string gyroYString = std::to_string(gyroY.f_val);
  uint8_t gyroYHash[KEY_SIZE];
  hashMachine.update((uint8_t*)&gyroY.f_val, sizeof(float));
  hashMachine.finalize(gyroYHash, sizeof(gyroYHash));
  RNG.stir(gyroYHash, sizeof(gyroYHash));
  hashMachine.clear(); hashMachine.reset();

  /*
  // FOR TESTING IMU READS
  std::string allImuDataStr =
    accelXString +
    gyroXString +
    accelYString +
    gyroYString
  ;
  // const char* cAllImuDataStdStr = allImuDataStr.c_str();
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(cAllImuDataStdStr));
  */

  if (!keyReady) {
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.printf("%s", (char*) "Gathering entropy...");
    // M5Cardputer.Display.setCursor(0, 20);
    // M5Cardputer.Display.printf("%s", (char*) "Jiggle!");
  }
  if (!keyReady && RNG.available(sizeof(rngKey))) {
    RNG.rand(rngKey, sizeof(rngKey));
    keyReady = true;
  }
  if (keyReady && !keyRngHashReady) {
    // M5Cardputer.Display.fillScreen(TFT_BLACK);

    for (size_t i = 0; i < sizeof(rngKey); ++i) {
      int keyItemInt = static_cast<int>(rngKey[i]);
      if (keyItemInt > 249) {
        continue;
      }
      int normalizedKeyItemInt = floor((static_cast<float>(keyItemInt) / 250.0f) * 10.0f);
      std::string keyItemIntStr = std::to_string(normalizedKeyItemInt);
      rngStr += keyItemIntStr;
    }

    rngHashMachine.clear(); rngHashMachine.reset();
    rngHashMachine.update((const uint8_t*) rngKey, sizeof(rngKey));
    rngHashMachine.finalize(rngHash, sizeof(rngHash));

    for (size_t i = 0; i < rngHashMachine.HASH_SIZE; i++) {
      int itemInt = static_cast<int>(rngHash[i]);
      if (itemInt > 249) {
        continue;
      }
      int normalizedItemInt = floor((static_cast<float>(itemInt) / 250.0f) * 10.0f);

      std::string delim = ", ";
      std::string itemIntStr = std::to_string(normalizedItemInt);
      // rngHashStr += itemIntStr + delim;
      rngHashStr += itemIntStr;

      std::string binStr = std::bitset<8>(rngKey[i]).to_string().c_str();
      bigBinStr += binStr.c_str() + delim;

      // rngHashStr += rngHash[i]; // Hex style byte
    }

    keyRngHashReady = true;
    serialOutput = true;
  }
  if (keyRngHashReady) {
    // M5Cardputer.Display.setCursor(0, 0);
    // M5Cardputer.Display.printf("%s", (char*) "Hash generated:      ");

    M5Cardputer.Display.setCursor(0, 0);
    // M5Cardputer.Display.print(rngStr.c_str());
    M5Cardputer.Display.print(rngStr.c_str());
    // M5Cardputer.Display.print(rngHashStr.c_str());
    // M5Cardputer.Display.print(bigBinStr); // .c_str() IS NOT NEEDED ??

    M5Cardputer.Display.setCursor(0, 120);
    M5Cardputer.Display.printf("%s", (char*) "Press enter to reset.");

    // LISTEN FOR KEYBOARD INPUT
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        if (status.enter) {
          M5Cardputer.Display.fillScreen(TFT_BLACK);
          keyReady = false;
          keyRngHashReady = false;
          std::string rngStr = "";
          std::string bigBinStr = "";
          std::string rngHashStr = "";
          resetRng = true;
          RNG.destroy();
        }
      }
    }
  }

  if (keyRngHashReady && serialOutput) {
    // Serial.printf("%s", rngHashStr.c_str());
    // Serial.printf("%s", bigBinStr.c_str());

    // Serial stream rngKey
    for (size_t i = 0; i < sizeof(rngKey); ++i) {
      int keyItemInt = static_cast<int>(rngKey[i]);
      if (keyItemInt > 249) {
        continue;
      }
      int normalizedKeyItemInt = floor((static_cast<float>(keyItemInt) / 250.0f) * 10.0f);
      std::string keyItemIntStr = std::to_string(normalizedKeyItemInt);
      Serial.printf("%s", keyItemIntStr.c_str());

      // Serial.printf("%02X", rngKey[i]);
      // if (i < sizeof(rngKey) - 1) Serial.print(" ");
      // std::string binStr = std::bitset<8>(rngKey[i]).to_string();
      // Serial.print(binStr.c_str());
    }
    // Serial.println();

    // Serial stream rngHash
    for (size_t i = 0; i < rngHashMachine.HASH_SIZE; ++i) {
      // Serial.printf("%02X", rngHash[i]);
      // if (i < sizeof(rngHash) - 1) Serial.print(" ");
    }
    // Serial.println();

    serialOutput = false;
  }

  if (!keyRngHashReady) {
    // delay(0);
  }
}