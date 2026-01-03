#ifndef __DEBUG_H__
#define __DEBUG_H__


#define DEBUG

#ifdef DEBUG
#define DEBUG_BEGIN(a) Serial.begin(a)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#define DEBUG_PRINT(m) Serial.print(m)
#define DEBUG_PRINTLN(m) Serial.println(m)
#else
#define DEBUG_BEGIN(a) 
#define DEBUG_PRINTF(...)
#define DEBUG_PRINT(m) 
#define DEBUG_PRINTLN(m) 
#endif
#endif