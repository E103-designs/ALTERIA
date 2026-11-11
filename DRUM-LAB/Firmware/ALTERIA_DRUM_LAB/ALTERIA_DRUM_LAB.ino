/*
  ALTERIA DRUM LAB 1.0.0 - 2026-10-27
  Nano ATmega328P 16 MHz / ALTERIA REV5-2 / MCP4822.
  Original fixed-point synthesis inspired by the LXR voice families.
  NOT the LXR firmware, a faithful emulation, or an Erica Synths product.
  Two simultaneous mono voices, eight engines per voice, 6.4 kHz audio.
  CV IN -> voice A -> OUT1; I/O INPUT -> voice B -> OUT2, about +/-2.5 V.
  MODE tap: next engine. MODE hold 650 ms: select A/B.
  Hold MODE then tap MANUAL: SOUND/FX page. MANUAL alone: selected voice.
  Hold MANUAL 2 s: save kit. Pot pickup prevents jumps after selection.
  SOUND P1 pitch, P2 decay, P3 character. FX P1 tone, P2 drive, P3 crush wet.
  LED1/2/3: engine 1..8 modulo 8, binary weights 1/2/4.
  LED4: A off / B on; FX page A short-on, B short-off every 500 ms.
  Existing ACAL calibration at EEPROM 256..301 is loaded, never guessed.
  Hold MODE+MANUAL at boot: same three-point hidden calibration as Allrounder.
  Kit EEPROM 512..939 only; old preset and calibration areas preserved.
  SPI.transfer/digitalWrite + extended LDAC preserve the working DAC path.
  No audio inputs, samples, MIDI, LXR kits or original LXR master effects.
  See README.md and TEST_REPORT.md before patching. No direct headphones.
*/
#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#if defined(__AVR__) && (!defined(__AVR_ATmega328P__) || F_CPU!=16000000UL)
#error "DRUM LAB requires an ATmega328P at 16 MHz (Arduino Nano)."
#endif

#define PIN_OUT3 2
#define PIN_INPUT_LED 3
#define PIN_MODE_BUTTON 4
#define PIN_IO_LED 5
#define PIN_LED_MODE_1 6
#define PIN_DAC_CS 10
#define PIN_DAC_LDAC 12
#define PIN_POT_1 A0
#define PIN_POT_2 A1
#define PIN_CV_INPUT A2
#define PIN_MANUAL_BUTTON A3
#define PIN_POT_3 A6
#define ALTERIA_DEBUG_SERIAL 0

enum Engine : uint8_t { KICK,TOM,PERC,SNARE,CLAP,FM_METAL,CLOSED_HAT,OPEN_HAT };
struct Button {
  uint8_t pin;uint16_t debounceMs;
  bool raw,stable,rising,falling;uint32_t rawSinceMs;
};
struct Params {
  uint16_t inc,inc2,inc3,decay,bend;
  uint8_t algorithm,character,tone,drive,crush,repeatInterval;
};
struct Voice {
  uint16_t phase,phase2,phase3,env,body,pitch,rng,burstWait;
  int16_t noiseLow,filterLow,held;
  uint8_t repeats,holdCounter;bool choked;
};
const uint16_t AUDIO_RATE=6400;
const int8_t SINE_TABLE[256] PROGMEM={
  0,3,6,9,12,16,19,22,25,28,31,34,37,40,43,46,
  49,51,54,57,60,63,65,68,71,73,76,78,81,83,85,88,
  90,92,94,96,98,100,102,104,106,107,109,111,112,113,115,116,
  117,118,120,121,122,122,123,124,125,125,126,126,126,127,127,127,
  127,127,127,127,126,126,126,125,125,124,123,122,122,121,120,118,
  117,116,115,113,112,111,109,107,106,104,102,100,98,96,94,92,
  90,88,85,83,81,78,76,73,71,68,65,63,60,57,54,51,
  49,46,43,40,37,34,31,28,25,22,19,16,12,9,6,3,
  0,-3,-6,-9,-12,-16,-19,-22,-25,-28,-31,-34,-37,-40,-43,-46,
  -49,-51,-54,-57,-60,-63,-65,-68,-71,-73,-76,-78,-81,-83,-85,-88,
  -90,-92,-94,-96,-98,-100,-102,-104,-106,-107,-109,-111,-112,-113,-115,-116,
  -117,-118,-120,-121,-122,-122,-123,-124,-125,-125,-126,-126,-126,-127,-127,-127,
  -127,-127,-127,-127,-126,-126,-126,-125,-125,-124,-123,-122,-122,-121,-120,-118,
  -117,-116,-115,-113,-112,-111,-109,-107,-106,-104,-102,-100,-98,-96,-94,-92,
  -90,-88,-85,-83,-81,-78,-76,-73,-71,-68,-65,-63,-60,-57,-54,-51,
  -49,-46,-43,-40,-37,-34,-31,-28,-25,-22,-19,-16,-12,-9,-6,-3
};
int16_t DAC_ZERO_CODE[2]={2048,2048};
int16_t DAC_PLUS5_CODE[2]={87,87};
int16_t DAC_MINUS5_CODE[2]={4008,4008};
uint16_t audioZero[2],audioPos[2],audioNeg[2];
uint16_t dacLastA=65535,dacLastB=65535;
Voice voices[2];
Params parameters[2][2];
volatile uint8_t liveBank[2]={0,0},pendingHits=0,pendingReset=0;
volatile bool audioActive=false;
volatile uint16_t audioFrames=0,hitCount[2]={0,0};
uint16_t settings[2][8][6];
uint8_t engine[2]={KICK,SNARE},selectedVoice=0,page=0;
bool picked[3]={false,false,false};
uint16_t previousPot[3],pot1=0,pot2=0,pot3=0;
uint32_t lastPotsMs=0,lastCvPollUs=0,ledUntil[2]={0,0};
bool ledOn[2]={false,false};
bool cvGate=false,ioGate=false;
Button manualBtn={PIN_MANUAL_BUTTON,18,false,false,false,false,0};
Button modeBtn={PIN_MODE_BUTTON,35,false,false,false,false,0};
bool modePressArmed=false,modeLongHandled=false,chordLocked=false;
bool manualArmed=false,ignoreButtonsUntilRelease=false;
uint32_t modePressedMs=0,manualPressedMs=0,noticeAt=0;
uint8_t notice=0;
const uint16_t KIT_BASE=512,KIT_STRIDE=224;
const uint16_t KIT_BYTES=204;
uint8_t kitRecord[KIT_BYTES],kitBank=1,kitWriteBank=0;
uint16_t kitSequence=0;
int16_t kitWriteStep=-1;
#if !defined(__AVR__)
uint16_t audioWritten[2];
#endif

