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

TransistorNoiseSource noise1(G3);
TransistorNoiseSource noise2(G4);
TransistorNoiseSource noise3(G5);
TransistorNoiseSource noise4(G6);
TransistorNoiseSource noise5(G13);

m5::imu_data_t imuData;

const int KEY_SIZE = 256;
byte rngKey[KEY_SIZE];

bool keyReady = false;
bool outputReady = false;
bool outputSent = false;
bool serialReady = false;
bool serialSent = false;

bool resetRng = false;

struct FloatBytes {
  float f_val;
  byte b_array[sizeof(float)];
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

  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

      if (
        true // status.enter // && outputSent
    ) {
        M5Cardputer.Display.fillScreen(TFT_BLACK);
        
        std::string rngStr = "";
        keyReady = false;
        outputReady = false;
        outputSent = false;
        serialReady = false;
        serialSent = false;

        RNG.destroy();
        resetRng = true;

        delay(100);
      }
    }
  }

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  uint8_t rngHash[KEY_SIZE];
  std::string rngStr = "";

  hashMachine.clear();
  hashMachine.reset();

  if (resetRng) {
    RNG.begin("CardputerRNG v1");
    resetRng = false;
  }

  RNG.loop();

  FloatBytes accelX;
  accelX.f_val = imuData.accel.x;
  uint8_t accelXHash[KEY_SIZE];
  hashMachine.update((uint8_t*)&accelX.f_val, sizeof(float));
  hashMachine.finalize(accelXHash, sizeof(accelXHash));
  RNG.stir(accelXHash, sizeof(accelXHash));
  hashMachine.clear(); hashMachine.reset();

  FloatBytes gyroX;
  gyroX.f_val = imuData.gyro.x;
  uint8_t gyroXHash[KEY_SIZE];
  hashMachine.update((uint8_t*)&gyroX.f_val, sizeof(float));
  hashMachine.finalize(gyroXHash, sizeof(gyroXHash));
  RNG.stir(gyroXHash, sizeof(gyroXHash));
  hashMachine.clear(); hashMachine.reset();

  FloatBytes accelY;
  accelY.f_val = imuData.gyro.x;
  uint8_t accelYHash[KEY_SIZE];
  hashMachine.update((uint8_t*)&accelY.f_val, sizeof(float));
  hashMachine.finalize(accelYHash, sizeof(accelYHash));
  RNG.stir(accelYHash, sizeof(accelYHash));
  hashMachine.clear(); hashMachine.reset();

  FloatBytes gyroY;
  gyroY.f_val = imuData.gyro.y;
  uint8_t gyroYHash[KEY_SIZE];
  hashMachine.update((uint8_t*)&gyroY.f_val, sizeof(float));
  hashMachine.finalize(gyroYHash, sizeof(gyroYHash));
  RNG.stir(gyroYHash, sizeof(gyroYHash));
  hashMachine.clear(); hashMachine.reset();

  if (!keyReady) {
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.printf("%s", (char*) "Gathering entropy...");
  }

  if (!keyReady && RNG.available(sizeof(rngKey))) {
    RNG.rand(rngKey, sizeof(rngKey));
    keyReady = true;
  }

  if (keyReady) {
    for (size_t i = 0; i < sizeof(rngKey); ++i) {
      int keyItemInt = static_cast<int>(rngKey[i]);
      if (keyItemInt > 249) {
        continue;
      }

      // int normalizedKeyItemInt = floor((static_cast<float>(keyItemInt) / 250.0f) * 10.0f);
      // std::string keyItemIntStr = std::to_string(normalizedKeyItemInt);

      int normalizedKeyItemInt = floor((static_cast<float>(keyItemInt) / 50.0f) * 10.0f);
      std::string keyItemIntStr = (normalizedKeyItemInt < 10 ? "0" : "") + std::to_string(normalizedKeyItemInt);
      
      rngStr += keyItemIntStr + " ";
    }

    outputReady = true;
    serialReady = true;

    delay(100);
  }

  if (outputReady && !outputSent) {
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.print(rngStr.c_str());

    // M5Cardputer.Display.setCursor(0, 120);
    // M5Cardputer.Display.printf("%s", (char*) "Press enter to reset.");

    outputSent = true;

    delay(1200);
  }

  if (serialReady && !serialSent) {
    Serial.printf("%s", rngStr.c_str());
    Serial.println();

    serialReady = false;
    serialSent = true;

    delay(100);
  }
}