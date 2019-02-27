/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 * 
 * Based on the Code from Neil Kolban: https://github.com/nkolban/esp32-snippets/blob/master/hardware/infra_red/receiver/rmt_receiver.c
 * Based on the Code from pcbreflux: https://github.com/pcbreflux/espressif/blob/master/esp32/arduino/sketchbook/ESP32_IR_Remote/ir_demo/ir_demo.ino
 * Based on the Code from Xplorer001: https://github.com/ExploreEmbedded/ESP32_RMT
 */
/* This is a simple code to receive and then save IR received by an IR sensor connected to the ESP32 and then resend it back out via an IR LED  
 * Reason that I created this I could not find any complete examples of IR send and Received in the RAW format, I am hoping this code can then be used
 * to build on the existing IR libraries out there.
 *
 * 20/12/1018
 * Air con Remote too long. Cannot use irrecv in RMT driver ( Error RMT BUFFER FULL)
 * Use Interrupt to detect change on GPIO PIN and get different of time in micros 
 * for getting time period in micro second.
 *
 */
#define DEBUG   // Debug to print

#include "DebugPrint.h"

 
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EEPROM.h"

#include "BLEDevice.h"
#include "time.h"


#include "AirCoolIR.h"
#include "AirAlgorithm.h"

// Watch Dog 
#define WATCHDOG_TIMEOUT  10

static uint32_t watchdog_counter = 0;
static bool     watchdog_active  = false;

// Define command to control AIR Conditioner


#define PRE_SCALE       40000      // 80 Mhz/Prescale  = time to tick timer Ex 40000/80 Mhz = 0.5 msec   range from  2 to 65536 
#define TIME_TO_TICK    2000     // 2000 x0.5   = 1000 ms = 1 sec   
#define GMT  7


/*   
 aircoolFlag extract by get fix string position
 Format w=1,a=1,t=1,s=1,o=0,m=0
 w = check weather flag
 a = check activities flag
 t = check duration time flag
 s = check sleep flag
 o =  On/Off flag
 m = Motion detect flag
 set = Setting temperature
*/
#define  WEATHER_FLAG    2
#define  ACTIVITIES_FLAG 6
#define  TIME_FLAG      10
#define  SLEEP_FLAG     14
#define  ONOFF_FLAG     18
#define  MOTION_FLAG    22
#define  SET_TEMP       28

// EEPROM CONFIG SAVE
#define EEPROM_SIZE     16
#define POWER_UP    0      // First time power up or reset after power up
#define SET_TMP     1
#define TARGET_TMP  2
#define DURATION_TIME 3   // Duration high byte
#define DURATION_LOW 4    // Duration low byte




const int STATUS_PIN  = 26;   //Status LED Active
const int RESET_PIN   = 19;   //Status LED Active




// WiFi Config
const char* ssid = "SSID";
const char* password =  "Password";
const String location = "Location for openweather";
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+location+"&units=metric&APPID=";
const String key = "OpenWeather API key you need to register to get key";    //Openweather API


// Time from NTP
const char* ntpServer = "th.pool.ntp.org";
const long  gmtOffset_sec = GMT*3600;
const int   daylightOffset_sec = GMT*3600;

// Allocate the JSON document
//
// Inside the brackets, 200 is the size of the memory pool in bytes.
// Don't forget to change this value to match your JSON document.
// Use arduinojson.org/assistant to compute the capacity.
const size_t capacity = JSON_ARRAY_SIZE(3) + 2*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(12) + 480;
StaticJsonDocument<capacity> jsonBuffer;

// StaticJsonDocument<N> allocates memory on the stack, it can be
// replaced by DynamicJsonDocument which allocates in the heap.
//
// DynamicJsonDocument jsonBuffer(capacity);


// Global variables
struct AirCoolConfig  airCoolFlag;
struct weatherStatus  weatherNow; 
struct CurrentEnv envstate;


 


// The remote service we wish to connect to. This number may be need to change depend on UUID 
// at NXP IOT
static BLEUUID  serviceUUID("409045d7-27f1-45ca-b87d-26cbaeb4d003");   // UUID of service 
static BLEUUID advertiseUUID("409045d7-27f1-45ca-b87d-26cbaeb4d003");  // UUID when device scan 
static BLEUUID    configUUID("409045d7-27f1-45ca-b87d-26cbaeb4d005");
static BLEUUID      tempUUID("409045d7-27f1-45ca-b87d-26cbaeb4d006");


