#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../ALTERIA_ALLROUNDER_2_1_0.ino"
static void tick(int n=1){while(n--)voiceAudioTick();}
static void advance(uint16_t ms){while(ms--){clockUs+=1000;loop();tick(6);}}
int main(){
 setup();
 // Continuous pitch law, not quantized to notes; 0..5 V spans five octaves.
 uint32_t previous=0;
 for(int cv=0;cv<=4095;cv++){
  uint32_t inc=voicePitchIncrement(cv);assert(inc>previous);previous=inc;
  double expected=440*std::pow(2.0,(24.0+60.0*cv/4095-69)/12)*4294967296.0/VOICE_RATE;
  double cents=1200*std::log2(inc/expected);assert(std::abs(cents)<1.0);
 }
 assert(voicePitchIncrement(4095)==voicePitchIncrement(0)*32U);
 enterMode(MODE_ACID_VOICE);assert(voiceActive&&dirs[2]==INPUT);tick();
 assert(voiceWritten[0]==DAC_ZERO_CODE[0]&&voiceWritten[1]==DAC_ZERO_CODE[1]);
 voiceEvents(false,true);tick(32);assert(voiceSquare&&voiceWaveMix==32&&!voiceHits);
 voiceEvents(false,true);tick(32);assert(!voiceSquare&&voiceWaveMix==0&&!voiceHits);
 // Filter states, signed math, full control corners and exact envelope endpoints.
 for(int wave=0;wave<2;wave++)for(int decay: {0,400,1023})for(int resonance: {0,256,512,768,1023})
 for(int modulation: {0,256,512,768,1023})for(int cv: {0,1024,2048,3072,4095}){
  pot1=decay;pot2=resonance;pot3=modulation;voiceSquare=wave;voiceStart();
  cvLevelQ12=cv;voiceControls(millis()+2,true);voiceEvents(true,false);bool moved=false;
  for(int i=0;i<1600;i++){
   tick();moved|=voiceWritten[0]!=voiceZeroA;
   assert(voiceEnv<=32768&&voiceF>=VOICE_FILTER_BASE&&voiceF<=186);
   assert(std::abs(voiceLow)<=8192&&std::abs(voiceBandState)<=8192);
   assert(voiceWritten[0]>=voiceZeroA-voicePos&&voiceWritten[0]<=voiceZeroA+voiceNeg);
   assert(voiceWritten[1]>=DAC_PLUS5_CODE[1]&&voiceWritten[1]<=DAC_ZERO_CODE[1]);
   if(i==15)assert(voiceWritten[1]==DAC_PLUS5_CODE[1]);
  }
  assert(moved);tick(24000);assert(!voiceEnv&&!voiceAttack&&!voiceLow&&!voiceBandState);
  assert(voiceWritten[0]==voiceZeroA&&voiceWritten[1]==voiceZeroB);
 }
 // Retrigger ramps from the current level; held gates do not continuously retrigger.
 pot1=600;pot2=600;pot3=700;voiceStart();voiceEvents(true,false);tick(500);
 uint16_t level=voiceEnv;voiceEvents(true,false);tick();assert(voiceEnv>=level&&voiceAttack==15);
 // Retrigger very near the peak must never exceed the envelope's +5 V endpoint.
 for(int gap=0;gap<256;gap++){
  voiceEnv=32768-gap;voiceEvents(true,false);
  for(int n=0;n<16;n++){tick();assert(voiceEnv<=32768&&voiceWritten[1]>=DAC_PLUS5_CODE[1]);}
 }
 // Real loop routing and UI: pitch changes never become triggers; MANUAL is wave only.
 voiceSquare=false;adc[A2]=0;pins[2]=HIGH;enterMode(MODE_ACID_VOICE);advance(10);assert(!voiceHits);
 adc[A2]=900;advance(20);assert(!voiceHits&&voiceEnv==0);
 pins[PIN_MANUAL_BUTTON]=HIGH;advance(25);pins[PIN_MANUAL_BUTTON]=LOW;advance(25);
 assert(voiceSquare&&!voiceHits&&pins[PIN_INPUT_LED]==HIGH);
 pins[2]=LOW;advance(5);pins[2]=HIGH;advance(5);pins[2]=LOW;advance(5);assert(voiceHits==1);
 advance(50);assert(voiceHits==1);
 // Both ranges follow independent calibration, including the envelope's +5 endpoint.
 DAC_ZERO_CODE[0]=2068;DAC_PLUS5_CODE[0]=114;DAC_MINUS5_CODE[0]=4032;
 DAC_ZERO_CODE[1]=2064;DAC_PLUS5_CODE[1]=126;voiceStart();tick();
 assert(voiceWritten[0]==2068&&voiceWritten[1]==2064&&voicePos==977);
 voiceEvents(true,false);tick(16);assert(voiceWritten[1]==126);
 enterMode(MODE_GRIDS);assert(!voiceActive&&dirs[2]==OUTPUT&&dacLastA==2068);
 puts("PASS: continuous pitch/octaves, waveform-only MANUAL, 750 filter/control/pitch cases, mixed DAC ranges, finite tails, retriggers, held-gate startup, loop routing, calibration and safe exit");
}
