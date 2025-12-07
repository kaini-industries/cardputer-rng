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

SHA256 hashMachine = SHA256();
SHA256 rngHashMachine = SHA256();
TransistorNoiseSource noise1(G3);
TransistorNoiseSource noise2(G4);
TransistorNoiseSource noise3(G5);
TransistorNoiseSource noise4(G6);
TransistorNoiseSource noise5(G13);
m5::imu_data_t imuData;
String entropy = "";

const int KEY_SIZE = 256;
bool keyReady = false;
bool keyRngHashReady = false;
bool resetRng = false;
byte rngKey[KEY_SIZE];
bool serialOutput = false;

// uint8_t hash[hashMachine.HASH_SIZE];
uint8_t hash[KEY_SIZE];
// uint8_t hash[256];

// uint8_t rngHash[rngHashMachine.HASH_SIZE];
uint8_t rngHash[KEY_SIZE];
// uint8_t rngHash[256];

String rngHashStr = "";

std::vector<uint8_t> doubleToHexBytes(double x) {
  std::vector<uint8_t> byte_return; // std::vector<byte> byte_return;
  uint8_t bytes[8];
  std::memcpy(bytes, &x, sizeof(double));

  for (int i = 7; i >= 0; --i) {
    byte_return.push_back(bytes[i]);
    // byte_return.push_back(static_cast<int>(bytes[i]));
    /*
    std::cout << std::hex << std::uppercase
              << std::setw(2) << std::setfill('0')
              << static_cast<int>(bytes[i]);
    */
  }

  return byte_return;
}

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

  hashMachine.clear();
  hashMachine.reset();
  rngHashMachine.clear();
  rngHashMachine.reset();

  M5Cardputer.Display.setFont(&fonts::FreeMonoBold9pt7b);
  M5Cardputer.Display.setCursor(0, 0);

  Serial.begin(115200);
  delay(10);
  Serial.println("Cardputer ADV Ready...");
}