static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true;
static BLEClient *pClient;
static BLERemoteCharacteristic* pTempCharacteristic;
static BLERemoteCharacteristic* pConfigCharacteristic;
static BLEAdvertisedDevice* myDevice;


volatile int interruptCounter;

unsigned int timefromInterrupt;  // time set for read config every seconds
unsigned int durationChangeTemp; // duration before change setting

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


void watchdog_poll(void) {
  if (watchdog_active) {
    watchdog_counter = 0; // reset watchdog
  }
}

void hard_reset(void){
  
       digitalWrite(RESET_PIN,HIGH); 
}
 
// Interrupt handle for timer
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  timefromInterrupt++;   // Tick for every seconds
  if (watchdog_counter++ >= 2 * WATCHDOG_TIMEOUT) {
//    ESP.restart();
      hard_reset();
  }
  portEXIT_CRITICAL_ISR(&timerMux);
 
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    debugPrintln("Failed to obtain time");
    return;
  }
  debugPrintMulti(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  envstate.timeofday = checkLocalTime();
  debugPrint ("TIME OF DAY =  ");
  debugPrintln(envstate.timeofday);
  
}



// Print the data extracted from the JSON
void printweatherNow( struct weatherStatus * weather) 
{

  debugPrint("Temp = ");
  debugPrint(weather->currentTemp);
  debugPrint("\n");
  debugPrint("Humidity = ");
  debugPrint(weather->humidity);
  debugPrint("\n");
  debugPrint("Weather = ");
  debugPrint(weather->description); 
  debugPrint("\n");
}

