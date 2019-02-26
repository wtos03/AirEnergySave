/* 
 *  Define variable to use in Algorirhm
 *  
 */
#define MIN_TEMP 22
#define MAX_TEMP 28

// Activities
#define  SLEEP          0
#define  NON_ACTIVE     1
#define  ACTIVE         2   
#define  STILL_MEASURE -1

// Number of motion detect per second
#define  SLEEP_SETTING     5
#define  ACTIVE_SETTING   20
#define  ACTIVITIES_TIME       60     //60  seconds


// Different of Temperature between Outside Temp and Setting Temperature
#define TOUT_HOT     4     // Different more than this conider hot
#define WEATHER_HOT  0
#define WEATHER_OK   1
#define WEATHER_COOL 2


// Different of Temperature between Room Temp and Setting Temperature
#define TROOM_HOT 3       
#define ROOM_HOT  0
#define ROOM_OK   1
#define ROOM_COOL 2



// Weather 
#define NOT_FOUND     0
#define THUNDERSTORM  0
#define DRIZZLE       0
#define RAIN          0
#define CLOUDS        1
#define CLEAR         2 
#define HAZE          1

// String use to compare from openweathermap  https://openweathermap.org/weather-conditions
#define STR_THUNDERSTORM  "Thunderstorm"
#define STR_DRIZZLE       "Drizzle"
#define STR_RAIN          "Rain"
#define STR_CLOUDS        "Clouds"
#define STR_CLEAR         "Clear"
#define STR_HAZE          "Haze"


// TIME OF DAY
#define AFTERNOON   0
#define MORNING     1
#define NIGHT       2
#define TIME_ERROR  -1



struct weatherStatus {
  char description[10];
  float currentTemp;
  float maxTemp;
  float minTemp;
  float humidity;
};

struct AirCoolConfig {
   bool weatherFlag;
   bool activitiesFlag;
   bool timeFlag;
   bool sleepFlag;
   bool motionFlag;
   bool onoffFlag;
   bool tempChange;
   int  setTmp;
   int  targetTmp;   // Coeffiecient to setting tmp
   int  duration;
   int  fanspeed;
   float tempRoom;
};


struct CurrentEnv {
  int activities;
  int weather;
  int diffweather;
  int diffroom;
  int timeofday;
};

int checkActivities (bool motion, unsigned int *seconds);
int checkWeather (struct weatherStatus*  chk);
int checkWeatherTemp (struct weatherStatus* chk, struct AirCoolConfig* airconfig);
int checkRoomTemp (struct AirCoolConfig* airconfig);
int getCoeff(struct CurrentEnv* envi,struct AirCoolConfig* airconfig);
int checkLocalTime();

       
 

