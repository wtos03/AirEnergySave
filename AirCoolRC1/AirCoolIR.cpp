// Send IR for AirCool project

#include "ESP32_IR_Remote.h"
#include "AirCoolIR.h"
#include "AirCommand.h"
#include  "DebugPrint.h"


ESP32_IRrecv irrecv;



// Send IR 
void sendIR( unsigned int *data, unsigned int codeLen)
{       
        irrecv.ESP32_IRsendPIN(SEND_PIN,0);//channel 0 so it can use the full memory of the channels
        irrecv.initSend(); //setup the ESP32 to send IR code
        debugPrint("RAW code length: ");
        debugPrintln(codeLen);
        for(int i = 0;i<SendIRxTimes;i++){ //send IR code that was recorded
            delay(500);         
            irrecv.sendIR(data,codeLen);
        }
        irrecv.stopIR(); //uninstall the RMT channel so it can be reused for Receiving IR or send IR
}

void  sendCommand( unsigned int cmd)
{ 
        int len;
        switch (cmd)
        {
        case TEMP22AUTO:
             len =  ((sizeof(Temp22Auto))/sizeof(unsigned int));
             sendIR(Temp22Auto,len);
             break;
        case TEMP23AUTO:
             len =  ((sizeof(Temp23Auto))/sizeof(unsigned int));
             sendIR(Temp23Auto,len);
             break;
        case TEMP24AUTO:
             len =  ((sizeof(Temp24Auto))/sizeof(unsigned int));
             sendIR(Temp24Auto,len);
             break;
        case TEMP25AUTO:
             len =  ((sizeof(Temp25Auto))/sizeof(unsigned int));
             sendIR(Temp25Auto,len);
             break;
        case TEMP26AUTO:
             len =  ((sizeof(Temp26Auto))/sizeof(unsigned int));
             sendIR(Temp26Auto,len);
             break;
        case TEMP27AUTO:
             len =  ((sizeof(Temp27Auto))/sizeof(unsigned int));
             sendIR(Temp27Auto,len);
             break;
        case TEMP28AUTO:
             len =  ((sizeof(Temp28Auto))/sizeof(unsigned int));
             sendIR(Temp28Auto,len);
             break;
        case TEMP22HIGH:
             len =  ((sizeof(Temp22High))/sizeof(unsigned int));
             sendIR(Temp22High,len);
             break;
        case TEMP23HIGH:
             len =  ((sizeof(Temp23High))/sizeof(unsigned int));
             sendIR(Temp22High,len);
             break;
        case TEMP24HIGH:
             len =  ((sizeof(Temp24High))/sizeof(unsigned int));
             sendIR(Temp24High,len);
             break;
        case TEMP25HIGH:
             len =  ((sizeof(Temp25High))/sizeof(unsigned int));
             sendIR(Temp25High,len);
             break;
        case TEMP26HIGH:
             len =  ((sizeof(Temp26High))/sizeof(unsigned int));
             sendIR(Temp26High,len);
             break;
        case TEMP27HIGH:
             len =  ((sizeof(Temp27High))/sizeof(unsigned int));
             sendIR(Temp27High,len);
             break;
        case TEMP28HIGH:
             len =  ((sizeof(Temp28High))/sizeof(unsigned int));
             sendIR(Temp28High,len);
             break;
        case TURNOFF:
             len =  ((sizeof(TurnOff))/sizeof(unsigned int));
             sendIR(TurnOff,len);
             break;       
        }
   
   
}