#if defined(__AVR__)
#pragma GCC push_options
#pragma GCC optimize("O2")
#endif
static bool elapsedMs(uint32_t now,uint32_t since,uint32_t period){return (uint32_t)(now-since)>=period;}
static int16_t clip(int16_t x){return x>127?127:x<-127?-127:x;}
static int16_t sine(uint8_t x){return (int8_t)pgm_read_byte(&SINE_TABLE[x]);}
// Inputs are bounded to +/-254: their unsigned magnitude product fits 16 bits.
static inline __attribute__((always_inline)) int16_t signedGain8(int16_t value,uint8_t gain){
  if(value>=0)return ((uint16_t)value*gain)>>8;
  return -(int16_t)(((uint16_t)(-value)*gain)>>8);
}
static int16_t mix8(int16_t a,int16_t b,uint8_t wet){return a+signedGain8(b-a,wet);}
static int16_t amplitude(int16_t sample,uint16_t env){return (sample*(int16_t)(env>>8))>>7;}
static uint16_t decayEnvelope(uint16_t env,uint16_t coefficient){
  if(env<256)return 0; // Below the output gain resolution; also finish chokes promptly.
  uint16_t coarse=env>>8;
  uint16_t d=coarse*(coefficient>>8)+((coarse*(coefficient&255))>>8);
  return env-(d?d:(env?1:0));
}
static void dacWriteRaw(uint8_t ch,uint16_t code){
  digitalWrite(PIN_DAC_CS,LOW);
  SPI.transfer((uint8_t)(((ch?0x9000:0x1000)|code)>>8));SPI.transfer((uint8_t)code);
  digitalWrite(PIN_DAC_CS,HIGH);
}
static void latchDac(){
  digitalWrite(PIN_DAC_LDAC,LOW);
#if defined(__AVR__)
  __builtin_avr_delay_cycles(F_CPU/1000000UL);
#endif
  digitalWrite(PIN_DAC_LDAC,HIGH);
}
static uint16_t audioCode(uint8_t ch,int16_t sample){
  uint16_t span=sample>=0?audioPos[ch]:audioNeg[ch];
  uint8_t magnitude=sample>=0?sample:-sample;
  uint16_t offset=magnitude*(span>>7)+((magnitude*(span&127))>>7);
  return sample>=0?audioZero[ch]-offset:audioZero[ch]+offset;
}
static void writeFrame(uint16_t a,uint16_t b){
#if defined(__AVR__)
  dacWriteRaw(0,a);dacWriteRaw(1,b);latchDac();
#else
  audioWritten[0]=a;audioWritten[1]=b;
#endif
  audioFrames++;
}
static inline __attribute__((always_inline)) int16_t filterSample(Voice &v,int16_t input,uint8_t tone){
  if(tone>=124&&tone<=132)return input;
  uint8_t coefficient=tone<124?8+tone*2:(tone-132)*2;
  v.filterLow+=signedGain8(input-v.filterLow,coefficient);
  return tone<124?v.filterLow:clip(input-v.filterLow);
}
static int16_t morphWave(uint8_t phase,uint8_t colour){
  uint8_t t=phase+64;
  int16_t triangle=t<128?-127+(int16_t)t*2:383-(int16_t)t*2;
  int16_t saw=clip((int16_t)phase-128);
  if(colour<85)return mix8(sine(phase),triangle,colour*3);
  if(colour<170)return mix8(triangle,saw,(colour-85)*3);
  return mix8(saw,phase<128?127:-127,(colour-170)*3);
}
static void strike(uint8_t ch,const Params &p){
  Voice &v=voices[ch];v.env=v.body=32767;v.pitch=p.bend;
  v.phase=v.phase2=v.phase3=0;v.choked=false;v.repeats=p.algorithm==CLAP?3:0;
  v.burstWait=p.repeatInterval;v.holdCounter=0;v.held=0;
  hitCount[ch]++;
}
static inline __attribute__((always_inline)) int16_t renderVoice(uint8_t ch,const Params &p){
  Voice &v=voices[ch];
  if(!v.env&&!v.repeats)return 0;
  v.rng=(v.rng>>1)^((uint16_t)(-(int16_t)(v.rng&1))&0xB400);
  int16_t noise=(int8_t)(v.rng>>8);
  v.noiseLow+=(noise-v.noiseLow)>>2;
  int16_t high=clip(noise-v.noiseLow);
  v.phase+=p.inc+v.pitch;v.phase2+=p.inc2;v.phase3+=p.inc3;
  uint8_t phase=v.phase>>8;
  int16_t raw=0;
  if(p.algorithm<=PERC){
    int16_t pm=(sine(v.phase2>>8)*p.character)>>9;
    raw=p.algorithm==PERC?morphWave(phase+pm,p.character):sine(phase+pm);
    if(p.algorithm==KICK){
      int16_t snap=(high*(int16_t)(v.body>>8))>>9;
      raw=clip(raw+((snap*p.character)>>8));
    }
  } else if(p.algorithm==SNARE){
    int16_t body=(sine(phase)+sine(v.phase2>>8))/2;
    body=amplitude(body,v.body);
    raw=mix8(body,filterSample(v,high,p.tone),80+((uint16_t)p.character*160>>8));
  } else if(p.algorithm==CLAP){
    raw=filterSample(v,high,p.tone);
    if(v.repeats&&!--v.burstWait){
      v.env=16384+(uint16_t)p.character*64;v.repeats--;v.burstWait=p.repeatInterval;
    }
  } else {
    // Three operators, with the second modulator morphing from sine into noise.
    int16_t m2=mix8(sine(v.phase3>>8),noise,p.character);
    int16_t pm=signedGain8(sine(v.phase2>>8)+m2,32+(p.character>>1));
    raw=sine(phase+pm);
    if(p.algorithm>=CLOSED_HAT)raw=mix8(raw,high,100+(p.character>>1));
  }
  if(p.algorithm!=SNARE&&p.algorithm!=CLAP)raw=filterSample(v,raw,p.tone);
  // Saturation and fixed-rate degradation are dry/wet, never switched at mid-pot.
  if(p.drive)raw=mix8(raw,clip(raw*3),((uint16_t)p.drive*p.drive)>>8);
  if(p.crush){
    if(!v.holdCounter){v.held=(int16_t)((uint8_t)(raw+128)&0xF0)-120;v.holdCounter=7;}
    else v.holdCounter--;
    raw=mix8(raw,v.held,((uint16_t)p.crush*p.crush)>>8);
  }else v.holdCounter=0;
  return clip(amplitude(raw,v.env));
}
static void audioTick(){
  uint8_t reset=pendingReset;pendingReset=0;
  for(uint8_t ch=0;ch<2;ch++)if(reset&(1U<<ch)){
    voices[ch].env=0;voices[ch].filterLow=voices[ch].noiseLow=0;
  }
  uint8_t hits=pendingHits;pendingHits=0;
  const Params &a=parameters[0][liveBank[0]],&b=parameters[1][liveBank[1]];
  if((hits&1)&&a.algorithm==CLOSED_HAT&&b.algorithm==OPEN_HAT){voices[1].choked=true;hits&=(uint8_t)~2;}
  if((hits&2)&&b.algorithm==CLOSED_HAT&&a.algorithm==OPEN_HAT){voices[0].choked=true;hits&=(uint8_t)~1;}
  if(hits&1)strike(0,a);if(hits&2)strike(1,b);
  int16_t outA=renderVoice(0,a),outB=renderVoice(1,b);
  // Stagger the four envelope updates: at most one wide product per sample.
  uint8_t phase=audioFrames&3,ch=phase>>1;Voice &v=voices[ch];
  if(phase&1)v.body=decayEnvelope(v.body,2048);
  else {
    const Params &p=ch?b:a;
    v.env=decayEnvelope(v.env,v.choked?32768U:(v.repeats?16384U:p.decay));
    uint16_t drop=v.pitch>>5;v.pitch-=drop?drop:(v.pitch?1:0);
  }
  writeFrame(audioCode(0,outA),audioCode(1,outB));
}
#if defined(__AVR__)
ISR(TIMER1_COMPA_vect){audioTick();}
#pragma GCC pop_options
#endif
static void stopAudio(){
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
#if defined(__AVR__)
    TIMSK1&=(uint8_t)~_BV(OCIE1A);TCCR1B=0;
#endif
    audioActive=false;pendingHits=pendingReset=0;
  }
}
static uint16_t frequencyIncrement(uint16_t hz){return (uint32_t)hz*65536UL/AUDIO_RATE;}
static Params makeParams(uint8_t ch){
  const uint16_t *s=settings[ch][engine[ch]];
  Params p;p.algorithm=engine[ch];p.character=s[2]>>2;p.tone=s[3]>>2;p.drive=s[4]>>2;p.crush=s[5]>>2;
  p.repeatInterval=32+(uint32_t)s[0]*128/1023;
  uint16_t hz,minMs,maxMs;
  switch(p.algorithm){
    case KICK:hz=45+(uint32_t)s[0]*90/1023;minMs=60;maxMs=1800;break;
    case TOM:hz=70+(uint32_t)s[0]*430/1023;minMs=50;maxMs=1400;break;
    case PERC:hz=100+(uint32_t)s[0]*1300/1023;minMs=20;maxMs=1200;break;
    case SNARE:hz=185+(uint32_t)s[0]*70/1023;minMs=60;maxMs=1000;break;
    case CLAP:hz=180;minMs=70;maxMs=700;break;
    default:hz=250+(uint32_t)s[0]*1500/1023;minMs=p.algorithm==CLOSED_HAT?15:100;maxMs=p.algorithm==CLOSED_HAT?300:2200;break;
  }
  p.inc=frequencyIncrement(hz);
  uint16_t f2=p.algorithm==SNARE?330+(uint32_t)s[0]*110/1023:
    (uint32_t)hz*(p.algorithm<=PERC?200:141)/100;
  uint16_t f3=(uint32_t)hz*237/100;
  if(f2>2850)f2=2850;if(f3>2900)f3=2900;
  p.inc2=frequencyIncrement(f2);p.inc3=frequencyIncrement(f3);
  p.bend=p.algorithm==KICK?p.inc*3:p.algorithm==TOM?p.inc/2:p.algorithm==PERC?p.inc:0;
  uint16_t decayMs=minMs+(uint32_t)s[1]*s[1]*(maxMs-minMs)/(1023UL*1023UL);
  p.decay=245760UL/decayMs; // Envelope runs at AUDIO_RATE/4, not per sample.
  return p;
}
static void publish(uint8_t ch,bool reset=false){
  uint8_t bank=liveBank[ch]^1;parameters[ch][bank]=makeParams(ch);
  // Inactive bank is complete before its single-byte publication.
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){liveBank[ch]=bank;if(reset)pendingReset|=1U<<ch;}
}
static void trigger(uint8_t mask,uint32_t now){
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){pendingHits|=mask;}
  for(uint8_t ch=0;ch<2;ch++)if(mask&(1U<<ch)){ledUntil[ch]=now+25;ledOn[ch]=true;}
}
static void resetPickup(){
  uint16_t pots[3]={pot1,pot2,pot3};
  for(uint8_t i=0;i<3;i++){picked[i]=false;previousPot[i]=pots[i];}
}
static void defaults(){
  const uint16_t sound[8][3]={{220,450,80},{350,450,160},{400,300,450},{500,430,550},
    {500,420,300},{350,520,520},{600,260,680},{600,500,680}};
  for(uint8_t ch=0;ch<2;ch++)for(uint8_t e=0;e<8;e++)for(uint8_t i=0;i<6;i++)
    settings[ch][e][i]=i<3?sound[e][i]:i==3?(e>=CLOSED_HAT?720:512):0;
  engine[0]=KICK;engine[1]=SNARE;
}
static uint16_t kitCrc(const uint8_t *r){
  uint16_t crc=0xFFFF;
  for(uint16_t i=0;i<KIT_BYTES-3;i++){
    crc^=(uint16_t)r[i]<<8;
    for(uint8_t b=0;b<8;b++)crc=(crc&0x8000)?(uint16_t)((crc<<1)^0x1021):(uint16_t)(crc<<1);
  }
  return crc;
}
static bool kitValid(const uint8_t *r){
  if(r[0]!='D'||r[1]!='L'||r[2]!='A'||r[3]!='B'||r[4]!=1||r[7]>7||r[8]>7||r[KIT_BYTES-1]!=0xA5)return false;
  uint16_t crc=kitCrc(r);if(r[KIT_BYTES-3]!=(uint8_t)crc||r[KIT_BYTES-2]!=(uint8_t)(crc>>8))return false;
  for(uint16_t i=9;i<201;i+=2)if(((uint16_t)r[i]|((uint16_t)r[i+1]<<8))>1023)return false;
  return true;
}
static void loadKit(){
  defaults();uint8_t best=255;uint16_t sequence=0;
  for(uint8_t bank=0;bank<2;bank++){
    for(uint16_t i=0;i<KIT_BYTES;i++)kitRecord[i]=EEPROM.read(KIT_BASE+bank*KIT_STRIDE+i);
    if(!kitValid(kitRecord))continue;
    uint16_t seq=(uint16_t)kitRecord[5]|((uint16_t)kitRecord[6]<<8);
    if(best==255||(int16_t)(seq-sequence)>0){best=bank;sequence=seq;}
  }
  kitBank=best==255?1:best;kitSequence=sequence;if(best==255)return;
  for(uint16_t i=0;i<KIT_BYTES;i++)kitRecord[i]=EEPROM.read(KIT_BASE+best*KIT_STRIDE+i);
  engine[0]=kitRecord[7];engine[1]=kitRecord[8];uint16_t pos=9;
  for(uint8_t ch=0;ch<2;ch++)for(uint8_t e=0;e<8;e++)for(uint8_t i=0;i<6;i++){
    settings[ch][e][i]=(uint16_t)kitRecord[pos]|((uint16_t)kitRecord[pos+1]<<8);pos+=2;
  }
}
static void startKitSave(){
  if(kitWriteStep>=0)return;
  kitRecord[0]='D';kitRecord[1]='L';kitRecord[2]='A';kitRecord[3]='B';kitRecord[4]=1;
  uint16_t seq=kitSequence+1;kitRecord[5]=seq;kitRecord[6]=seq>>8;
  kitRecord[7]=engine[0];kitRecord[8]=engine[1];uint16_t pos=9;
  for(uint8_t ch=0;ch<2;ch++)for(uint8_t e=0;e<8;e++)for(uint8_t i=0;i<6;i++){
    uint16_t v=settings[ch][e][i];kitRecord[pos++]=v;kitRecord[pos++]=v>>8;
  }
  uint16_t crc=kitCrc(kitRecord);kitRecord[201]=crc;kitRecord[202]=crc>>8;kitRecord[203]=0xA5;
  kitWriteBank=kitBank^1;kitWriteStep=0;
}
static void serviceKitSave(uint32_t now){
  if(kitWriteStep<0||!eeprom_is_ready())return;
  uint16_t base=KIT_BASE+kitWriteBank*KIT_STRIDE;
  if(!kitWriteStep)EEPROM.update(base+203,0);
  else if(kitWriteStep<=203)EEPROM.update(base+kitWriteStep-1,kitRecord[kitWriteStep-1]);
  else if(kitWriteStep==204)EEPROM.update(base+203,0xA5);
  else {
    bool valid=true;for(uint16_t i=0;i<KIT_BYTES;i++)if(EEPROM.read(base+i)!=kitRecord[i])valid=false;
    notice=valid?1:2;noticeAt=now;
    if(valid){kitBank=kitWriteBank;kitSequence++;}kitWriteStep=-1;return;
  }
  kitWriteStep++;
}
static void scanButton(Button &b,uint32_t now){
  b.rising=b.falling=false;bool raw=digitalRead(b.pin)==HIGH;
  if(raw!=b.raw){b.raw=raw;b.rawSinceMs=now;}
  if(raw!=b.stable&&elapsedMs(now,b.rawSinceMs,b.debounceMs)){
    b.stable=raw;b.rising=raw;b.falling=!raw;
  }
}
static void readPots(uint32_t now){
  if(!elapsedMs(now,lastPotsMs,12))return;lastPotsMs=now;
  pot1=analogRead(PIN_POT_1);pot2=analogRead(PIN_POT_2);pot3=analogRead(PIN_POT_3);
}
static void editPots(){
  uint16_t pots[3]={pot1,pot2,pot3};bool changed=false;
  for(uint8_t i=0;i<3;i++){
    uint16_t &value=settings[selectedVoice][engine[selectedVoice]][page*3+i];
    int16_t error=(int16_t)pots[i]-value,previous=(int16_t)previousPot[i]-value;
    if(!picked[i]&&(abs(error)<=14||(error<0&&previous>0)||(error>0&&previous<0)))picked[i]=true;
    if(picked[i]&&abs(error)>=3){value=pots[i];changed=true;}
    previousPot[i]=pots[i];
  }
  if(changed)publish(selectedVoice);
}
static void serviceUI(uint32_t now){
  if(ignoreButtonsUntilRelease){if(!modeBtn.stable&&!manualBtn.stable)ignoreButtonsUntilRelease=false;return;}
  if(modeBtn.raw&&manualBtn.rising&&!modeLongHandled&&!chordLocked){
    page^=1;resetPickup();chordLocked=true;modePressArmed=manualArmed=false;
  }
  if(chordLocked){if(!modeBtn.raw&&!manualBtn.raw&&!modeBtn.stable&&!manualBtn.stable){chordLocked=false;modeLongHandled=false;}return;}
  if(modeBtn.rising){modePressArmed=true;modeLongHandled=false;modePressedMs=now;}
  if(modePressArmed&&modeBtn.stable&&!modeLongHandled&&elapsedMs(now,modePressedMs,650)){
    selectedVoice^=1;page=0;resetPickup();modeLongHandled=true;
  }
  if(modeBtn.falling){
    if(modePressArmed&&!modeLongHandled){engine[selectedVoice]=(engine[selectedVoice]+1)&7;page=0;publish(selectedVoice,true);resetPickup();}
    modePressArmed=false;modeLongHandled=false;
  }
  if(manualBtn.rising&&!modeBtn.raw){trigger(1U<<selectedVoice,now);manualArmed=true;manualPressedMs=now;}
  if(manualArmed&&manualBtn.stable&&!modeBtn.raw&&elapsedMs(now,manualPressedMs,2000)){
    manualArmed=false;startKitSave();
  }
  if(manualBtn.falling)manualArmed=false;
}
static void display(uint32_t now){
  uint8_t bits=(engine[selectedVoice]+1)&7;
  bool voiceLed=selectedVoice;
  if(page)voiceLed=selectedVoice?(now%500>=80):(now%500<80);
  if(voiceLed)bits|=8;
  if(kitWriteStep>=0)bits=(now/100)&1?15:0;
  else if(notice&&!elapsedMs(now,noticeAt,700))bits=notice==1?15:((now/60)&1?15:0);
  else notice=0;
  for(uint8_t i=0;i<4;i++)digitalWrite(PIN_LED_MODE_1+i,(bits&(1U<<i))?HIGH:LOW);
  for(uint8_t ch=0;ch<2;ch++)if(ledOn[ch]&&(int32_t)(now-ledUntil[ch])>=0)ledOn[ch]=false;
  digitalWrite(PIN_INPUT_LED,ledOn[0]?HIGH:LOW);digitalWrite(PIN_IO_LED,ledOn[1]?HIGH:LOW);
}
static void startAudio(){
  stopAudio();
  for(uint8_t ch=0;ch<2;ch++){
    voices[ch]=Voice();voices[ch].rng=ch?0xBEEF:0xACE1;
    audioZero[ch]=DAC_ZERO_CODE[ch];audioPos[ch]=(DAC_ZERO_CODE[ch]-DAC_PLUS5_CODE[ch])/2;
    audioNeg[ch]=(DAC_MINUS5_CODE[ch]-DAC_ZERO_CODE[ch])/2;publish(ch);
  }
  audioFrames=hitCount[0]=hitCount[1]=0;resetPickup();
  cvGate=analogRead(PIN_CV_INPUT)>=388;ioGate=digitalRead(PIN_OUT3)==HIGH;lastCvPollUs=micros();
  SPI.beginTransaction(SPISettings(8000000UL,MSBFIRST,SPI_MODE0));SPI.endTransaction();
  writeFrame(audioZero[0],audioZero[1]);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    audioActive=true;
#if defined(__AVR__)
    TCCR1A=0;TCCR1B=0;TCNT1=0;OCR1A=F_CPU/AUDIO_RATE-1;
    TIFR1=_BV(OCF1A);TIMSK1|=_BV(OCIE1A);TCCR1B=_BV(WGM12)|_BV(CS10);
#endif
  }
}