void getWeather(struct weatherStatus* weather)
{
     HTTPClient http;
    
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        debugPrintln(httpCode);
        debugPrintln(payload);
         // Deserialize the JSON document
      
        DeserializationError error = deserializeJson(jsonBuffer, payload);
         // Test if parsing succeeds.
        if (error) {
            debugPrint(F("deserializeJson() failed: "));
            debugPrintln(error.c_str());
            return;
        }
        // Get the root object in the document
       JsonObject root = jsonBuffer.as<JsonObject>();
 
        // Fetch values.
        // Most of the time, you can rely on the implicit casts.
        // In other case, you can do root["time"].as<long>();
        strcpy (weather->description,root["weather"][0]["main"]);
        weather->currentTemp =  root["main"]["temp"];
        weather->maxTemp =  root["main"]["temp_min"];
        weather->minTemp =  root["main"]["temp_max"];
        weather->humidity =  root["main"]["humidity"];
     }
    else {
        debugPrintln("Error on HTTP request");
    }
    http.end(); //Free the resources
}


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,size_t length,bool isNotify) {
    debugPrint("Notify callback for characteristic ");
    debugPrint(pBLERemoteCharacteristic->getUUID().toString().c_str());
    debugPrint(" of data length ");
    debugPrintln(length);
    debugPrint("data: ");
    debugPrintln((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    debugPrintln("BLE Disconnect");
    digitalWrite(STATUS_PIN,HIGH);
//    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
     delay(100);
//     ESP.restart();  
     hard_reset();
  }
};

BLERemoteCharacteristic* getRemoteCharacteristics(BLERemoteService* service ,BLEUUID uuid)
{
    BLERemoteCharacteristic* characteristic;
   // Obtain a reference to the characteristic in the service of the remote BLE server.
    characteristic = service->getCharacteristic(uuid);
    if ( characteristic == nullptr) {
      debugPrint("Failed to find our characteristic UUID: ");
      debugPrintln(uuid.toString().c_str());
      return nullptr;
    }
    debugPrintln(" - Found characteristic");
    // Read the value of the characteristic.
    if(characteristic->canRead()) {
      std::string value = characteristic->readValue();
      debugPrint("The characteristic value was: ");
      debugPrintln(value.c_str());
    }

    if(characteristic->canNotify())
    {
      debugPrintln ("Register Notify Callback");
      characteristic->registerForNotify(notifyCallback);
    }
   return characteristic;
}

bool connectToServer() {
    debugPrint("Forming a connection to ");
    debugPrintln(myDevice->getAddress().toString().c_str());
    
    pClient  = BLEDevice::createClient();
    debugPrintln(" - Created client");
    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remote BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    debugPrintln(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      debugPrint("Failed to find our service UUID: ");
      debugPrintln(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    debugPrintln(" - Found our service");

   pTempCharacteristic = getRemoteCharacteristics(pRemoteService, tempUUID);
   if (pTempCharacteristic == nullptr)
   {
     pClient->disconnect();
     return false;
   }

  pConfigCharacteristic = getRemoteCharacteristics(pRemoteService, configUUID);
   if (pConfigCharacteristic == nullptr)
   {
     pClient->disconnect();
     return false;
   }
  
    connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    debugPrint("BLE Advertised Device found: ");
    debugPrintln(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(advertiseUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks



void  wifiGetWeather()
{
//  btStop();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    debugPrintln("Connecting to WiFi..");
  }
 
  debugPrint("Connected to the WiFi network ");
  debugPrint(ssid);
  debugPrint ("  at : ");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  getWeather(&weatherNow);
  printweatherNow (&weatherNow);
  envstate.weather = checkWeather(&weatherNow);
  WiFi.mode(WIFI_OFF);

}

/* Force a client to disconnect
 */


void startBLE()
{
  debugPrintln("Starting BLE service...");
  BLEDevice::init("");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

}

void sendBLE(String str,BLERemoteCharacteristic* pRemote )
{
//   String newValue = "Time since boot: " + String(millis()/1000);
   debugPrintln("Setting new characteristic value to \"" + str + "\"");   
// Set the characteristic's value to be the array of bytes that is actually a string.
   pRemote->writeValue(str.c_str(), str.length());
}

/*
 * Get temperature outside
 *   User global variable  struct AirCoolConfig  airCoolFlag;
 */
void setRoomTemp (String temp)
{
  airCoolFlag.tempRoom = temp.toFloat();
  debugPrint ( "Room temperature =   ");
  debugPrintln ( airCoolFlag.tempRoom);
}


/*
 * Send IR command 
 */
 void sendIRCommand (void)
 {
     static int turnon = 0;
     
      if (airCoolFlag.onoffFlag && !turnon) // Mean change from off to on  Turn on Air Con
      {
          debugPrintln ("Turn ON AIRCON");
          turnon = 1;
          sendCommand(airCoolFlag.setTmp - STARTTEMP);
          airCoolFlag.tempChange = false;
      }  
     if (!airCoolFlag.onoffFlag && turnon) // Mean change from off to on  Turn on Air Con      
      {
          debugPrintln ("Turn Off AIRCON");
          turnon = 0; 
          sendCommand(TURNOFF);             
      }

// Temperature setting change   
      if ( airCoolFlag.tempChange)
      {
         debugPrintln ( "Temperature Setting Changed ");
         // Setting IR command 
         if (airCoolFlag.onoffFlag) // If off should not send command 
         {
              sendCommand(airCoolFlag.setTmp - STARTTEMP);
         }
      }
 }


/* 
 *  Get All the setting from NXP IOT
 *  
 */

void getConfig(String str)
{  
     static int  prevtemp =  airCoolFlag.setTmp;
     if (str.charAt(WEATHER_FLAG) == '1')
     {
          debugPrint ("WeatherFlag Enable, ");
          airCoolFlag.weatherFlag = true;
     }
     else
     {
          debugPrint ("WeatherFlag Disable, ");
          airCoolFlag.weatherFlag = false;
     }
     if (str.charAt(ACTIVITIES_FLAG) == '1')
     {
          debugPrint ("ActivitiesFlag Enable, ");
          airCoolFlag.activitiesFlag = true;
     }
     else
     {
          debugPrint ("ActivitiesFlag Disable, ");
          airCoolFlag.activitiesFlag = false;
     }
     if (str.charAt(TIME_FLAG) == '1')
     {
          debugPrint("timeFlag Enable, ");
          airCoolFlag.timeFlag = true;
     }
     else
     {
          debugPrint("timeFlag Disable, ");
          airCoolFlag.timeFlag = false;
     }
     if (str.charAt(SLEEP_FLAG) == '1')
     {
          debugPrint("sleepFlag Enable, ");
          airCoolFlag.sleepFlag = true;
     }
     else
     {
          debugPrint ("sleepFlag Disable, ");
          airCoolFlag.sleepFlag = false;
     }
     if (str.charAt(MOTION_FLAG) == '1')
     {
          debugPrintln ("Motion Detected");
          airCoolFlag.motionFlag = true;
     }
     else
     {
          debugPrintln ("Motion Not detect");
          airCoolFlag.motionFlag = false;
     }
     debugPrint("Timer =  ");
     debugPrint(timefromInterrupt);
    int act = checkActivities (airCoolFlag.motionFlag, &timefromInterrupt);
    if (act < 0)
    {
       debugPrint("    Measuring and current activity state = ");
       debugPrintln (envstate.activities);
 
    }
    else 
    {
        envstate.activities = act;
        debugPrint("    Finished measure and activitiy state = ");
        debugPrintln (envstate.activities);
 
    }
     
     if (str.charAt(ONOFF_FLAG) == '1')
             airCoolFlag.onoffFlag = true;
     else
             airCoolFlag.onoffFlag = false;

// Get Setting temperature 
      airCoolFlag.setTmp =(str.substring(SET_TEMP)).toInt();
      debugPrint ("Set temp :  ");
      debugPrintln (airCoolFlag.setTmp);
      if ( prevtemp != airCoolFlag.setTmp)
         airCoolFlag.tempChange = true;
      else
         airCoolFlag.tempChange = false;
         prevtemp = airCoolFlag.setTmp;
}



void setup() {

 #ifdef DEBUG 
    Serial.begin(115200);
 #endif 
  pinMode (STATUS_PIN,OUTPUT);
  pinMode (RESET_PIN,OUTPUT);
  debugPrintln("Starting Air Cool application...");
  wifiGetWeather();

// Set timer interrupt  
  timer = timerBegin(0, PRE_SCALE, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, TIME_TO_TICK, true);
  timerAlarmEnable(timer);
  interruptCounter = 0;
  timefromInterrupt = 0;
  durationChangeTemp = 0;
  watchdog_active = true;
  
  startBLE();
  
} // End of setup.


// This is the Arduino main loop function.
void loop() {
// If the flag "doConnect" is true then we have scanned for and found the desired
// BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
// connected we set the connected flag to be true.
unsigned int len = 0;
static unsigned int duration;
static bool toggle;
    watchdog_poll();
    if (doConnect == true) {
          if (connectToServer()) {
              debugPrintln("We are now connected to the BLE Server.");
          } else {
              debugPrintln("We have failed to connect to the server; there is nothin more we will do.");
          }
          doConnect = false;
    }
    if (interruptCounter > 0) {
       timerAlarmDisable(timer);
       portENTER_CRITICAL(&timerMux);
       interruptCounter--;
       portEXIT_CRITICAL(&timerMux);  
       printLocalTime(); 
       if (toggle)
       {
           digitalWrite(STATUS_PIN,HIGH);
            toggle = false;
       }
       else
       {
           digitalWrite(STATUS_PIN,LOW);
           toggle = true;          
       }
       
        if (connected) {
            debugWrite(27);     //Print "esc"
            debugPrint("[2J");  // Clear Screen
            debugWrite(27);     //Print "esc"
            debugPrint("[H");  // Clear Screen
            
            std::string temp = pTempCharacteristic->readValue();
            setRoomTemp(temp.c_str());
            getConfig(pConfigCharacteristic->readValue().c_str());
            sendIRCommand();
  // Check temp different outside and room
            envstate.diffweather = checkWeatherTemp (&weatherNow, &airCoolFlag);
            debugPrint ("Outside temp - setting temp  =  ");
            debugPrintln (envstate.diffweather);           
            envstate.diffroom = checkRoomTemp (&airCoolFlag);
            debugPrint ("Room temp - setting temp  =  ");
            debugPrintln (envstate.diffroom);
  // Get Coefficient
           getCoeff(&envstate,&airCoolFlag);    
           duration = airCoolFlag.duration*60;   // Min x 60 Change if environment change but durationChangeTemp still counting
           if ( durationChangeTemp >= duration )
           {
                durationChangeTemp = 0;   // Min x 60 
                // Send Command to Air Condition
                int index = 0;
                int diff   = airCoolFlag.targetTmp - airCoolFlag.setTmp;
                if ((diff) != 0)
                {
                    index  += (airCoolFlag.setTmp +diff)- STARTTEMP;
                   
                    if ( airCoolFlag.fanspeed  > 1 )
                      index +=  HIGHFAN;
                    if (airCoolFlag.onoffFlag) // Mean change from on to off  Turn off Air Con   
                    { 
                      debugPrint ("Set temperature to   ");
                      debugPrintln (STARTTEMP + index);
                      sendCommand(index);
                    }
                }
                    
 // Get new weather 
                wifiGetWeather(); 
           }
           else
           {
                durationChangeTemp++;
                debugPrint ("Duration left :   ");
                debugPrintln(duration - durationChangeTemp);                 
           }
             
        }else if(doScan){
            debugPrintln ("Rescan BLE device");
            BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
        }        
        timerAlarmEnable(timer);          
    }
//Time interval check weather

  
} // End of loop
