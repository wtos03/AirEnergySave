/* Algorithm to predefine three characteristics of  Air Condition
 *    - Temperature to change  The higher temperature setting, the more power we can save but too high will get user uncomfortable
 *    - Wind Speed  Auto or High speed depend on how fast you want to reach the desired temperature
 *    - Duration to change temperature  The duration must in interval that human cannot detect temperature change or allow human body to adjust temperature
 */



#include <Arduino.h>   // Need to include this if seperate module   *.ino no need to include this file
#include <time.h>
#include <stdbool.h>
#include <string.h>


#define DEBUG   // Need to put behind <Arduino.h>  to avoid compile error ??

#include "AirAlgorithm.h"
#include "DebugPrint.h"


/* Table for calculate value for temperature setting, Wind speed and Duration to change temp.
 *  It's depend on many variables and factors
 *  
 */
// Temperature setting
int activValue[] = {2,1,0};  // SLEEP, NON-ACTIVE,ACTIVE
int weatherValue[] = { 1,0,-1};
int diffTempOutValue[] = {-1,0,1};
int diffRoomTempValue[] = {-1,0,1};
int dayTimeValue[] = {-1,0,1};

 // Windspeed 
int activWind[]   = {0,0,1};  // SLEEP, NON-ACTIVE,ACTIVE
int weatherWind[] = { 0,0,1};
int diffTempOutWind[] = {1,0,0};
int diffRoomTempWind[] = {1,0,0};
int dayTimeWind[] = {1,0,0};

 // Duration 
 int activDuration[] = {30,20,10};  // SLEEP, NON-ACTIVE,ACTIVE
int weatherDuration[] = {30,20,10};
int diffTempOutDuration[] = {10,20,30};
int diffRoomTempDuration[] = {10,20,30};
int dayTimeDuration[] = {10,20,30};


/*
 * Check activity in duration of time 
 * 
 */

int  checkActivities ( bool motion, unsigned int *seconds)
 { 
    
      static unsigned int activities = 0;
      int measure = STILL_MEASURE;
      if (*seconds < ACTIVITIES_TIME)
      {
          if ( motion)
          {
              activities++;
          }   
      }
      if (*seconds == ACTIVITIES_TIME) // Check for number of motions detect in one minute or more
      {
     //     debugPrint ( " Activities movement =   ");
     //     debugPrint ( activities);
     // Don't rearrange this checking order. The results will incorrect. Need to check NON ACTIVE before SLEEP
          if (activities < ACTIVE_SETTING)
                  measure = NON_ACTIVE;
          if (activities < SLEEP_SETTING)
                  measure = SLEEP;
          if (activities >= ACTIVE_SETTING)
                  measure = ACTIVE;
          activities = 0;
          *seconds  = 0;                             
      }
      return measure;  
 }

int checkRoomTemp (struct AirCoolConfig* airconfig)
{
   int  diff = (int)(airconfig->tempRoom - airconfig->setTmp);
  if (diff  >= TROOM_HOT)
       return ROOM_HOT;
  if ((diff < TROOM_HOT) && (diff >= 0))
       return ROOM_OK;
  if (diff <= 0)
       return ROOM_COOL;    
}

int checkWeatherTemp (struct weatherStatus* chk, struct AirCoolConfig* airconfig)
{
  int  diff = (int)(chk->currentTemp - airconfig->setTmp);
  if (diff  >= TOUT_HOT)
       return WEATHER_HOT;
  if ((diff < TOUT_HOT) && (diff >= 0))
       return WEATHER_OK;
  if (diff < 0)
       return WEATHER_COOL;     
}

