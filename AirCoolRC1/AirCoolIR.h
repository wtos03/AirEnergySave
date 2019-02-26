//Send IR for Aircool project
// Include file for  IR command  start from 22- 28 with fan AUTO and high speed 
// Command for air con brand name is in AirCommand.h



#define SendIRxTimes 2

// Command send to AirCon

#define TEMP22AUTO  1
#define TEMP23AUTO  2
#define TEMP24AUTO  3
#define TEMP25AUTO  4
#define TEMP26AUTO  5
#define TEMP27AUTO  6
#define TEMP28AUTO  7
#define TEMP22HIGH  8
#define TEMP23HIGH  9
#define TEMP24HIGH  10
#define TEMP25HIGH  11
#define TEMP26HIGH  12
#define TEMP27HIGH  13
#define TEMP28HIGH  14
#define TURNOFF     20
#define TURNON       4   // Turn on air and set to 25 C  Auto Fan

#define STARTTEMP   21
#define HIGHFAN      7   // Offset for High Fan



void sendIR( unsigned int *data, unsigned int codeLen);
void  sendCommand( unsigned int cmd);
