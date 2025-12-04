#include <M5Cardputer.h>
#include <Crypto.h>
#include <SHA256.h>
// #include <RNG.h>

SHA256 hashMachine = SHA256();

m5::imu_data_t imuData;

struct FloatBytes {
  float f_val;
  byte b_array[sizeof(float)]; // sizeof(float) is typically 4 bytes // ??
};
/*
union FloatBytes {
  float f_val;
  byte b_array[sizeof(float)]; // sizeof(float) is typically 4 bytes // ??
};
*/

String entropy = "";

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  // randomSeed(analogRead(0)); // hm...

  M5Cardputer.Display.setFont(&fonts::FreeMonoBold9pt7b);
  M5Cardputer.Display.setCursor(0, 0);

  hashMachine.clear();
  hashMachine.reset();
}

void loop() {
  // SHA256 object
  SHA256 hashMachine = SHA256();
  hashMachine.clear();
  hashMachine.reset();

  M5Cardputer.update();

  // WORKS
  // int number = random(0, 9);
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(number)+" PULSE");

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  byte floatDataArr[] = {}; // TEST

  FloatBytes accelX;
  accelX.f_val = imuData.accel.x;
  // uint8_t accelXArr[] = {};
  byte accelXArr[] = {};

  FloatBytes gyroX;
  gyroX.f_val = imuData.gyro.x;
  // uint8_t gyroXArr[] = {};
  byte gyroXArr[] = {};

  FloatBytes accelY;
  accelY.f_val = imuData.gyro.x;
  // uint8_t accelYArr[] = {};
  byte accelYArr[] = {};

  FloatBytes gyroY;
  gyroY.f_val = imuData.gyro.y;
  // uint8_t gyroYArr[] = {};
  byte gyroYArr[] = {};

  FloatBytes accelZ;
  accelZ.f_val = imuData.accel.y;
  // uint8_t accelZArr[] = {};
  byte accelZArr[] = {};

  FloatBytes gyroZ;
  gyroZ.f_val = imuData.gyro.z;
  // uint8_t gyroZArr[] = {};
  byte gyroZArr[] = {};

  accelX.f_val = imuData.accel.x;
  gyroX.f_val = imuData.gyro.x;
  accelY.f_val = imuData.accel.y;
  gyroY.f_val = imuData.gyro.y;
  accelZ.f_val = imuData.accel.z;
  gyroZ.f_val = imuData.gyro.z;

  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.print(
    String(accelX.f_val) +
    String(gyroX.f_val) +
    String(accelY.f_val) +
    String(gyroY.f_val) +
    String(accelZ.f_val) +
    String(gyroZ.f_val)
  );

  // byte myArray[] = {0x10, 0x2A, 0xFF, 0x05}; // TEST ARRAY OF BYTES
  for (int i = 0; i < sizeof(gyroX.b_array); i++) {
    // M5Cardputer.Display.setCursor(0, 20);
    // M5Cardputer.Display.print(String(gyroX.b_array[i]));
    // delay(1000);

    String byteValue = ""; // TEST

    byteValue += String(gyroX.b_array[i]);

    floatDataArr[i] = (byte) gyroX.b_array[i]; //
  }

  std::string myStdString = "myStdString"; // TEST
  const char* cStyleString = myStdString.c_str();  // TEST
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.print(String(cStyleString));

  uint8_t hash[hashMachine.HASH_SIZE];
  // uint8_t hash[32]; // ??
  hashMachine.update((const uint8_t*) cStyleString, strlen(cStyleString)); // Feed the data
  hashMachine.finalize(hash, sizeof(hash)); // Finalize and get the hash
  for (size_t i = 0; i < hashMachine.HASH_SIZE; i++) {
    // M5Cardputer.Display.print(hash[i], HEX); // Print each byte in hex
  }

  hashMachine.clear();
  hashMachine.reset();
  // floatDataArr[] = {};

  // int entropyLength = entropy.length();
  // M5Cardputer.Display.setCursor(0, 20);
  // M5Cardputer.Display.print(String(entropyLength));
  // M5Cardputer.Display.setCursor(0, 40);
  // M5Cardputer.Display.print(String(entropy));

  /* // CHARGING DEMO
  bool isCharging = M5Cardputer.Power.isCharging();
  int batteryLevel = M5Cardputer.Power.getBatteryLevel();
  int batteryVoltage = M5Cardputer.Power.getBatteryVoltage();
  */

  delay(1200);
}