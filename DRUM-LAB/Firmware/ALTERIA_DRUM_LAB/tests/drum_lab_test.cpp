#include <cassert>
#include <cstdio>
#include <vector>
#include "../ALTERIA_DRUM_LAB.ino"
static void tick(int n=1){while(n--)audioTick();}
static void advance(uint32_t ms){for(uint32_t i=0;i<ms;i++){clockUs+=1000;loop();}}
static void button(uint8_t pin,uint16_t hold=60){pins[pin]=HIGH;advance(hold);pins[pin]=LOW;advance(60);}
static void drainSave(){for(int i=0;i<300&&kitWriteStep>=0;i++)serviceKitSave(millis());assert(kitWriteStep<0);}
static void choose(uint8_t a,uint8_t b){engine[0]=a;engine[1]=b;publish(0,true);publish(1,true);tick();}
int main(){
 setup();assert(audioActive&&dirs[2]==INPUT&&pins[2]==LOW);tick();
 assert(audioWritten[0]==DAC_ZERO_CODE[0]&&audioWritten[1]==DAC_ZERO_CODE[1]);
 for(int ch=0;ch<2;ch++)for(int sample=-127;sample<=127;sample++){
  uint16_t expected=sample>=0?audioZero[ch]-((uint32_t)sample*audioPos[ch]>>7):
    audioZero[ch]+((uint32_t)(-sample)*audioNeg[ch]>>7);
  assert(audioCode(ch,sample)==expected);
 }
 // Every engine, both isolated outputs, full parameter corners and retriggers.
 for(int ch=0;ch<2;ch++)for(int e=0;e<8;e++)for(int corner=0;corner<64;corner++){
  engine[ch]=e;for(int i=0;i<6;i++)settings[ch][e][i]=(corner&(1<<i))?1023:0;
  voices[0]=Voice();voices[1]=Voice();voices[0].rng=0xACE1;voices[1].rng=0xBEEF;
  publish(ch,true);tick();trigger(1<<ch,0);bool moved=false;
  for(int n=0;n<700;n++){
   tick();moved|=audioWritten[ch]!=audioZero[ch];
   assert(audioWritten[1-ch]==audioZero[1-ch]);
   assert(audioWritten[ch]>=audioZero[ch]-audioPos[ch]&&audioWritten[ch]<=audioZero[ch]+audioNeg[ch]);
  }
  // Extreme filtering may intentionally remove a low-pitched voice completely.
  if(!(corner&8))assert(moved);
  tick(24000);assert(voices[ch].env==0&&voices[ch].repeats==0&&audioWritten[ch]==audioZero[ch]);
 }
 defaults();startAudio();choose(SNARE,CLAP);trigger(3,0);tick();assert(hitCount[0]==1&&hitCount[1]==1);
 trigger(1,0);tick();assert(hitCount[0]==2&&hitCount[1]==1);
 // Clap must complete all bursts even with the widest spacing and silence gaps.
 engine[0]=CLAP;settings[0][CLAP][0]=1023;publish(0,true);tick();trigger(1,0);tick();
 uint8_t repeats=voices[0].repeats;int bursts=0;
 for(int i=0;i<1000;i++){tick();if(voices[0].repeats<repeats){bursts++;repeats=voices[0].repeats;}}
 assert(bursts==3&&repeats==0);
 // Closed hat wins simultaneous triggers and chokes an existing open tail.
 choose(CLOSED_HAT,OPEN_HAT);trigger(2,0);tick(30);assert(voices[1].env>0);
 trigger(1,0);tick();assert(voices[1].choked);tick(300);assert(!voices[1].env);
 uint16_t bHits=hitCount[1];trigger(3,0);tick();assert(hitCount[1]==bHits);
 choose(OPEN_HAT,CLOSED_HAT);trigger(1,0);tick(20);trigger(2,0);tick();assert(voices[0].choked);
 // No false CV/I/O edges at startup with gates already high.
 adc[A2]=800;pins[2]=HIGH;startAudio();advance(10);assert(!pendingHits);
 adc[A2]=0;pins[2]=LOW;advance(10);adc[A2]=800;advance(2);assert(pendingHits&1);tick();
 pins[2]=HIGH;advance(2);assert(pendingHits&2);tick();adc[A2]=0;pins[2]=LOW;
 // UI: next engine, long press selects B without stepping, chord toggles FX only.
 defaults();startAudio();selectedVoice=0;page=0;modePressArmed=modeLongHandled=false;
 button(PIN_MODE_BUTTON);assert(engine[0]==TOM&&selectedVoice==0);
 button(PIN_MODE_BUTTON,750);assert(selectedVoice==1&&engine[1]==SNARE&&page==0);
 uint16_t oldA=hitCount[0],oldB=hitCount[1];uint8_t oldEngine=engine[1];
 pins[PIN_MODE_BUTTON]=HIGH;advance(60);pins[PIN_MANUAL_BUTTON]=HIGH;advance(60);
 pins[PIN_MODE_BUTTON]=pins[PIN_MANUAL_BUTTON]=LOW;advance(60);tick();
 assert(page==1&&engine[1]==oldEngine&&hitCount[0]==oldA&&hitCount[1]==oldB);
 button(PIN_MANUAL_BUTTON);tick();assert(hitCount[1]==oldB+1&&hitCount[0]==oldA);
 // Pickup cannot jump a recalled value, then follows after crossing it.
 page=0;selectedVoice=0;engine[0]=KICK;settings[0][KICK][0]=700;
 pot1=100;resetPickup();editPots();assert(!picked[0]&&settings[0][KICK][0]==700);
 pot1=800;editPots();assert(picked[0]&&settings[0][KICK][0]==800);
 trigger(1,0xFFFFFFF0U);display(5);assert(ledOn[0]);display(10);assert(!ledOn[0]);
 // Full kit persistence, isolation, CRC and every interrupted two-bank write.
 defaults();engine[0]=FM_METAL;settings[0][FM_METAL][4]=333;startKitSave();drainSave();
 assert(notice==1&&kitSequence==1);uint8_t original[1024];memcpy(original,EEPROM.bytes,1024);
 for(int cut=0;cut<=205;cut++){
  memcpy(EEPROM.bytes,original,1024);loadKit();settings[0][FM_METAL][4]=777;startKitSave();
  for(int i=0;i<cut;i++)serviceKitSave(0);
  kitWriteStep=-1;loadKit();assert(engine[0]==FM_METAL);
  assert(settings[0][FM_METAL][4]==(cut>=205?777:333));
  for(int i=0;i<512;i++)assert(EEPROM.bytes[i]==original[i]);
 }
 loadKit();startKitSave();EEPROM.blockWrites=true;drainSave();assert(notice==2);EEPROM.blockWrites=false;
 EEPROM.bytes[KIT_BASE+kitBank*KIT_STRIDE+9]^=1;loadKit();assert(settings[0][FM_METAL][4]==333);
 // Hidden calibration is input-only and restarts audio only on exit.
 stopAudio();startCalibration(0);assert(calibrationActive&&!audioActive&&dirs[2]==INPUT);
 calExit();assert(!calibrationActive&&audioActive);
 puts("PASS: 8 engines x 64 corners x 2 outputs, tails/ranges/independence, clap bursts, hat choke, gate edges, UI/chord/pickup, kit CRC/interrupted writes/isolation, calibration transition");
}
