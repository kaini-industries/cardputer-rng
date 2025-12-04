#include <M5Cardputer.h>
#include <Crypto.h>
#include <SHA256.h>
#include <RNG.h>

SHA256 hashMachine = SHA256();
String allImuDataStr = "";

m5::imu_data_t imuData;

struct FloatBytes {
  float f_val;
  byte b_array[sizeof(float)];
};

String entropy = "";

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  M5.Imu.update();
  imuData = M5.Imu.getImuData();
  String allImuDataStr = "";

  // randomSeed(analogRead(0)); // hm...

  M5Cardputer.Display.setFont(&fonts::FreeMonoBold9pt7b);
  M5Cardputer.Display.setCursor(0, 0);

  hashMachine.clear();
  hashMachine.reset();
}

void loop() {
  SHA256 hashMachine = SHA256();
  hashMachine.clear();
  hashMachine.reset();

  M5Cardputer.update();

  // int number = random(0, 9);
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(number)+" PULSE");

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  FloatBytes accelX;
  accelX.f_val = imuData.accel.x;
  std::string accelXString = std::to_string(accelX.f_val);
  // uint8_t accelXArr[] = {};
  byte accelXArr[] = {};

  FloatBytes gyroX;
  gyroX.f_val = imuData.gyro.x;
  std::string gyroXString = std::to_string(gyroX.f_val);
  // uint8_t gyroXArr[] = {};
  byte gyroXArr[] = {};

  FloatBytes accelY;
  accelY.f_val = imuData.gyro.x;
  std::string accelYString = std::to_string(accelY.f_val);
  // uint8_t accelYArr[] = {};
  byte accelYArr[] = {};

  FloatBytes gyroY;
  gyroY.f_val = imuData.gyro.y;
  std::string gyroYString = std::to_string(gyroY.f_val);
  // uint8_t gyroYArr[] = {};
  byte gyroYArr[] = {};

  FloatBytes accelZ;
  accelZ.f_val = imuData.accel.y;
  std::string accelZString = std::to_string(accelZ.f_val);
  // uint8_t accelZArr[] = {};
  byte accelZArr[] = {};

  FloatBytes gyroZ;
  gyroZ.f_val = imuData.gyro.z;
  std::string gyroZString = std::to_string(gyroZ.f_val);
  // uint8_t gyroZArr[] = {};
  byte gyroZArr[] = {};

  accelX.f_val = imuData.accel.x;
  gyroX.f_val = imuData.gyro.x;
  accelY.f_val = imuData.accel.y;
  gyroY.f_val = imuData.gyro.y;
  accelZ.f_val = imuData.accel.z;
  gyroZ.f_val = imuData.gyro.z;

  std::string allImuDataStr =
    accelXString +
    gyroXString +
    accelYString +
    gyroYString +
    accelZString +
    gyroZString
  ;

  std::string allImuDataStdStr = allImuDataStr;
  const char* cAllImuDataStdStr = allImuDataStdStr.c_str();
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print(String(cAllImuDataStdStr));

  uint8_t hash[hashMachine.HASH_SIZE];
  // uint8_t hash[32]; // ??
  hashMachine.update((const uint8_t*) cAllImuDataStdStr, strlen(cAllImuDataStdStr)); // Feed the data, nom nom
  hashMachine.finalize(hash, sizeof(hash)); // Finalize and get the hash
  for (size_t i = 0; i < hashMachine.HASH_SIZE; i++) {
    // M5Cardputer.Display.setCursor(0, 20);
    // M5Cardputer.Display.print(hash[i], HEX); // Print each byte in hex
    // delay(1000);
    entropy += hash[i];
  }

  hashMachine.clear();
  hashMachine.reset();

  int entropyLength = entropy.length();
  M5Cardputer.Display.setCursor(0, 40);
  M5Cardputer.Display.print(String(entropyLength));
  M5Cardputer.Display.setCursor(0, 60);
  M5Cardputer.Display.print(String(entropy));

  /* // CHARGING DEMO
  bool isCharging = M5Cardputer.Power.isCharging();
  int batteryLevel = M5Cardputer.Power.getBatteryLevel();
  int batteryVoltage = M5Cardputer.Power.getBatteryVoltage();
  */

  delay(1500);
}