void loop() {
  M5Cardputer.update();
  // M5Cardputer.Display.fillScreen(TFT_BLACK);

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  hashMachine.clear();
  hashMachine.reset();
  rngHashMachine.clear();
  rngHashMachine.reset();

  if (resetRng) {
    RNG.begin("CardputerRNG v1");
    resetRng = false;
  }

  RNG.loop();

  // SIMPLE RANDOM
  // int number = random(0, 9);
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(number));

  /* // TEST STIR WITH SIGNED DOUBLE
  double testDouble = -0.123;
  const std::vector<uint8_t> vectorOfBytes = doubleToHexBytes(testDouble);
  for (uint8_t b : vectorOfBytes) {
    // std::string s_str = std::to_string(b);
    // M5Cardputer.Display.setCursor(0, 0);
    // M5Cardputer.Display.printf(s_str.c_str());
    // delay(500);
    uint8_t stirValue = b;
    const uint8_t *stirPointer = &stirValue;
    RNG.stir(stirPointer, sizeof(stirValue));
  }
  */

  FloatBytes accelX;
  accelX.f_val = imuData.accel.x;
  std::string accelXString = std::to_string(accelX.f_val);
  // uint8_t accelXArr[] = {}; // byte accelXArr[] = {};
  DoubleBytes accelX_d;
  accelX_d.d_val = imuData.accel.x;
  std::string accelX_dString = std::to_string(accelX_d.d_val);
  // uint8_t accelX_dArr[] = {}; // byte accelX_dArr[] = {};

  FloatBytes gyroX;
  gyroX.f_val = imuData.gyro.x;
  std::string gyroXString = std::to_string(gyroX.f_val);
  // uint8_t gyroXArr[] = {}; // byte gyroXArr[] = {};
  DoubleBytes gyroX_d;
  gyroX_d.d_val = imuData.gyro.x;
  std::string gyroX_dString = std::to_string(gyroX_d.d_val);
  // uint8_t gyroX_dArr[] = {}; // byte gyroX_dArr[] = {};

  FloatBytes accelY;
  accelY.f_val = imuData.gyro.x;
  std::string accelYString = std::to_string(accelY.f_val);
  // uint8_t accelYArr[] = {}; // byte accelYArr[] = {};
  DoubleBytes accelY_d;
  accelY_d.d_val = imuData.gyro.x;
  std::string accelY_dString = std::to_string(accelY_d.d_val);
  // uint8_t accelY_dArr[] = {}; // byte accelY_dArr[] = {};

  FloatBytes gyroY;
  gyroY.f_val = imuData.gyro.y;
  std::string gyroYString = std::to_string(gyroY.f_val);
  // uint8_t gyroYArr[] = {}; // byte gyroYArr[] = {};
  DoubleBytes gyroY_d;
  gyroY_d.d_val = imuData.gyro.y;
  std::string gyroY_dString = std::to_string(gyroY_d.d_val);
  // uint8_t gyroY_dArr[] = {}; // byte gyroY_dArr[] = {};

  std::string allImuDataStr =
    accelXString +
    gyroXString +
    accelYString +
    gyroYString
  ;

  const char* cAllImuDataStdStr = allImuDataStr.c_str();
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(cAllImuDataStdStr));

  std::vector<uint8_t> allImuDataVector(allImuDataStr.begin(), allImuDataStr.end());
  uint8_t *allImuUint8_t = &allImuDataVector[0];
  const uint8_t *stirPointer = allImuUint8_t;
  RNG.stir(stirPointer, sizeof(allImuUint8_t));

  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.printf("%s", (char*) allImuUint8_t);

  hashMachine.update((const uint8_t*) cAllImuDataStdStr, strlen(cAllImuDataStdStr));
  hashMachine.finalize(hash, sizeof(hash));
  
  String entropyPulse = "";

  for (size_t i = 0; i < hashMachine.HASH_SIZE; i++) { // USE KEY_SIZE ??
    // M5Cardputer.Display.setCursor(0, 20);
    // M5Cardputer.Display.print(hash[i], HEX);
    // delay(1000);
    entropy += hash[i];
    entropyPulse += hash[i];
  }

  hashMachine.clear();
  hashMachine.reset();

  int entropyLength = entropy.length();
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(entropyLength));
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(entropyPulse);
  // M5Cardputer.Display.printf(entropyPulse.c_str());
  // M5Cardputer.Display.setCursor(0, 60);
  // M5Cardputer.Display.print(String(entropy));

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
    // // M5Cardputer.Display.fillScreen(TFT_BLACK);
    // M5Cardputer.Display.setCursor(0, 0);
    // M5Cardputer.Display.printf("%s", (char*) "Key generated: ");
    // M5Cardputer.Display.setCursor(0, 20);
    // M5Cardputer.Display.printf("%s", (char*) key);

    rngHashMachine.clear();
    rngHashMachine.reset();

    rngHashMachine.update((const uint8_t*) rngKey, sizeof(rngKey));
    rngHashMachine.finalize(rngHash, sizeof(rngHash));

    for (size_t i = 0; i < rngHashMachine.HASH_SIZE; i++) { // USE KEY_SIZE ??
      rngHashStr += rngHash[i];
    }

    keyRngHashReady = true;
    serialOutput = true;
  }
  if (keyRngHashReady) {
    // M5Cardputer.Display.fillScreen(TFT_BLACK);

    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.printf("%s", (char*) "Hash generated:      ");

    M5Cardputer.Display.setCursor(0, 20);
    M5Cardputer.Display.print(rngHashStr);
    // M5Cardputer.Display.printf("%s", (char*) rngHash);

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
    // Serial.println(rngHashStr);
    Serial.println(rngHashStr);

    serialOutput = false;
  }

  // MAIN DELAY
  if (!keyRngHashReady) {
    // delay(0);
  }
}