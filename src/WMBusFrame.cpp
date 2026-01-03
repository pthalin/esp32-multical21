/*
 Copyright (C) 2020 chester4444@wolke7.net
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Uncoment the line below for exprimental FlowIQ 2200 support. NOTE: It only reports the volume, other values like temperature will be 0. */

//#define FLOWIQ2200

#include "WMbusFrame.h"
#include "debug.h"

void  mqttMyData(const char* debug_str);
void  mqttMyDataJson(const char* debug_str);

WMBusFrame::WMBusFrame()
{
  aes128.setKey(key, sizeof(key));
}

void WMBusFrame::check()
{
    // check meterId
    for (uint8_t i = 0; i< 4; i++)
    {
        if (meterId[i] != payload[6-i])
        {
          isValid = false;
          return;
        }
    }

  DEBUG_PRINTF("Payload: "); 
  for(int k=0; k < length; k++) { 
    DEBUG_PRINTF("%02x", payload[k]);
  }
  DEBUG_PRINTF("\n\r");

  isValid = true;
}

void WMBusFrame::printMeterInfo(uint8_t *data, size_t len)
{
    // init positions for compact frame
  int pos_tt = 9; // total consumption, 9, 10, 11, 12
  int pos_tg = 13; // target consumption 13, 14, 15, 16
  //int pos_ic = 7; // info codes
  int pos_ft = 17; // flow temp
  int pos_at = 18; // ambient temp
  
  char mqttstring[25];
  char mqttjsondstring[100];

  DEBUG_PRINTF("Data: "); 
  for(int k=0; k < len; k++) { 
    DEBUG_PRINTF("%02x", data[k]);
  }
  DEBUG_PRINTF("\n\r");

#ifdef FLOWIQ2200
  if(data[2] == 0x79)  //compact frame
  {
    pos_tt = 29; 
  }
#else
  // Multical 21
  if(data[2] == 0x79)  //compact frame
  {
    pos_tt = 9;
    pos_tg = 13;
    //pos_ic = 7;
    pos_ft = 17;
    pos_at = 18;
  }
  else if (data[2] == 0x78) // long frame
  {
    // overwrite it with long frame positions
    pos_tt = 10;
    pos_tg = 16;
    //pos_ic = 6;
    pos_ft = 23;
    pos_at = 29;
  } 
#endif
  else
    return;

  uint16_t calc_crc = crc16_EN13757(data+2, len-2);
  uint16_t read_crc = data[1] << 8 | data[0];
  DEBUG_PRINTF("calc_crc: 0x%04x\n\r", calc_crc);
  DEBUG_PRINTF("read_crc: 0x%04x\n\r", read_crc);

  if (calc_crc == read_crc) 
  {
    DEBUG_PRINTF("CRC: OK\n\r");
  }
  else{
    DEBUG_PRINTF("CRC: ERROR\n\r");
    return;
  }


  char total[10];
  uint32_t tt = data[pos_tt]
              + (data[pos_tt+1] << 8)
              + (data[pos_tt+2] << 16)
              + (data[pos_tt+3] << 24);
  snprintf(total, sizeof(total), "%d.%03d", tt/1000, tt%1000 );
  DEBUG_PRINTF("total: %s m%c - ", total, 179);
  snprintf(mqttstring, sizeof(mqttstring), "%d.%03d", tt/1000, tt%1000 );
  mqttMyData(mqttstring);

#ifdef FLOWIQ2200
  snprintf(mqttjsondstring, sizeof(mqttjsondstring), "{\"CurrentValue\": %d.%03d,\"MonthStartValue\": %d.%03d,\"WaterTemp\": %2d,\"RoomTemp\": %2d}",tt/1000, tt%1000, 0, 0, 0, 0);
#else
  char target[10];
  uint32_t tg = data[pos_tg]
              + (data[pos_tg+1] << 8)
              + (data[pos_tg+2] << 16)
              + (data[pos_tg+3] << 24);
  snprintf(target, sizeof(target), "%d.%03d", tg/1000, tg%1000 );
  DEBUG_PRINTF("target: %s m%c - ", target, 179);

  char flow_temp[3];
  snprintf(flow_temp, sizeof(flow_temp), "%2d", data[pos_ft]);
  DEBUG_PRINTF("%s %cC - ", flow_temp, 176);

  char ambient_temp[3];
  snprintf(ambient_temp, sizeof(ambient_temp), "%2d", data[pos_at]);
  DEBUG_PRINTF("%s %cC\n\r", ambient_temp, 176);

  snprintf(mqttjsondstring, sizeof(mqttjsondstring), "{\"CurrentValue\": %d.%03d,\"MonthStartValue\": %d.%03d,\"WaterTemp\": %2d,\"RoomTemp\": %2d}",tt/1000, tt%1000, tg/1000, tg%1000, data[pos_ft],data[pos_at]);
#endif
  mqttMyDataJson(mqttjsondstring);
}

void WMBusFrame::decode()
{
  // check meterId, CRC
  check();
  if (!isValid) return;

  uint8_t cipherLength = length - 2 - 16; // cipher starts at index 16, remove 2 crc bytes
  memcpy(cipher, &payload[16], cipherLength);

  memset(iv, 0, sizeof(iv));   // padding with 0
  memcpy(iv, &payload[1], 8);
  iv[8] = payload[10];
  memcpy(&iv[9], &payload[12], 4);

  aes128.setIV(iv, sizeof(iv));
  aes128.decrypt(plaintext, (const uint8_t *) cipher, cipherLength);

/*
  DEBUG_PRINTF("C:     ");
  for (size_t i = 0; i < cipherLength; i++)
  {
    DEBUG_PRINTF("%02X", cipher[i]);
  }
  DEBUG_PRINTLN();
  DEBUG_PRINTF("P(%d): ", cipherLength);
  for (size_t i = 0; i < cipherLength; i++)
  {
    DEBUG_PRINTF("%02X", plaintext[i]);
  }
  DEBUG_PRINTLN();
*/

  printMeterInfo(plaintext, cipherLength);
}


uint16_t WMBusFrame::crc16_EN13757(uint8_t *data, size_t len)
{
    uint16_t crc = 0x0000;

    assert(len == 0 || data != NULL);

    for (size_t i=0; i<len; ++i)
    {
        crc = crc16_EN13757_per_byte(crc, data[i]);
    }

    return (~crc);
}

#define CRC16_EN_13757 0x3D65

uint16_t WMBusFrame::crc16_EN13757_per_byte(uint16_t crc, uint8_t b)
{
    unsigned char i;

    for (i = 0; i < 8; i++) {

        if (((crc & 0x8000) >> 8) ^ (b & 0x80)){
            crc = (crc << 1)  ^ CRC16_EN_13757;
        }else{
            crc = (crc << 1);
        }

        b <<= 1;
    }

    return crc;
}