int checkWeather (struct weatherStatus* chk)
 {

    if (!strcmp(chk->description,STR_CLEAR))
    {
        debugPrintln("Clear sky detect");
        return CLEAR;
    }    
    if (!strcmp(chk->description,STR_CLOUDS))
    {
        debugPrintln("Clouds  detect");
        return CLOUDS;
    }    
    if (!strcmp(chk->description,STR_RAIN))
    {
        debugPrintln("Rain detect");
        return RAIN;
    }    
        if (!strcmp(chk->description,STR_HAZE))
    {
        debugPrintln("Haze detect");
        return HAZE;
    }    
        if (!strcmp(chk->description,STR_THUNDERSTORM))
    {
        debugPrintln("Thunderstorm detect");
        return THUNDERSTORM;
    }    
    
    if (!strcmp(chk->description,STR_DRIZZLE))
    {
        debugPrintln("Drizzle detect");
        return DRIZZLE;
    }   
       debugPrintln("Cannot determine weather characteristics.");
        return NOT_FOUND;
 }


int  checkLocalTime()
{
  struct tm timeinfo;
  int    chktime;
  int  timeofday;
  if(!getLocalTime(&timeinfo)){
    debugPrintln("Failed to obtain time");
    return TIME_ERROR ;
  }
  chktime = timeinfo.tm_hour;
  if ((chktime  >= 5) && (chktime < 11))
        return MORNING;
  if ((chktime >= 11) && (chktime < 18))
        return AFTERNOON;
  else
        return NIGHT;
  debugPrint("Time now  :  ");
  debugPrintln(chktime);
}



/* Get temperature coefficient by sum all the factors in table and deduct to 
 * range  -3 -> +3 
 * Check if factors flag not checked. We will not calculate
 *  
 */
int getCoeff(struct CurrentEnv* envi,struct AirCoolConfig* airconfig)
{
   int tempcoef = 0;
   int fancoef = 0;
   int timecoef = 0;
   if ((airconfig->activitiesFlag) || (airconfig->sleepFlag))
   {
        tempcoef += activValue[envi->activities];
        fancoef  += activWind[envi->activities];
        timecoef += activDuration[envi->activities];
   }
   if (airconfig->weatherFlag)
   {
        tempcoef += weatherValue[envi->weather];  
        fancoef  += weatherWind[envi->weather];
        timecoef += weatherDuration[envi->weather];
 
   }
   if (airconfig->weatherFlag)
   {
        tempcoef += diffTempOutValue[envi->diffweather];
        fancoef  += diffTempOutWind[envi->diffweather];
        timecoef += diffTempOutDuration[envi->diffweather];
   }
   if (airconfig->timeFlag)
   {
        tempcoef += dayTimeValue[envi->timeofday];     
        fancoef  += dayTimeWind[envi->timeofday];
        timecoef += dayTimeDuration[envi->timeofday];
   }
   tempcoef += diffRoomTempValue[envi->diffroom];    // Always check 
   fancoef  += diffRoomTempWind[envi->diffroom];
   timecoef += diffRoomTempDuration[envi->diffroom];
// Adjust coef into range -2 < temp < 3,  fan speed  = auto, high, 25 <  duration < 75
   debugPrint("Before adjust  Temp  =  ");
   debugPrint ( tempcoef);
   debugPrint("   Wind  =  ");
   debugPrint ( fancoef);
   debugPrint("   Duration  =  ");
   debugPrintln ( timecoef);
 
   tempcoef  /= 2;
   timecoef  /= 2;
   fancoef   /= 2;
   

   airconfig->targetTmp = airconfig->setTmp + tempcoef;
   // Check target Tmp is in range MIN- MAX
   if (airconfig->targetTmp < MIN_TEMP)
        airconfig->targetTmp = MIN_TEMP;
   if (airconfig->targetTmp > MAX_TEMP)
        airconfig->targetTmp = MAX_TEMP;
        
   airconfig->duration  = timecoef;
   airconfig->fanspeed  = fancoef;
   debugPrint("Target Temp  =  ");
   debugPrint (airconfig->targetTmp);
   debugPrint("   Fan Speed  =  ");
   debugPrint (airconfig->fanspeed);
   debugPrint("   Duration  =  ");
   debugPrintln (airconfig->duration);

 
}


