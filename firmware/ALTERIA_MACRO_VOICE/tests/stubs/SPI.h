#pragma once
#define MSBFIRST 1
#define SPI_MODE0 0
struct SPISettings { SPISettings(unsigned long,int,int){} };
struct SPIClass { void begin(){} void beginTransaction(SPISettings){} uint8_t transfer(uint8_t v){return v;} void endTransaction(){} };
static SPIClass SPI;
