#pragma once
#include <stdint.h>
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A5 19
#define A6 20
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
static int pins[32], dirs[32], adc[32];
static uint32_t clockUs;
inline void digitalWrite(int p,int v){pins[p]=v;}
inline int digitalRead(int p){return pins[p];}
inline void pinMode(int p,int v){dirs[p]=v;}
inline int analogRead(int p){return adc[p];}
inline uint32_t micros(){return clockUs;}
inline uint32_t millis(){return clockUs/1000;}
#include <stdlib.h>
#define PI 3.14159265358979323846
inline long map(long x,long a,long b,long c,long d){return (x-a)*(d-c)/(b-a)+c;}
inline long random(long a,long b){return b>a?a+rand()%(b-a):a;}
inline long random(long b){return random(0,b);}
inline void randomSeed(uint32_t s){srand(s);}
template<typename T> T max(T a,T b){return a>b?a:b;}
template<typename T> T min(T a,T b){return a<b?a:b;}
#include <string>
#define F(s) s
struct MockSerial {
  std::string tx;
  void begin(unsigned long){} void end(){}
  void print(const char *s){tx+=s;} void print(char c){tx+=c;}
  template<class T> void print(T v){tx+=std::to_string(v);}
  template<class T> void println(T v){print(v);tx+='\n';}
};
static MockSerial Serial;
