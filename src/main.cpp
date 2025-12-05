#include <M5Cardputer.h>
#include <Crypto.h>
#include <SHA256.h>
#include <RNG.h>
#include <TransistorNoiseSource.h>
#include <SPI.h>

SHA256 hashMachine = SHA256();
TransistorNoiseSource noise(G3);
m5::imu_data_t imuData;
String entropy = "";

// RNG TEST VARS
bool haveKey = false;
byte key[32];
//

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
  RNG.addNoiseSource(noise);

  hashMachine.clear();
  hashMachine.reset();

  M5Cardputer.Display.setFont(&fonts::FreeMonoBold9pt7b);
  M5Cardputer.Display.setCursor(0, 0);
}

void loop() {
  M5Cardputer.update();
  M5Cardputer.Display.fillScreen(TFT_BLACK);

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  hashMachine.clear();
  hashMachine.reset();

  RNG.loop();

  // SIMPLE RANDOM
  // int number = random(0, 9);
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(number));

  FloatBytes accelX;
  accelX.f_val = imuData.accel.x;
  std::string accelXString = std::to_string(accelX.f_val);
  // uint8_t accelXArr[] = {};
  // byte accelXArr[] = {};

  FloatBytes gyroX;
  gyroX.f_val = imuData.gyro.x;
  std::string gyroXString = std::to_string(gyroX.f_val);
  // uint8_t gyroXArr[] = {};
  // byte gyroXArr[] = {};

  FloatBytes accelY;
  accelY.f_val = imuData.gyro.x;
  std::string accelYString = std::to_string(accelY.f_val);
  // uint8_t accelYArr[] = {};
  // byte accelYArr[] = {};

  FloatBytes gyroY;
  gyroY.f_val = imuData.gyro.y;
  std::string gyroYString = std::to_string(gyroY.f_val);
  // uint8_t gyroYArr[] = {};
  // byte gyroYArr[] = {};

  FloatBytes accelZ;
  accelZ.f_val = imuData.accel.y;
  std::string accelZString = std::to_string(accelZ.f_val);
  // uint8_t accelZArr[] = {};
  // byte accelZArr[] = {};

  FloatBytes gyroZ;
  gyroZ.f_val = imuData.gyro.z;
  std::string gyroZString = std::to_string(gyroZ.f_val);
  // uint8_t gyroZArr[] = {};
  // byte gyroZArr[] = {};

  std::string allImuDataStr =
    accelXString +
    gyroXString +
    accelYString +
    gyroYString +
    accelZString +
    gyroZString
  ;

  const char* cAllImuDataStdStr = allImuDataStr.c_str();

  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(cAllImuDataStdStr));

  uint8_t hash[hashMachine.HASH_SIZE];
  hashMachine.update((const uint8_t*) cAllImuDataStdStr, strlen(cAllImuDataStdStr)); // Feed the data, nom nom
  hashMachine.finalize(hash, sizeof(hash));

  String entropyPulse = "";

  for (size_t i = 0; i < hashMachine.HASH_SIZE; i++) {
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

  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print(entropyPulse);
  // M5Cardputer.Display.printf(entropyPulse.c_str());

  // M5Cardputer.Display.setCursor(0, 60);
  // M5Cardputer.Display.print(String(entropy));

  // RNG TEST LOGIC
  if (!haveKey && RNG.available(sizeof(key))) {
    RNG.rand(key, sizeof(key));
    haveKey = true;
  }
  if (!haveKey) {
    M5Cardputer.Display.setCursor(0, 80);
    M5Cardputer.Display.print(0);
  }
  if (haveKey) {
    M5Cardputer.Display.setCursor(0, 80);
    M5Cardputer.Display.print(1);
    M5Cardputer.Display.setCursor(0, 80);
    M5Cardputer.Display.printf("%s", (char*) key);
  }
  //

  // MAIN PULSE DELAY
  delay(1500);
}