// Hidden boot calibration. This record format is independent of preset EEPROM.
const uint16_t CAL_BASE=256;
const uint8_t CAL_BYTES=22,CAL_STRIDE=24;
const uint16_t CAL_FACTORY[3][2]={{2048,2048},{87,87},{4008,4008}};
const uint16_t CAL_CENTER_LOW=442,CAL_CENTER_HIGH=582;
bool calibrationActive=false,calBootPending=false,calNeedRelease=false;
bool calReady[2]={false,false},calSaved=false,calError=false;
bool calManualArmed=false,calModeArmed=false,calModeLong=false;
bool calResetTracked=false;
uint8_t calStage=0,calBank=1,calWriteBank=0;
uint16_t calSequence=0,calWork[3][2],calAnchor[3][2];
uint8_t calRecord[CAL_BYTES];
int8_t calWriteStep=-1;
uint32_t calBootStart=0,calManualSince=0,calModeSince=0;
uint32_t calNext[2]={0,0},calSavedAt=0,calReportAt=0;
uint32_t calResetSince=0;
bool calHasStored=false;

static uint16_t calCrc(const uint8_t *p) {
  uint16_t c=0xFFFF;
  for(uint8_t i=0;i<19;i++) {
    c^=(uint16_t)p[i]<<8;
    for(uint8_t b=0;b<8;b++)c=(c&0x8000)?(uint16_t)((c<<1)^0x1021):(uint16_t)(c<<1);
  }
  return c;
}
static bool calValid(const uint8_t *r) {
  if(r[0]!='A'||r[1]!='C'||r[2]!='A'||r[3]!='L'||r[4]!=1||r[21]!=0xA5)return false;
  uint16_t crc=calCrc(r);
  if(r[19]!=(uint8_t)crc||r[20]!=(uint8_t)(crc>>8))return false;
  uint16_t p[3][2];
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++) {
    uint8_t i=7+s*4+ch*2;p[s][ch]=(uint16_t)r[i]|((uint16_t)r[i+1]<<8);
    if(p[s][ch]>4095)return false;
  }
  for(uint8_t ch=0;ch<2;ch++)
    if(p[0][ch]<1024||p[0][ch]>3072||p[1][ch]+512>=p[0][ch]||p[0][ch]+512>=p[2][ch])return false;
  return true;
}
static uint16_t calActivePoint(uint8_t stage,uint8_t ch) {
  return stage==0?DAC_ZERO_CODE[ch]:stage==1?DAC_PLUS5_CODE[ch]:DAC_MINUS5_CODE[ch];
}
static void calApplyRecord(const uint8_t *r) {
  for(uint8_t ch=0;ch<2;ch++) {
    DAC_ZERO_CODE[ch]=(uint16_t)r[7+ch*2]|((uint16_t)r[8+ch*2]<<8);
    DAC_PLUS5_CODE[ch]=(uint16_t)r[11+ch*2]|((uint16_t)r[12+ch*2]<<8);
    DAC_MINUS5_CODE[ch]=(uint16_t)r[15+ch*2]|((uint16_t)r[16+ch*2]<<8);
  }
}
static void loadCalibration() {
  for(uint8_t ch=0;ch<2;ch++) {
    DAC_ZERO_CODE[ch]=CAL_FACTORY[0][ch];DAC_PLUS5_CODE[ch]=CAL_FACTORY[1][ch];DAC_MINUS5_CODE[ch]=CAL_FACTORY[2][ch];
  }
  uint8_t r[2][CAL_BYTES];
  for(uint8_t b=0;b<2;b++)for(uint8_t i=0;i<CAL_BYTES;i++)r[b][i]=EEPROM.read(CAL_BASE+b*CAL_STRIDE+i);
  bool a=calValid(r[0]),b=calValid(r[1]);
  calHasStored=a||b;calBank=1;calSequence=0;
  if(!calHasStored)return;
  uint16_t sa=(uint16_t)r[0][5]|((uint16_t)r[0][6]<<8);
  uint16_t sb=(uint16_t)r[1][5]|((uint16_t)r[1][6]<<8);
  calBank=!a?1:!b?0:((int16_t)(sb-sa)>0?1:0);
  calSequence=calBank?sb:sa;calApplyRecord(r[calBank]);
}
static void calSelectStage(uint8_t next,uint32_t now) {
  calStage=next;calReady[0]=calReady[1]=false;
  calNext[0]=calNext[1]=now;calManualArmed=false;calModeArmed=false;
}
static void startCalibration(uint32_t now) {
  stopAudio();
  calibrationActive=true;calNeedRelease=true;calSaved=false;calError=false;
  calWriteStep=-1;calModeLong=false;calResetTracked=false;
  pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);pinMode(A5,INPUT);
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++)calWork[s][ch]=calAnchor[s][ch]=calActivePoint(s,ch);
  calSelectStage(0,now);calReportAt=now;
  Serial.begin(115200);
  Serial.println(F("CAL: release buttons. Center P1/P2; CW raises volts."));
}
static void calExit() {
  calibrationActive=calBootPending=calNeedRelease=false;
  calManualArmed=calModeArmed=modePressArmed=false;
  ignoreButtonsUntilRelease=true;
  Serial.end();startAudio();
}
static void serviceCalBoot(uint32_t now) {
  if(digitalRead(PIN_MODE_BUTTON)!=HIGH||digitalRead(PIN_MANUAL_BUTTON)!=HIGH) {
    calBootPending=false;ignoreButtonsUntilRelease=true;startAudio();return;
  }
  if(elapsedMs(now,calBootStart,100)) {calBootPending=false;startCalibration(now);}
}
static void calWriteOutput() {
  uint8_t s=calStage<3?calStage:0;
  uint16_t a=calWork[s][0],b=calWork[s][1];
  if(dacLastA!=a){dacWriteRaw(0,a);dacLastA=a;}
  if(dacLastB!=b){dacWriteRaw(1,b);dacLastB=b;}
  latchDac();
}
static void calBuildRecord() {
  calRecord[0]='A';calRecord[1]='C';calRecord[2]='A';calRecord[3]='L';calRecord[4]=1;
  uint16_t seq=calSequence+1;calRecord[5]=(uint8_t)seq;calRecord[6]=seq>>8;
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++) {
    uint8_t p=7+s*4+ch*2;calRecord[p]=(uint8_t)calWork[s][ch];calRecord[p+1]=calWork[s][ch]>>8;
  }
  uint16_t crc=calCrc(calRecord);calRecord[19]=(uint8_t)crc;calRecord[20]=crc>>8;calRecord[21]=0xA5;
}
static void calStartSave(uint32_t now) {
  calBuildRecord();calError=false;
  if(!calValid(calRecord)){calError=true;return;}
  bool equal=true;
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++)if(calWork[s][ch]!=calActivePoint(s,ch))equal=false;
  if(equal&&calHasStored){calSaved=true;calSavedAt=now;return;}
  calWriteBank=calBank^1;calWriteStep=0;
}
static void serviceCalSave(uint32_t now) {
  if(!eeprom_is_ready())return;
  uint16_t addr=CAL_BASE+calWriteBank*CAL_STRIDE;
  if(calWriteStep==0)EEPROM.update(addr+21,0);
  else if(calWriteStep<=21)EEPROM.update(addr+calWriteStep-1,calRecord[calWriteStep-1]);
  else if(calWriteStep==22)EEPROM.update(addr+21,0xA5);
  else {
    uint8_t verify[CAL_BYTES];for(uint8_t i=0;i<CAL_BYTES;i++)verify[i]=EEPROM.read(addr+i);
    calWriteStep=-1;
    if(!calValid(verify)){calError=true;return;}
    for(uint8_t i=0;i<CAL_BYTES;i++)if(verify[i]!=calRecord[i]){calError=true;return;}
    calBank=calWriteBank;calSequence=(uint16_t)verify[5]|((uint16_t)verify[6]<<8);
    calApplyRecord(verify);calHasStored=true;calSaved=true;calSavedAt=now;
    Serial.print(F("SAVED ZERO="));Serial.print(DAC_ZERO_CODE[0]);Serial.print(',');Serial.print(DAC_ZERO_CODE[1]);
    Serial.print(F(" PLUS5="));Serial.print(DAC_PLUS5_CODE[0]);Serial.print(',');Serial.print(DAC_PLUS5_CODE[1]);
    Serial.print(F(" MINUS5="));Serial.print(DAC_MINUS5_CODE[0]);Serial.print(',');Serial.println(DAC_MINUS5_CODE[1]);
    return;
  }
  calWriteStep++;
}
static void calAdjust(uint8_t ch,uint16_t pot,uint32_t now) {
  bool centered=pot>=CAL_CENTER_LOW&&pot<=CAL_CENTER_HIGH;
  if(!calReady[ch]) {if(centered){calReady[ch]=true;calNext[ch]=now;}return;}
  if(centered){calNext[ch]=now;return;}
  uint16_t distance=pot<CAL_CENTER_LOW?CAL_CENTER_LOW-pot:pot-CAL_CENTER_HIGH;
  uint16_t interval=300-(uint32_t)distance*260/442;
  if(!elapsedMs(now,calNext[ch],interval))return;
  calNext[ch]=now;
  int16_t code=(int16_t)calWork[calStage][ch]+(pot>CAL_CENTER_HIGH?-1:1);
  int16_t lo=(int16_t)calAnchor[calStage][ch]-256;
  int16_t hi=(int16_t)calAnchor[calStage][ch]+256;
  if(lo<0)lo=0;if(hi>4095)hi=4095;
  if(code<lo)code=lo;if(code>hi)code=hi;
  calWork[calStage][ch]=(uint16_t)code;
}
static void serviceCalibration(uint32_t now) {
  calWriteOutput();
  uint8_t bits=calSaved?15:(calWriteStep>=0||calError)?(((now/100)&1)?15:0):
    (calStage==3?(((now/250)&1)?8:0):(1U<<calStage));
  for(uint8_t i=0;i<4;i++)digitalWrite(PIN_LED_MODE_1+i,(bits&(1U<<i))?HIGH:LOW);
  digitalWrite(PIN_INPUT_LED,calReady[0]?HIGH:LOW);digitalWrite(PIN_IO_LED,calReady[1]?HIGH:LOW);
  if(calSaved){if(elapsedMs(now,calSavedAt,800))calExit();return;}
  if(calWriteStep>=0){serviceCalSave(now);return;}
  if(calNeedRelease) {if(!manualBtn.stable&&!modeBtn.stable)calNeedRelease=false;return;}
  if(modeBtn.stable&&manualBtn.stable) {
    if(!calResetTracked){calResetTracked=true;calResetSince=now;calManualArmed=calModeArmed=false;}
    if(elapsedMs(now,calResetSince,3000)) {
      for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++)calWork[s][ch]=calAnchor[s][ch]=CAL_FACTORY[s][ch];
      calResetTracked=false;calNeedRelease=true;calError=false;calSelectStage(0,now);
      Serial.println(F("CAL factory defaults: confirm all points to save"));
    }
    return;
  }
  if(calResetTracked) {
    if(!modeBtn.stable&&!manualBtn.stable){calResetTracked=false;calManualArmed=calModeArmed=false;}
    return;
  }
  if(modeBtn.rising){calModeArmed=true;calModeLong=false;calModeSince=now;}
  if(calModeArmed&&modeBtn.stable&&!calModeLong&&elapsedMs(now,calModeSince,1200)) {
    calModeLong=true;Serial.println(F("CAL cancelled"));calExit();return;
  }
  if(modeBtn.falling&&calModeArmed){calModeArmed=false;if(!calModeLong){calError=false;calSelectStage(calStage>0?calStage-1:0,now);}return;}
  if(manualBtn.rising){calManualArmed=true;calManualSince=now;}
  if(calStage==3&&calManualArmed&&manualBtn.stable&&elapsedMs(now,calManualSince,1000)) {
    calManualArmed=false;calStartSave(now);return;
  }
  if(manualBtn.falling&&calManualArmed) {
    calManualArmed=false;
    if(calStage<3&&calReady[0]&&calReady[1]&&
       pot1>=CAL_CENTER_LOW&&pot1<=CAL_CENTER_HIGH&&pot2>=CAL_CENTER_LOW&&pot2<=CAL_CENTER_HIGH)
      calSelectStage(calStage+1,now);
    return;
  }
  if(calStage<3) {
    calAdjust(0,pot1,now);calAdjust(1,pot2,now);
    if(elapsedMs(now,calReportAt,500)) {
      calReportAt=now;Serial.print(F("CAL step="));Serial.print(calStage);
      Serial.print(F(" A="));Serial.print(calWork[calStage][0]);Serial.print(F(" B="));Serial.println(calWork[calStage][1]);
    }
  }
}


