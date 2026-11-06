#pragma once
#include <stdint.h>
#include <cstring>
struct EEPROMClass {
  uint8_t bytes[1024];
  bool blockWrites=false;
  unsigned writes=0;
  EEPROMClass(){memset(bytes,255,sizeof(bytes));}
  uint8_t read(int n){return bytes[n];}
  void update(int n,uint8_t v){if(!blockWrites){bytes[n]=v;writes++;}}
};
static EEPROMClass EEPROM;
