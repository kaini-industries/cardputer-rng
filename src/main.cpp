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

std::string hexToBinaryStr(const std::string& hex) {
    // Convert hex string -> integer
    unsigned int value = std::stoul(hex, nullptr, 16);
    // Convert integer -> 8-bit binary
    return std::bitset<8>(value).to_string();
}

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
  String rngHashStr = "";

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
    M5Cardputer.Display.setCursor(0, 20);
    M5Cardputer.Display.printf("%s", (char*) "Get jiggling!");
  }
  if (!keyReady && RNG.available(sizeof(rngKey))) {
    RNG.rand(rngKey, sizeof(rngKey));
    keyReady = true;
  }
  if (keyReady && !keyRngHashReady) {
    // M5Cardputer.Display.fillScreen(TFT_BLACK);

    rngHashMachine.clear(); rngHashMachine.reset();
    rngHashMachine.update((const uint8_t*) rngKey, sizeof(rngKey));
    rngHashMachine.finalize(rngHash, sizeof(rngHash));

    for (size_t i = 0; i < rngHashMachine.HASH_SIZE; i++) {
      rngHashStr += rngHash[i];
    }

    keyRngHashReady = true;
    serialOutput = true;
  }
  if (keyRngHashReady) {
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.printf("%s", (char*) "Hash generated:      ");

    M5Cardputer.Display.setCursor(0, 20);
    M5Cardputer.Display.print(rngHashStr);

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
          rngHashStr = "";
          resetRng = true;
          RNG.destroy();
        }
      }
    }
  }

  if (keyRngHashReady && serialOutput) {
    // Serial.print(rngHashStr);
    
    // Print rngKey as hex values
    for (size_t i = 0; i < sizeof(rngKey); ++i) {
      // Serial.printf("%02X", rngKey[i]);
      // if (i < sizeof(rngKey) - 1) Serial.print(" ");
      std::string binStr = std::bitset<8>(rngKey[i]).to_string();
      Serial.print(binStr.c_str());
    }
    Serial.println();

    // Print rngHash as hex values
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