void setup(){
  stopAudio();
  pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);pinMode(A5,INPUT);
  pinMode(PIN_MODE_BUTTON,INPUT);pinMode(PIN_MANUAL_BUTTON,INPUT);pinMode(PIN_CV_INPUT,INPUT);
  pinMode(PIN_INPUT_LED,OUTPUT);pinMode(PIN_IO_LED,OUTPUT);
  for(uint8_t i=0;i<4;i++)pinMode(PIN_LED_MODE_1+i,OUTPUT);
  pinMode(PIN_DAC_CS,OUTPUT);digitalWrite(PIN_DAC_CS,HIGH);SPI.begin();
  digitalWrite(PIN_DAC_LDAC,HIGH);pinMode(PIN_DAC_LDAC,OUTPUT);
  SPI.beginTransaction(SPISettings(8000000UL,MSBFIRST,SPI_MODE0));SPI.endTransaction();
  manualBtn.raw=manualBtn.stable=digitalRead(PIN_MANUAL_BUTTON)==HIGH;
  modeBtn.raw=modeBtn.stable=digitalRead(PIN_MODE_BUTTON)==HIGH;
  uint32_t now=millis();modeBtn.rawSinceMs=manualBtn.rawSinceMs=now;
  pot1=analogRead(PIN_POT_1);pot2=analogRead(PIN_POT_2);pot3=analogRead(PIN_POT_3);
  loadCalibration();loadKit();
  if(modeBtn.stable&&manualBtn.stable){
    calBootPending=true;calBootStart=now;dacWriteRaw(0,DAC_ZERO_CODE[0]);dacWriteRaw(1,DAC_ZERO_CODE[1]);latchDac();
  }else {ignoreButtonsUntilRelease=modeBtn.stable||manualBtn.stable;startAudio();}
}
void loop(){
  uint32_t now=millis();scanButton(manualBtn,now);scanButton(modeBtn,now);readPots(now);
  if(calBootPending){serviceCalBoot(now);return;}
  if(calibrationActive){serviceCalibration(now);return;}
  uint8_t mask=0;
  uint32_t us=micros();
  if((uint32_t)(us-lastCvPollUs)>=550){
    lastCvPollUs=us;uint16_t cv=analogRead(PIN_CV_INPUT);
    bool level=cvGate?cv>225:cv>=388;if(level&&!cvGate)mask|=1;cvGate=level;
  }
  bool io=digitalRead(PIN_OUT3)==HIGH;if(io&&!ioGate)mask|=2;ioGate=io;
  if(mask)trigger(mask,now);
  serviceUI(now);editPots();serviceKitSave(now);display(now);
}
