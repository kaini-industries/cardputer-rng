#include <M5Cardputer.h>
#include <Crypto.h>
#include <SHA256.h>

m5::imu_data_t imuData;

String entropy = "";

union FloatBytes {
  float f_val;
  byte b_array[sizeof(float)]; // sizeof(float) is typically 4 bytes // ??
};

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  // randomSeed(analogRead(0));
  M5Cardputer.Display.setFont(&fonts::FreeMonoBold9pt7b);
  M5Cardputer.Display.setCursor(0, 0);

  FloatBytes floatData;
}

void loop() {
  M5Cardputer.update();

  //
  int number = random(0, 9);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print(String(number));
  //

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  SHA256 pulesHash = SHA256();
  String aHash = "";

  FloatBytes floatData;

  String floatTest = "";
  
  FloatBytes floatData; // TEST
  byte floatDataArr[] = {}; // TEST

  FloatBytes accelX;
  // uint8_t accelXArr[] = {};
  byte accelXArr[] = {};
  FloatBytes gyroX;
  // uint8_t gyroXArr[] = {};
  byte gyroXArr[] = {};
  FloatBytes accelY;
  // uint8_t accelYArr[] = {};
  byte accelYArr[] = {};
  FloatBytes gyroY;
  // uint8_t gyroYArr[] = {};
  byte gyroYArr[] = {};
  FloatBytes accelZ;
  // uint8_t accelZArr[] = {};
  byte accelZArr[] = {};
  FloatBytes gyroZ;
  // uint8_t gyroZArr[] = {};
  byte gyroZArr[] = {};

  floatData.f_val = imuData.accel.x; // TEST

  accelX.f_val = imuData.accel.x;
  gyroX.f_val = imuData.gyro.x;
  accelY.f_val = imuData.accel.y;
  gyroY.f_val = imuData.gyro.y;
  accelZ.f_val = imuData.accel.z;
  gyroZ.f_val = imuData.gyro.z;

  M5Cardputer.Display.setCursor(0, 0); // ??

  // byte myArray[] = {0x10, 0x2A, 0xFF, 0x05}; // Array size determined by initializer list
  for (int i = 0; i < sizeof(floatData.b_array); i++) {
    String byteValue = "";

    floatTest += "0x";
    byteValue += "0x";
    if (floatData.b_array[i] < 0x10) {
      floatTest += "0";
      byteValue += "0";
    } 
    // Serial.print(floatData.b_array[i], HEX); // ??
    floatTest += String(floatData.b_array[i]);
    byteValue += String(floatData.b_array[i]);

    // floatDataArr[i] = (byte) byteValue; // FIX THIS
  }

  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.print(String(floatTest));

  /*
  uint8_t data[3] = {0x16, 0xF4, 0x00}; // Example: 5876 = 0x16F4, fill as needed
  pulesHash.update(data, sizeof(data));
  uint8_t hash[32];
  pulesHash.finalize(hash, sizeof(hash));
  for (int i = 0; i < sizeof(hash); i++) {
    char buf[3];
    sprintf(buf, "%02x", hash[i]);
    aHash += buf;
  }
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.print(String(aHash));
  */

  /*
  bool isCharging = M5Cardputer.Power.isCharging();
  int batteryLevel = M5Cardputer.Power.getBatteryLevel();
  int batteryVoltage = M5Cardputer.Power.getBatteryVoltage();
  */

  /*
  int number = random(0, 9);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print(String(number));
  */

  /*
  // String imuPulse = imuData.accel.x + imuData.gyro.x + imuData.accel.y + imuData.gyro.y + imuData.accel.z + imuData.gyro.z;
  // String imuHash
  // entropy += imuData.accel.x + imuData.gyro.x + imuData.accel.y + imuData.gyro.y + imuData.accel.z + imuData.gyro.z;
  int entropyLength = entropy.length();
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.print(String(entropyLength));
  */

  /*
  M5Cardputer.Display.setCursor(120, 55);
  M5Cardputer.Display.printf("%3d %%", batteryLevel);
  M5Cardputer.Display.setCursor(120, 72);
  M5Cardputer.Display.printf("%4d mV", batteryVoltage);
  */

  delay(1000);
}

int handleSingleImuRead(int read) {
  int result;

  result = read;

  if (read < 0) {
    result = read * -1;
  }

  return result;
}

int handleGroupedImuRead(int read) {
  int result;

  result = read;

  if (read < 0) {
    result = read * -1;
  }

  return result;
}