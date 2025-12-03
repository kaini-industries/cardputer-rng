#include <M5Cardputer.h>
#include <Crypto.h>
#include <SHA256.h>

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
String floatTest = ""; // ??

// String allByteValuesStr = "";
// FloatBytes floatData; // TEST VAR // NEEDS RESET ??

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  // INITS
  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  // randomSeed(analogRead(0)); // hm...
  M5Cardputer.Display.setFont(&fonts::FreeMonoBold9pt7b);
  M5Cardputer.Display.setCursor(0, 0);

  // RESETS // WHICH ORDER FOR THESE // DO THESE NEED TO BE HERE
  String allByteValuesStr = "";
  // hashMachine.reset();
  // hashMachine.clear();
}

void loop() {
  // SHA256 object
  SHA256 hashMachine = SHA256();

  M5Cardputer.update();

  std::__cxx11::string allByteValuesStr = "";

  // WORKS
  // int number = random(0, 9);
  // M5Cardputer.Display.setCursor(0, 0);
  // M5Cardputer.Display.print(String(number)+" PULSE");

  M5.Imu.update();
  imuData = M5.Imu.getImuData();

  String pulseHashStr = "";
  // String floatTest = ""; // ??
  std::string floatTest = ""; // ??

  FloatBytes floatData; // TEST VAR // NEEDS RESET ??
  byte floatDataArr[] = {}; // NEEDS BETTER RESET ??

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

  floatData.f_val = imuData.gyro.x; // TEST
  // M5Cardputer.Display.setCursor(0, 20);
  // M5Cardputer.Display.print(allByteValuesStr);

  accelX.f_val = imuData.accel.x;
  gyroX.f_val = imuData.gyro.x;
  accelY.f_val = imuData.accel.y;
  gyroY.f_val = imuData.gyro.y;
  accelZ.f_val = imuData.accel.z;
  gyroZ.f_val = imuData.gyro.z;

  // byte myArray[] = {0x10, 0x2A, 0xFF, 0x05}; // TEST ARRAY OF BYTES
  for (int i = 0; i < sizeof(floatData.b_array); i++) {
    // M5Cardputer.Display.setCursor(0, 20);
    // M5Cardputer.Display.print(String(floatData.b_array[i]));
    // delay(1000);

    // STRING TO BYTE DOES NOT WORK cool
    String byteValue = "";

    floatTest += "0x";
    byteValue += "0x";
    allByteValuesStr += "0x";

    if (floatData.b_array[i] < 0x10) {
      floatTest += "0";
      byteValue += "0";
      allByteValuesStr += "0";
    } 
    // floatTest += String(floatData.b_array[i]);
    byteValue += String(floatData.b_array[i]);
    // allByteValuesStr += String(floatData.b_array[i]);
    // std::__cxx11::string cFloatStr = String(floatData.b_array[i]);
    // allByteValuesStr = allByteValuesStr + String(floatData.b_array[i]);

    // floatDataArr[i] = (byte) byteValue; // STRING TO BYTE DOES NOT WORK

    floatDataArr[i] = (byte) floatData.b_array[i]; // FIX THIS OR CHECK FOR ZERO BYTES // ??
  }

  // 
  // M5Cardputer.Display.setCursor(0, 20);
  // M5Cardputer.Display.print(allByteValuesStr);

  // HASHing floatTest or somethimg
  std::string testString = allByteValuesStr;
  std::string myStdString = testString;
  
  // const char* cStyleString = myStdString.c_str(); 
  // M5Cardputer.Display.setCursor(0, 20);
  // M5Cardputer.Display.print(String(cStyleString));

  //+ const char* cStyleString = myStdString.c_str(); 
  // std::string myStdString = String(allByteValuesStr);
  // const char* cStyleString = myStdString.c_str(); 

  uint8_t hash[hashMachine.HASH_SIZE];
  // hashMachine.update((const uint8_t*) cStyleString, strlen(cStyleString)); // Feed the data
  hashMachine.finalize(hash, sizeof(hash)); // Finalize and get the hash
  for (size_t i = 0; i < hashMachine.HASH_SIZE; i++) {
    // M5Cardputer.Display.print(hash[i], HEX); // Print each byte in hexadecimal
    // pulseHashStr += hash[i];
    // entropy += hash[i];
  }

  //+ M5Cardputer.Display.setCursor(0, 20);
  //+ M5Cardputer.Display.print(pulseHashStr);

  /*
  hashMachine.update(floatDataArr, sizeof(floatDataArr));
  uint8_t hash[32]; // THE HASH TO BE CREATED // SIZE ??
  hashMachine.finalize(hash, sizeof(hash)); // hash or not

  for (int i = 0; i < sizeof(hash); i++) {
    char buf[3];
    sprintf(buf, "%02x", hash[i]); // ??
    // pulseHashStr += buf;

    M5Cardputer.Display.setCursor(0, 20);
    // M5Cardputer.Display.print(hash);
    M5Cardputer.Display.print((hash[i]));
  }
  */

  // entropy += String(hash); // NOT HERE

  // RESETS // WHICH ORDER FOR THESE
  hashMachine.reset();
  hashMachine.clear();
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

  /*
  // String imuPulse = imuData.accel.x + imuData.gyro.x + imuData.accel.y + imuData.gyro.y + imuData.accel.z + imuData.gyro.z;
  // String imuHash
  // entropy += imuData.accel.x + imuData.gyro.x + imuData.accel.y + imuData.gyro.y + imuData.accel.z + imuData.gyro.z;
  int entropyLength = entropy.length();
  M5Cardputer.Display.setCursor(0, 20);
  M5Cardputer.Display.print(String(entropyLength));
  */

  delay(1200);
}