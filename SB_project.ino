/*
   File name    : Smart-Nose-huzzah32.ino
   Author       : Sabari Kannan M. and Mouliha Sree S.V.
   Created on   : 04/06/2021
   Last edit    : 30/09/2021

   Connections:
   Stack the Adafruit FeatherWing OLED over the Adafruit HUZZAH32

   BME688 and VL53L0X   ->  Adafruit HUZZAH32
                  SCL   ->  SCL
                  SDA   ->  SDA
                  3V3   ->  3V3
                  GND   ->  GND
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_VL53L0X.h>

#include "bsec.h"   //BME688 
#include <time.h>

//-------------OLED-------------------------------------------------------------------//
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define mqtt_server "xxx"
#define mqtt_port xxx
#define ssid "xxx"
#define password "xxx"

#define clientID "smart_bin_1"
#define pubTopic "ubilab/smartBin"
#define highBinLevelmm 320

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

//------------------------------------------------------------------------------------//

//------------BME688------------------------------------------------------------------//
// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);

// Create an object of the class Bsec
Bsec iaqSensor;

String output;
float raw_T = 0.0; //raw temperature [°C]
int P = 0; //pressure [hPa]
float raw_hum = 0.0; //raw humidity [%]
double gas_R = 0.0; //gas resistance [Ohm]
double IAQ = 0.0; //index of air quality
int IAQ_accuracy = 0; //index of air quality accuracy
float T = 0.0; //temperature [°C]
float hum = 0.0; //humidity [%]
//------------------------------------------------------------------------------------//


//-----------------TOF sensor--------------------------------------------------------//
Adafruit_VL53L0X lox = Adafruit_VL53L0X();//TOF sensor

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char mqttMessage[300] = {0};

int binLevel = 0;
int VL53L0X_measurement = 0;
//-----------------------------------------------------------------------------------//

//----------Time--------------------------------------------------------------------//
const char* NTP_SERVER = "ch.pool.ntp.org";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)

tm timeinfo;
time_t now;
long unsigned lastNTPtime;
unsigned long lastEntryTime;

unsigned char date;
unsigned char month;
unsigned char year;
unsigned char hour;
unsigned char minute;
unsigned char second;

int lastMin = 0;
int currentMin = 0;
//----------------------------------------------------------------------------------//

void setup() {
  Serial.begin(115200);
  // wait until serial port opens for native USB devices
  while (! Serial)
  {
    delay(1);
  }

  if (!lox.begin())
  {
    Serial.println(F("Failed to boot VL53L0X"));
    while (1);
  }
  //------------------------------------------------------------------------------//

  //--------BME688 setup------------------------------------------------//
  Wire.begin();

  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  // Print the header
  output = "Timestamp [ms], raw temperature [°C], pressure [hPa], raw relative humidity [%], gas [Ohm], IAQ, IAQ accuracy, temperature [°C], relative humidity [%], Static IAQ, CO2 equivalent, breath VOC equivalent";
  Serial.println(output);
  //--------------------------------------------------------------------//

  //--------------Wi-Fi-----------------------------------------------//
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  //client.setCallback(callback);
  //--------------------------------------------------------------------//

  NTP_server_init();
  getTimeReducedTraffic(3600);
  lastMin = minute;
  currentMin = minute;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  //----------BME688 get sensor data-------------------------------//

  //wait for new sensor data
  while (!iaqSensor.run()) {
    checkIaqSensorStatus(); //check whether sensor is available
    getTimeReducedTraffic(3600); //update time
    currentMin = minute; //update current time
    delay(250);
  }

  raw_T = iaqSensor.rawTemperature;
  P = iaqSensor.pressure;
  raw_hum = iaqSensor.rawHumidity;
  gas_R = iaqSensor.gasResistance;
  IAQ = iaqSensor.iaq;
  IAQ_accuracy = iaqSensor.iaqAccuracy;
  T = iaqSensor.temperature;
  hum = iaqSensor.humidity;
  Serial.println(String(raw_T) + " | " +
                 String(P) + " | " +
                 String(raw_hum) + " | " +
                 String(gas_R) + " | " +
                 String(IAQ) + " | " +
                 String(IAQ_accuracy) + " | " +
                 String(T) + " | " +
                 String(hum));
  Serial.println("sensor data read");

  //---------------------------------------------------------------//

  //-----------------------Bin level measurement-------------------------//
  VL53L0X_RangingMeasurementData_t measure;

  Serial.print("Reading a TOF measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4)
  {
    VL53L0X_measurement = measure.RangeMilliMeter;
    binLevel = map(VL53L0X_measurement, 0, highBinLevelmm, 0, 100);
    binLevel = 100 - binLevel;
    binLevel = (binLevel / 10) * 10;

    Serial.print("Distance (mm): ");
    Serial.println(VL53L0X_measurement);
  }
  else
  {
    Serial.println(" out of range ");
  }
  //---------------------------------------------------------------//

  //---------------update OLED diaplay-----------------------------//
  display.clearDisplay(); // Clear the buffer
  display.setCursor(0, 0);  //set cursor

  display.print("IAQ:");
  display.println(IAQ);
  display.display(); // actually display all of the above

  display.print("Level:");
  display.print(binLevel);
  display.println("%");
  display.display(); // actually display all of the above

  delay(10);
  yield();
  display.display();
  //--------------------------------------------------------------//

  if ((currentMin - lastMin == 1) || (currentMin - lastMin == -59)) {
    lastMin = currentMin;

    getTimeReducedTraffic(3600); //get time

    //------------JSON-------------------------------------------//

    StaticJsonDocument<300> Document;

    Document["bin_ID"] = clientID;
    Document["date_time"] = "20" + String(year) + "-" +
                            String(month) + "-" +
                            String(date) + " " +

                            String(hour) + ":" +
                            String(minute); //eg. 2021-1-31 5:37

    Document["raw_T"] = String(iaqSensor.rawTemperature);
    Document["P"] = String(iaqSensor.pressure);
    Document["raw_hum"] = String(iaqSensor.rawHumidity);
    Document["gas_R"] = String(iaqSensor.gasResistance);
    Document["IAQ"] = (int)(iaqSensor.iaq * 100);
    Document["IAQ_accuracy"] = String(iaqSensor.iaqAccuracy);
    Document["T"] = String(iaqSensor.temperature);
    Document["hum"] = String(iaqSensor.humidity);

    Document["VL53L0X_bin_level"] = binLevel;

    serializeJson(Document, mqttMessage);
    Serial.println(mqttMessage);
    //----------------------------------------------------------//

    client.publish(pubTopic, mqttMessage); //publish message
  }
}

void setup_wifi() {
  initOLED();
  //--------------display status-----------------------------------------//
  display.clearDisplay(); // Clear the buffer
  display.setCursor(0, 0);  //set cursor
  display.println("Smart-bin");
  display.display(); // actually display all of the above
  delay(3000);

  display.clearDisplay(); // Clear the buffer
  display.setCursor(0, 0);  //set cursor
  display.println("Wi-Fi");
  display.println("connecting");
  display.display(); // actually display all of the above
  //-------------------------------------------------------------------//

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  display.clearDisplay(); // Clear the buffer
  display.setCursor(0, 0);  //set cursor
  display.println("Wi-Fi");
  display.println("connected");
  display.display(); // actually display all of the above
  delay(3000);
}

void initOLED() {
  //OLED Address 0x3C for 128x32
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // Clear the buffer.
  display.clearDisplay();
  display.display();
  display.setTextSize(2);   //text size
  display.setTextColor(SSD1306_WHITE);  //text color
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID)) {
      Serial.println("connected");
      // Subscribe
      //client.subscribe(subTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


//--------------------BME688 helper functions----------------------------------------------------------//
// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}
//-----------------------------------------------------------------------------------------------------------//

//----------------------Time functions----------------------------------------------------------------------//
void NTP_server_init() {
  configTime(0, 0, NTP_SERVER);
  // See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
  setenv("TZ", TZ_INFO, 1);

  if (getNTPtime(10)) {  // wait up to 10sec to sync
  } else {
    Serial.println("Time not set");
    ESP.restart();
  }
  lastNTPtime = time(&now);
  lastEntryTime = millis();
}
bool getNTPtime(int sec) {

  {
    uint32_t start = millis();
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      Serial.print(".");
      delay(10);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
    Serial.print("now ");  Serial.println(now);
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
    Serial.println(time_output);
    Serial.println();
  }
  return true;
}

void getTimeReducedTraffic(int sec) {
  tm *ptm;
  if ((millis() - lastEntryTime) < (1000 * sec)) {
    now = lastNTPtime + (int)(millis() - lastEntryTime) / 1000;
  } else {
    lastEntryTime = millis();
    lastNTPtime = time(&now);
    now = lastNTPtime;
    Serial.println("Get NTP time");
  }
  ptm = localtime(&now);
  timeinfo = *ptm;

  date = timeinfo.tm_mday;
  month = timeinfo.tm_mon + 1;
  year = timeinfo.tm_year - 100;
  hour = timeinfo.tm_hour;
  minute = timeinfo.tm_min;
  second = timeinfo.tm_sec;
}
//----------------------------------------------------------------------------------------------------------//
