#ifdef DEBUG
#define debugPrint(x) Serial.print(x);
#define debugPrintln(x) Serial.println(x);
#define debugPrintMulti(x,y)  Serial.println(x,y);  
#define debugWrite(x)  Serial.write(x);
#else
#define debugPrint(x)  
#define debugPrintln(x)  
#define debugPrintMulti(x,y)  
#define debugWrite(x)
#endif
