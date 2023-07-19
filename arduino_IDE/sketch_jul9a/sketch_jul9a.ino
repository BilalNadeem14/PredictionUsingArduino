
/*
  HTS221 - Read Sensors

  This example reads data from the on-board HTS221 sensor of the
  Nano 33 BLE Sense and prints the temperature and humidity sensor
  values to the Serial Monitor once a second.

  The circuit:
  - Arduino Nano 33 BLE Sense

  This example code is in the public domain.
*/
#include <Arduino_HTS221.h>
#include <Arduino_LSM9DS1.h>
#include <ArduinoBLE.h>
#include <ArduinoJson.h>


const size_t CAPACITY = JSON_OBJECT_SIZE(1);
StaticJsonDocument<CAPACITY> doc;
float old_temp = 0;
float old_hum = 0;
float old_acc = 0;
float totalAcceleration = 0;
float temperature = 0;
float humidity = 0;

BLEService jsonService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEStringCharacteristic jsonCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite | BLENotify, 20);

void setup() {

  Serial.begin(9600);
  while (!Serial)
    ;

  if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1)
      ;
  }
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1)
      ;
  }


  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1)
      ;
  }

  BLE.setLocalName("ArduinoJSON");
  BLE.setAdvertisedService(jsonService);

  jsonService.addCharacteristic(jsonCharacteristic);
  BLE.addService(jsonService);

  BLE.advertise();



  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.print("Magnetometer sample rate = ");
  Serial.print(IMU.magneticFieldSampleRate());
  Serial.println(" Hz");
  Serial.println();
}




void runMotionSensor() {
  float x, y, z;



  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    totalAcceleration = sqrt(sq(x) + sq(y) + sq(z));

    if (abs(totalAcceleration - old_acc) >= 0.1) {
      old_acc = totalAcceleration;
      Serial.print("Total Acceleration: ");
      Serial.print(old_acc);
      Serial.println(" m/s^2");
      printToLocal("A", old_acc);
    }
    if (old_acc > 1.2) {  // Set a threshold for detecting potential seismic activity
      Serial.println("Possible seismic activity detected!");
    }
  }


  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);
    Serial.print("Gyroscope: ");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(z);
    Serial.println(" deg/s");
  }

  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(x, y, z);
    Serial.print("Magnetic Field: ");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.print(z);
    Serial.println(" uT");
  }

  Serial.println();
  delay(1000);

  printToLocal("A", totalAcceleration);
}

void runTempData() {
  temperature = HTS.readTemperature();
  humidity = HTS.readHumidity();

  // check if the range values in temperature are bigger than 0,5 ºC
  // and if the range values in humidity are bigger than 1%
  if (abs(old_temp - temperature) >= 0.5 || abs(old_hum - humidity) >= 1) {
    old_temp = temperature;
    old_hum = humidity;
    // print each of the sensor values
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.print("Humidity    = ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.println();
  }

  // print each of the sensor values
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity    = ");
  Serial.print(humidity);
  Serial.println(" %");

  printToLocal("T", temperature);
  printToLocal("H", humidity);
}

void printToLocal(const char* key, float val) {
  char valString[20];                                   // Buffer to store the converted float value
  snprintf(valString, sizeof(valString), "%.4f", val);  // Convert the float value to a string with 2 decimal places
  String jsonString = String("{") + "\"" + key + "\":\"" + valString + "\"}";
  jsonCharacteristic.setValue(jsonString);
}

void loop() {
  // read all the sensor values
  // runMotionSensor();
  // runTempData();

  // print an empty line
  Serial.println();

  // wait 1 second to print again
  delay(1000);

  // Check for a connected central device
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    // Send data to the central device while it is connected
    while (central.connected()) {
      runTempData();
      runMotionSensor();
      // printToLocal("Acceleration", totalAcceleration);
      // printToLocal("Temperature", temperature);
      // printToLocal("Humidity", humidity);
      delay(1000);
      // old_temp = 0;
      // old_hum = 0;
      // old_acc = 0;
      // totalAcceleration = 0;
      // temperature = 0;
      // humidity = 0;
      // delay(200);
    }

    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
