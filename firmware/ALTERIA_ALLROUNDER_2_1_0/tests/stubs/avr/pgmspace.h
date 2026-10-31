#pragma once
#include <stdint.h>
#define PROGMEM
inline uint16_t pgm_read_word(const uint16_t *p) { return *p; }
template<class T> inline uint8_t pgm_read_byte(const T *p){return (uint8_t)*p;}

inline uint32_t pgm_read_dword(const uint32_t *p){return *p;}
