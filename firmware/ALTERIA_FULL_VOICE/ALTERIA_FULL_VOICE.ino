/*
  ALTERIA FULL VOICE 1.0.0 - 2026-10-31
  Nano ATmega328P, 16 MHz, REV5-2, MCP4822. No Mozzi dependency.
  CV IN: nominal 1 V/oct. I/O: ALWAYS INPUT, rising edge starts AD.
  OUT1: filtered/VCA audio about +/-2.5 V. OUT2: AD 0..+5 V.
  MODE short: next page. MANUAL short: previous page (on release).
  MANUAL hold 500 ms: audition one note, no page change.
  MODE hold 1.2 s: save patch, no page change. Pots use soft pickup.
  Pages, LED1..4 one-hot:
    OSC: waveform (sine/triangle/saw/pulse), pulse width, fine tune.
    FILTER: LP cutoff, resonance, positive envelope amount.
    VCA: oscillator level, envelope amount (0 = drone), drive.
    ENV: Attack, Decay, continuous log/linear/exp curve. NO sustain.
  Boot MODE+MANUAL: existing ACAL output calibration, audio stopped.
  Patch EEPROM 320..393; ACAL 256..301. Other firmware areas untouched.
  Input voltage limits and physical 1 V/oct tracking require measurement.
*/
#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#if defined(__AVR__) && (!defined(__AVR_ATmega328P__) || F_CPU != 16000000UL)
#error "ALTERIA FULL VOICE needs an ATmega328P at 16 MHz."
#endif
#define PIN_OUT3 2
#define PIN_INPUT_LED 3
#define PIN_MODE_BUTTON 4
#define PIN_IO_LED 5
#define PIN_LED_MODE_1 6
#define PIN_LED_MODE_2 7
#define PIN_LED_MODE_3 8
#define PIN_LED_MODE_4 9
#define PIN_DAC_CS 10
#define PIN_DAC_LDAC 12
#define PIN_POT_1 A0
#define PIN_POT_2 A1
#define PIN_CV_INPUT A2
#define PIN_MANUAL_BUTTON A3
#define PIN_POT_3 A6
const uint16_t VOICE_RATE=6400;
// Nominal ADC endpoints, NOT measured calibration of this module.
const int16_t CV_IN_ADC_ZERO=0,CV_IN_ADC_PLUS5=1023;
int16_t DAC_ZERO_CODE[2]={2048,2048},DAC_PLUS5_CODE[2]={87,87},DAC_MINUS5_CODE[2]={4008,4008};
uint16_t dacLastA=65535,dacLastB=65535;
uint16_t pot1=0,pot2=0,pot3=0;
struct Button {uint8_t pin; bool raw,stable,rising,falling; uint32_t rawSinceMs;};
Button manualBtn={PIN_MANUAL_BUTTON,false,false,false,false,0};
Button modeBtn={PIN_MODE_BUTTON,false,false,false,false,0};
enum EnvStage : uint8_t {ENV_IDLE,ENV_ATTACK,ENV_DECAY};
enum Page : uint8_t {PAGE_OSC,PAGE_FILTER,PAGE_VCA,PAGE_ENV};
static bool elapsedMs(uint32_t n,uint32_t s,uint32_t d){return (uint32_t)(n-s)>=d;}
static void startNormal();
static void scanButton(Button &b,uint32_t now){
  b.rising=b.falling=false;bool raw=digitalRead(b.pin)==HIGH;
  if(raw!=b.raw){b.raw=raw;b.rawSinceMs=now;}
  if(raw!=b.stable&&elapsedMs(now,b.rawSinceMs,25)){
    b.stable=raw;b.rising=raw;b.falling=!raw;
  }
}
static uint16_t normalizeCv(int raw){
  int32_t v=((int32_t)raw-CV_IN_ADC_ZERO)*4095L/(CV_IN_ADC_PLUS5-CV_IN_ADC_ZERO);
  return v<0?0:v>4095?4095:v;
}
static void dacWriteRaw(uint8_t ch,uint16_t value){
  uint16_t w=(ch?0x9000U:0x1000U)|(value&4095);
  digitalWrite(PIN_DAC_CS,LOW);SPI.transfer((uint8_t)(w>>8));SPI.transfer((uint8_t)w);digitalWrite(PIN_DAC_CS,HIGH);
}
static void latchDac(){digitalWrite(PIN_DAC_LDAC,LOW);
#if defined(__AVR__)
  __builtin_avr_delay_cycles(F_CPU/1000000UL);
#endif
  digitalWrite(PIN_DAC_LDAC,HIGH);
}

const int8_t OSC_TABLES[3][4][128] PROGMEM={
{
{0,5,9,14,19,23,28,32,36,41,45,49,53,57,60,64,67,70,73,76,79,81,84,86,88,89,91,92,93,94,95,95,95,95,95,94,93,92,91,89,88,86,84,81,79,76,73,70,67,64,60,57,53,49,45,41,36,32,28,23,19,14,9,5,0,-5,-9,-14,-19,-23,-28,-32,-36,-41,-45,-49,-53,-57,-60,-64,-67,-70,-73,-76,-79,-81,-84,-86,-88,-89,-91,-92,-93,-94,-95,-95,-95,-95,-95,-94,-93,-92,-91,-89,-88,-86,-84,-81,-79,-76,-73,-70,-67,-64,-60,-57,-53,-49,-45,-41,-36,-32,-28,-23,-19,-14,-9,-5},
{0,5,9,14,19,23,28,32,36,41,45,49,53,57,60,64,67,70,73,76,79,81,84,86,88,89,91,92,93,94,95,95,95,95,95,94,93,92,91,89,88,86,84,81,79,76,73,70,67,64,60,57,53,49,45,41,36,32,28,23,19,14,9,5,0,-5,-9,-14,-19,-23,-28,-32,-36,-41,-45,-49,-53,-57,-60,-64,-67,-70,-73,-76,-79,-81,-84,-86,-88,-89,-91,-92,-93,-94,-95,-95,-95,-95,-95,-94,-93,-92,-91,-89,-88,-86,-84,-81,-79,-76,-73,-70,-67,-64,-60,-57,-53,-49,-45,-41,-36,-32,-28,-23,-19,-14,-9,-5},
{0,5,9,14,19,23,28,32,36,41,45,49,53,57,60,64,67,70,73,76,79,81,84,86,88,89,91,92,93,94,95,95,95,95,95,94,93,92,91,89,88,86,84,81,79,76,73,70,67,64,60,57,53,49,45,41,36,32,28,23,19,14,9,5,0,-5,-9,-14,-19,-23,-28,-32,-36,-41,-45,-49,-53,-57,-60,-64,-67,-70,-73,-76,-79,-81,-84,-86,-88,-89,-91,-92,-93,-94,-95,-95,-95,-95,-95,-94,-93,-92,-91,-89,-88,-86,-84,-81,-79,-76,-73,-70,-67,-64,-60,-57,-53,-49,-45,-41,-36,-32,-28,-23,-19,-14,-9,-5},
{0,5,9,14,19,23,28,32,36,41,45,49,53,57,60,64,67,70,73,76,79,81,84,86,88,89,91,92,93,94,95,95,95,95,95,94,93,92,91,89,88,86,84,81,79,76,73,70,67,64,60,57,53,49,45,41,36,32,28,23,19,14,9,5,0,-5,-9,-14,-19,-23,-28,-32,-36,-41,-45,-49,-53,-57,-60,-64,-67,-70,-73,-76,-79,-81,-84,-86,-88,-89,-91,-92,-93,-94,-95,-95,-95,-95,-95,-94,-93,-92,-91,-89,-88,-86,-84,-81,-79,-76,-73,-70,-67,-64,-60,-57,-53,-49,-45,-41,-36,-32,-28,-23,-19,-14,-9,-5}
},
{
{0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48,51,54,57,60,63,66,69,72,75,78,81,84,87,90,94,95,94,90,87,84,81,78,75,72,69,66,63,60,57,54,51,48,45,42,39,36,33,30,27,24,21,18,15,12,9,6,3,0,-3,-6,-9,-12,-15,-18,-21,-24,-27,-30,-33,-36,-39,-42,-45,-48,-51,-54,-57,-60,-63,-66,-69,-72,-75,-78,-81,-84,-87,-90,-94,-95,-94,-90,-87,-84,-81,-78,-75,-72,-69,-66,-63,-60,-57,-54,-51,-48,-45,-42,-39,-36,-33,-30,-27,-24,-21,-18,-15,-12,-9,-6,-3},
{0,3,6,9,12,15,18,21,24,27,30,33,37,40,43,46,49,52,55,58,61,64,67,70,73,76,79,82,86,89,92,94,95,94,92,89,86,82,79,76,73,70,67,64,61,58,55,52,49,46,43,40,37,33,30,27,24,21,18,15,12,9,6,3,0,-3,-6,-9,-12,-15,-18,-21,-24,-27,-30,-33,-37,-40,-43,-46,-49,-52,-55,-58,-61,-64,-67,-70,-73,-76,-79,-82,-86,-89,-92,-94,-95,-94,-92,-89,-86,-82,-79,-76,-73,-70,-67,-64,-61,-58,-55,-52,-49,-46,-43,-40,-37,-33,-30,-27,-24,-21,-18,-15,-12,-9,-6,-3},
{0,3,6,9,12,15,18,22,25,28,32,35,38,41,44,47,50,53,55,58,62,65,68,72,76,79,83,86,89,92,93,95,95,95,93,92,89,86,83,79,76,72,68,65,62,58,55,53,50,47,44,41,38,35,32,28,25,22,18,15,12,9,6,3,0,-3,-6,-9,-12,-15,-18,-22,-25,-28,-32,-35,-38,-41,-44,-47,-50,-53,-55,-58,-62,-65,-68,-72,-76,-79,-83,-86,-89,-92,-93,-95,-95,-95,-93,-92,-89,-86,-83,-79,-76,-72,-68,-65,-62,-58,-55,-53,-50,-47,-44,-41,-38,-35,-32,-28,-25,-22,-18,-15,-12,-9,-6,-3},
{0,5,9,14,19,23,28,32,36,41,45,49,53,57,60,64,67,70,73,76,79,81,84,86,88,89,91,92,93,94,95,95,95,95,95,94,93,92,91,89,88,86,84,81,79,76,73,70,67,64,60,57,53,49,45,41,36,32,28,23,19,14,9,5,0,-5,-9,-14,-19,-23,-28,-32,-36,-41,-45,-49,-53,-57,-60,-64,-67,-70,-73,-76,-79,-81,-84,-86,-88,-89,-91,-92,-93,-94,-95,-95,-95,-95,-95,-94,-93,-92,-91,-89,-88,-86,-84,-81,-79,-76,-73,-70,-67,-64,-60,-57,-53,-49,-45,-41,-36,-32,-28,-23,-19,-14,-9,-5}
},
{
{0,72,95,80,70,76,80,73,68,72,73,68,65,67,67,63,60,62,61,57,55,56,56,52,51,51,50,47,46,46,45,42,41,41,40,37,36,36,34,32,30,31,29,26,25,25,24,21,20,20,18,16,15,15,13,11,10,10,8,6,5,5,3,0,0,0,-3,-5,-5,-6,-8,-10,-10,-11,-13,-15,-15,-16,-18,-20,-20,-21,-24,-25,-25,-26,-29,-31,-30,-32,-34,-36,-36,-37,-40,-41,-41,-42,-45,-46,-46,-47,-50,-51,-51,-52,-56,-56,-55,-57,-61,-62,-60,-63,-67,-67,-65,-68,-73,-72,-68,-73,-80,-76,-70,-80,-95,-72},
{0,41,73,91,95,88,78,69,66,69,73,75,75,71,65,61,60,61,63,63,62,58,54,52,51,51,52,52,50,47,44,42,41,41,42,41,39,36,33,31,31,31,31,30,28,25,22,21,21,21,20,19,16,14,12,11,10,10,10,8,5,3,1,0,0,0,-1,-3,-5,-8,-10,-10,-10,-11,-12,-14,-16,-19,-20,-21,-21,-21,-22,-25,-28,-30,-31,-31,-31,-31,-33,-36,-39,-41,-42,-41,-41,-42,-44,-47,-50,-52,-52,-51,-51,-52,-54,-58,-62,-63,-63,-61,-60,-61,-65,-71,-75,-75,-73,-69,-66,-69,-78,-88,-95,-91,-73,-41},
{0,22,43,61,76,86,93,95,94,90,84,77,71,66,62,59,59,59,61,62,64,65,65,63,61,58,54,50,47,44,42,41,41,41,42,42,42,42,40,38,36,33,30,27,25,23,22,21,21,21,21,21,20,19,17,15,12,9,6,4,2,1,0,0,0,0,0,-1,-2,-4,-6,-9,-12,-15,-17,-19,-20,-21,-21,-21,-21,-21,-22,-23,-25,-27,-30,-33,-36,-38,-40,-42,-42,-42,-42,-41,-41,-41,-42,-44,-47,-50,-54,-58,-61,-63,-65,-65,-64,-62,-61,-59,-59,-59,-62,-66,-71,-77,-84,-90,-94,-95,-93,-86,-76,-61,-43,-22},
{0,7,14,21,28,35,42,48,54,60,65,70,74,79,82,86,88,91,92,94,95,95,95,94,93,92,90,88,86,83,80,77,73,69,66,62,58,54,50,46,42,38,34,30,27,24,21,18,15,13,11,9,7,5,4,3,2,1,1,1,0,0,0,0,0,0,0,0,0,-1,-1,-1,-2,-3,-4,-5,-7,-9,-11,-13,-15,-18,-21,-24,-27,-30,-34,-38,-42,-46,-50,-54,-58,-62,-66,-69,-73,-77,-80,-83,-86,-88,-90,-92,-93,-94,-95,-95,-95,-94,-92,-91,-88,-86,-82,-79,-74,-70,-65,-60,-54,-48,-42,-35,-28,-21,-14,-7}
}
};
const uint32_t VOICE_PITCH[13] PROGMEM={21946743UL,23251764UL,24634386UL,26099223UL,27651164UL,29295387UL,31037382UL,32882961UL,34838283UL,36909875UL,39104651UL,41429934UL,43893486UL};
const uint8_t CUTOFF_TABLE[33] PROGMEM={5,6,6,7,8,9,10,11,12,14,15,17,19,22,24,27,30,34,38,43,48,54,60,67,75,84,94,106,118,133,148,166,186};

// Audio state: the ISR owns phases, filter states and the output DACs.
volatile bool voiceActive=false;
volatile uint8_t targetWave=2,targetWidth=128,targetCutoff=40,targetLevel=100,targetEnvAmount=128,targetDrive=32;
volatile uint16_t targetDamping=396,targetEnv=0,voiceFrames=0,voiceHits=0;
volatile uint32_t voiceIncrement=21946743UL;
volatile uint8_t voiceBand=0;
const int8_t * volatile bandTables[3]={OSC_TABLES[0][0],OSC_TABLES[1][0],OSC_TABLES[2][0]};
uint32_t voicePhase=0;
uint16_t voiceEnv=0;
int16_t voiceLow=0,voiceBandState=0;
uint8_t waveNow=2,waveFrom=2,waveFade=32,cutoffNow=40,levelNow=100,envAmountNow=128,driveNow=32,widthNow=128;
uint16_t voiceZeroA=2048,voiceZeroB=2048,voicePos=980,voiceNeg=980,voiceEnvSpan=1961;
#if !defined(__AVR__)
uint16_t voiceWritten[2];
#endif
static int16_t limit16(int16_t v,int16_t limit){return v>limit?limit:v<-limit?-limit:v;}
static uint8_t approach8(uint8_t value,uint8_t target){return value<target?value+1:value>target?value-1:value;}
static int16_t tableSample(uint8_t wave,uint32_t phase){
  uint16_t top=phase>>16;uint8_t i=(uint8_t)(top>>8)>>1,frac=top>>1;
  const int8_t *table=bandTables[wave];
  int16_t a=(int8_t)pgm_read_byte(table+i);
  int16_t d=(int8_t)pgm_read_byte(table+((i+1)&127))-a;
  uint16_t amount=((uint16_t)(d<0?-d:d)*frac)>>8;
  return a+(d<0?-(int16_t)amount:(int16_t)amount);
}
static int16_t oscillator(uint8_t wave,uint32_t phase){
  if(wave<3)return tableSample(wave,phase);
  // Difference of two band-limited saws, rather than a raw comparator PWM.
  return (tableSample(2,phase)-tableSample(2,phase+((uint32_t)widthNow<<24)))>>1;
}
static uint16_t audioCode(int16_t sample){
  uint16_t span=sample>=0?voicePos:voiceNeg;uint8_t mag=sample>=0?sample:-sample;
  uint16_t delta=mag*(span>>7)+((mag*(span&127))>>7);
  return sample>=0?voiceZeroA-delta:voiceZeroA+delta;
}
static void voiceWrite(uint16_t a,uint16_t b){
#if defined(__AVR__)
  // Preserve the GPIO / SPI.transfer / LDAC path used by the working drums.
  dacWriteRaw(0,a);dacWriteRaw(1,b);latchDac();
#else
  voiceWritten[0]=a;voiceWritten[1]=b;
#endif
  voiceFrames++;
}
static void voiceAudioTick(){
  voicePhase+=voiceIncrement;
  switch(voiceFrames&3){
  case 0:{
    uint16_t goal=targetEnv;
    if(goal>voiceEnv){uint16_t step=(goal-voiceEnv)>>2;voiceEnv+=step?step:1;}
    else if(goal<voiceEnv){uint16_t step=(voiceEnv-goal)>>2;voiceEnv-=step?step:1;}
    cutoffNow=approach8(cutoffNow,targetCutoff);break;
  }
  case 1:
    levelNow=approach8(levelNow,targetLevel);envAmountNow=approach8(envAmountNow,targetEnvAmount);break;
  case 2:
    driveNow=approach8(driveNow,targetDrive);widthNow=approach8(widthNow,targetWidth);break;
  default:
    if(waveFade==32&&targetWave!=waveNow){waveFrom=waveNow;waveNow=targetWave;waveFade=0;}
    break;
  }
  uint8_t amp=128-envAmountNow+(((uint16_t)(voiceEnv>>8)*envAmountNow)>>7);
  int16_t sample=0;
  if(amp&&levelNow){
    int16_t wave=oscillator(waveNow,voicePhase);
    if(waveFade<32){
      int16_t old=oscillator(waveFrom,voicePhase);
      wave=old+(((wave-old)*(int16_t)waveFade)>>5);waveFade++;
    }
    int16_t input=limit16(((wave*levelNow)>>7)*(int16_t)driveNow,3040);
    uint8_t f=cutoffNow;uint16_t damping=targetDamping;
    for(uint8_t i=0;i<2;i++){
      voiceLow=limit16(voiceLow+(((int32_t)voiceBandState*f)>>8),8192);
      int16_t feedback=((int32_t)voiceBandState*damping)>>8;
      int16_t high=limit16(input-voiceLow-feedback,16000);
      voiceBandState=limit16(voiceBandState+(((int32_t)high*f)>>8),8192);
    }
    sample=(limit16(voiceLow>>5,127)*(int16_t)amp)>>7;
  }else {voiceLow=voiceBandState=0;waveFade=32;waveNow=targetWave;}
  uint16_t envCode=voiceZeroB-(((uint32_t)voiceEnv*voiceEnvSpan)>>15);
  voiceWrite(audioCode(sample),envCode);
}
#if defined(__AVR__)
ISR(TIMER1_COMPA_vect){voiceAudioTick();}
#endif
static void voiceStop(){
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
#if defined(__AVR__)
    TIMSK1&=(uint8_t)~_BV(OCIE1A);TCCR1B=0;
#endif
    voiceActive=false;
  }
}

// Four pages, three values each. Raw ADC values make pickup unambiguous.
const uint16_t PATCH_DEFAULT[4][3] PROGMEM={{640,512,512},{480,180,700},{800,1023,0},{30,260,512}};
uint16_t patch[4][3],pots[3]={0,0,0},lastPot[3]={0,0,0};
bool acquired[3]={false,false,false};
uint8_t page=0,adcSlot=0,waveSector=2;
uint32_t lastAdcMs=0,lastPitchMs=0,lastEnvMs=0,envStarted=0;
uint16_t pitchCv=0,envLevel=0,envFrom=0,envAttackMs=4,envDecayMs=260,envCurve=512;
EnvStage envStage=ENV_IDLE;
bool ioGate=false,manualArmed=false,modeArmed=false,manualLong=false,modeLong=false,chordBlocked=false;
uint32_t manualSince=0,modeSince=0,ledNoticeSince=0,pickupNoticeSince=0;
bool ledNotice=false,pickupNotice=false;
static uint16_t timeMs(uint16_t p){return 2+((uint32_t)p*p*3998)/(1023UL*1023UL);}
static uint16_t curve15(uint16_t x,uint16_t shape){
  uint16_t squared=((uint32_t)x*x)>>15;
  uint16_t curved=shape<512?(uint32_t)x*2-squared:squared;
  uint16_t amount=shape<512?512-shape:shape-512;
  return (uint16_t)((int32_t)x+(((int32_t)curved-x)*amount)/512);
}
static void advanceEnvelope(uint32_t now){
  if(envStage==ENV_IDLE){envLevel=0;return;}
  uint32_t dt=now-envStarted;
  if(envStage==ENV_ATTACK&&dt>=envAttackMs){
    envStage=ENV_DECAY;envStarted+=envAttackMs;dt=now-envStarted;
  }
  if(envStage==ENV_DECAY&&dt>=envDecayMs){envStage=ENV_IDLE;envLevel=0;return;}
  uint16_t duration=envStage==ENV_ATTACK?envAttackMs:envDecayMs;
  uint16_t progress=(dt*32768UL)/duration;
  uint16_t shaped=curve15(progress,envCurve);
  envLevel=envStage==ENV_ATTACK?envFrom+(((uint32_t)(32768-envFrom)*shaped)>>15):32768-shaped;
}
static void triggerEnvelope(uint32_t now){
  advanceEnvelope(now);envFrom=envLevel;envStarted=now;envStage=ENV_ATTACK;
  envAttackMs=timeMs(patch[PAGE_ENV][0]);envDecayMs=timeMs(patch[PAGE_ENV][1]);envCurve=patch[PAGE_ENV][2];
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){voiceHits++;}
}
static uint32_t pitchIncrement(uint16_t cv,uint16_t tune){
  // Add one octave before applying +/- one semitone, so no unsigned wrap.
  int16_t fine=(tune>=496&&tune<=528)?0:((int32_t)tune-512)/2;
  uint16_t q=3072+((uint32_t)cv*15360)/4095+fine;
  uint8_t octave=q/3072,note=(q/256)%12;
  uint32_t lo=pgm_read_dword(&VOICE_PITCH[note]),hi=pgm_read_dword(&VOICE_PITCH[note+1]);
  return ((lo+(((hi-lo)*(q&255))>>8))<<octave)>>1;
}
static void publishControls(){
  uint16_t p=patch[PAGE_FILTER][0];uint8_t idx=p>>5,frac=p&31;
  uint8_t base=pgm_read_byte(&CUTOFF_TABLE[idx]);
  if(idx<32)base+=((pgm_read_byte(&CUTOFF_TABLE[idx+1])-base)*frac)>>5;
  uint8_t mod=(uint32_t)patch[PAGE_FILTER][2]*(186-base)/1023;
  uint8_t cutoff=base+(((uint32_t)envLevel*mod)>>15);
  uint16_t damp=469-(uint32_t)patch[PAGE_FILTER][1]*414/1023;
  uint32_t inc=pitchIncrement(pitchCv,patch[PAGE_OSC][2]);
  uint8_t candidate=patch[PAGE_OSC][0]>>8;
  if(candidate>waveSector&&patch[PAGE_OSC][0]>=(uint16_t)(waveSector+1)*256+6)waveSector=candidate;
  if(candidate<waveSector&&patch[PAGE_OSC][0]+6<(uint16_t)waveSector*256)waveSector=candidate;
  uint8_t width=26+(uint32_t)patch[PAGE_OSC][1]*204/1023;
  uint8_t level=((uint32_t)patch[PAGE_VCA][0]*128)/1023;
  uint8_t amount=((uint32_t)patch[PAGE_VCA][1]*128)/1023;
  uint8_t drive=32+(uint32_t)patch[PAGE_VCA][2]*96/1023;
  uint8_t band=inc<60397978UL?0:inc<120795955UL?1:inc<241591910UL?2:3;
  const int8_t *sine=OSC_TABLES[0][band],*triangle=OSC_TABLES[1][band],*saw=OSC_TABLES[2][band];
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    targetEnv=envLevel;targetCutoff=cutoff;targetDamping=damp;
    targetWave=waveSector;targetWidth=width;targetLevel=level;
    targetEnvAmount=amount;targetDrive=drive;
    voiceIncrement=inc;voiceBand=band;bandTables[0]=sine;bandTables[1]=triangle;bandTables[2]=saw;
  }
}
static void selectPage(uint8_t next){
  page=next&3;pickupNotice=false;
  for(uint8_t i=0;i<3;i++){acquired[i]=false;lastPot[i]=pots[i];}
}
static void applyPot(uint8_t i,uint16_t value,uint32_t now){
  uint16_t saved=patch[page][i],prev=lastPot[i];pots[i]=value;
  int16_t error=(int16_t)value-saved;
  if(!acquired[i]&&(abs(error)<=12||(prev<=saved&&value>=saved)||(prev>=saved&&value<=saved)))acquired[i]=true;
  if(acquired[i])patch[page][i]=value;
  else if(abs((int16_t)value-(int16_t)prev)>8){pickupNotice=true;pickupNoticeSince=now;}
  lastPot[i]=value;
}

// Patch storage is separate from ACAL, old presets, and the DRUM LAB kit.
const uint16_t PATCH_BASE=320;
const uint8_t PATCH_BYTES=34,PATCH_STRIDE=40;
uint8_t patchBank=1,saveBank=0,saveRecord[PATCH_BYTES];
uint16_t patchSequence=0;
int8_t saveStep=-1;
bool patchStored=false,saveError=false;
static uint16_t patchCrc(const uint8_t *r){
  uint16_t crc=0xFFFF;
  for(uint8_t i=0;i<31;i++){crc^=(uint16_t)r[i]<<8;for(uint8_t b=0;b<8;b++)crc=crc&0x8000?(uint16_t)((crc<<1)^0x1021):(uint16_t)(crc<<1);}
  return crc;
}
static bool patchValid(const uint8_t *r){
  if(r[0]!='A'||r[1]!='F'||r[2]!='V'||r[3]!=1||r[33]!=0xA5)return false;
  uint16_t crc=patchCrc(r);if(r[31]!=(uint8_t)crc||r[32]!=(uint8_t)(crc>>8))return false;
  for(uint8_t i=0;i<12;i++)if(((uint16_t)r[7+i*2]|((uint16_t)r[8+i*2]<<8))>1023)return false;
  return true;
}
static void loadPatch(){
  for(uint8_t p=0;p<4;p++)for(uint8_t i=0;i<3;i++)patch[p][i]=pgm_read_word(&PATCH_DEFAULT[p][i]);
  uint8_t r[2][PATCH_BYTES];
  for(uint8_t b=0;b<2;b++)for(uint8_t i=0;i<PATCH_BYTES;i++)r[b][i]=EEPROM.read(PATCH_BASE+b*PATCH_STRIDE+i);
  bool a=patchValid(r[0]),b=patchValid(r[1]);patchStored=a||b;patchBank=1;patchSequence=0;
  if(!patchStored)return;
  uint16_t sa=r[0][4]|((uint16_t)r[0][5]<<8),sb=r[1][4]|((uint16_t)r[1][5]<<8);
  patchBank=!a?1:!b?0:((int16_t)(sb-sa)>0?1:0);patchSequence=patchBank?sb:sa;
  for(uint8_t i=0;i<12;i++)patch[i/3][i%3]=r[patchBank][7+i*2]|((uint16_t)r[patchBank][8+i*2]<<8);
}
static void startPatchSave(){
  if(saveStep>=0)return;
  saveRecord[0]='A';saveRecord[1]='F';saveRecord[2]='V';saveRecord[3]=1;
  uint16_t seq=patchSequence+1;saveRecord[4]=seq;saveRecord[5]=seq>>8;saveRecord[6]=0;
  for(uint8_t i=0;i<12;i++){saveRecord[7+i*2]=patch[i/3][i%3];saveRecord[8+i*2]=patch[i/3][i%3]>>8;}
  uint16_t crc=patchCrc(saveRecord);saveRecord[31]=crc;saveRecord[32]=crc>>8;saveRecord[33]=0xA5;
  saveBank=patchBank^1;saveStep=0;saveError=false;
}
static void servicePatchSave(uint32_t now){
  if(saveStep<0||!eeprom_is_ready())return;
  uint16_t addr=PATCH_BASE+saveBank*PATCH_STRIDE;
  if(saveStep==0)EEPROM.update(addr+33,0);
  else if(saveStep<=33)EEPROM.update(addr+saveStep-1,saveRecord[saveStep-1]);
  else if(saveStep==34)EEPROM.update(addr+33,0xA5);
  else {
    uint8_t r[PATCH_BYTES];for(uint8_t i=0;i<PATCH_BYTES;i++)r[i]=EEPROM.read(addr+i);
    saveError=!patchValid(r);for(uint8_t i=0;i<PATCH_BYTES;i++)if(r[i]!=saveRecord[i])saveError=true;
    saveStep=-1;if(!saveError){patchBank=saveBank;patchSequence=saveRecord[4]|((uint16_t)saveRecord[5]<<8);patchStored=true;}
    ledNotice=true;ledNoticeSince=now;return;
  }
  saveStep++;
}
static void serviceUI(uint32_t now){
  if(modeBtn.stable&&manualBtn.stable){chordBlocked=true;modeArmed=manualArmed=false;}
  if(chordBlocked){if(!modeBtn.stable&&!manualBtn.stable)chordBlocked=false;return;}
  if(modeBtn.rising){modeArmed=true;modeLong=false;modeSince=now;}
  if(modeArmed&&modeBtn.stable&&!modeLong&&elapsedMs(now,modeSince,1200)){modeLong=true;startPatchSave();}
  if(modeBtn.falling&&modeArmed){modeArmed=false;if(!modeLong)selectPage(page+1);}
  if(manualBtn.rising){manualArmed=true;manualLong=false;manualSince=now;}
  if(manualArmed&&manualBtn.stable&&!manualLong&&elapsedMs(now,manualSince,500)){manualLong=true;triggerEnvelope(now);}
  if(manualBtn.falling&&manualArmed){manualArmed=false;if(!manualLong)selectPage(page+3);}
}
static void displayPage(uint32_t now){
  uint8_t bits=1<<page;
  if(saveStep>=0||saveError)bits=(now/100)&1?15:0;
  else if(ledNotice&&!elapsedMs(now,ledNoticeSince,750))bits=(now/125)&1?15:0;
  else if(pickupNotice&&!elapsedMs(now,pickupNoticeSince,350)&&((now/80)&1))bits=0;
  for(uint8_t i=0;i<4;i++)digitalWrite(PIN_LED_MODE_1+i,(bits&(1<<i))?HIGH:LOW);
  digitalWrite(PIN_INPUT_LED,envStage!=ENV_IDLE?HIGH:LOW);
  digitalWrite(PIN_IO_LED,(ioGate||(manualArmed&&manualLong&&manualBtn.stable))?HIGH:LOW);
}
static void startNormal(){
  voiceStop();pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);
  ioGate=digitalRead(PIN_OUT3)==HIGH;envStage=ENV_IDLE;envLevel=voiceEnv=targetEnv=0;
  voiceLow=voiceBandState=0;voicePhase=0;voiceFrames=voiceHits=0;
  manualArmed=modeArmed=false;chordBlocked=modeBtn.stable||manualBtn.stable;
  pots[0]=pot1;pots[1]=pot2;pots[2]=pot3;selectPage(0);
  pitchCv=normalizeCv(analogRead(PIN_CV_INPUT));waveSector=patch[0][0]>>8;publishControls();
  waveNow=waveFrom=targetWave;waveFade=32;cutoffNow=targetCutoff;levelNow=targetLevel;
  envAmountNow=targetEnvAmount;driveNow=targetDrive;widthNow=targetWidth;
  voiceZeroA=DAC_ZERO_CODE[0];voiceZeroB=DAC_ZERO_CODE[1];
  voicePos=(DAC_ZERO_CODE[0]-DAC_PLUS5_CODE[0])/2;voiceNeg=(DAC_MINUS5_CODE[0]-DAC_ZERO_CODE[0])/2;
  voiceEnvSpan=DAC_ZERO_CODE[1]-DAC_PLUS5_CODE[1];voiceWrite(voiceZeroA,voiceZeroB);
  lastEnvMs=lastPitchMs=lastAdcMs=millis();
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    voiceActive=true;
#if defined(__AVR__)
    TCCR1A=0;TCCR1B=0;TCNT1=0;OCR1A=F_CPU/VOICE_RATE-1;
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
  voiceStop();
  calibrationActive=true;calNeedRelease=true;calSaved=false;calError=false;
  calWriteStep=-1;calModeLong=false;calResetTracked=false;
  pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);pinMode(A5,INPUT);
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++)calWork[s][ch]=calAnchor[s][ch]=calActivePoint(s,ch);
  calSelectStage(0,now);calReportAt=now;
  Serial.begin(115200);
  Serial.println(F("CAL: release buttons. Center P1/P2; CW raises volts."));
}
static void calExit() {
  calibrationActive=false;calBootPending=false;calNeedRelease=false;
  calManualArmed=calModeArmed=false;
  Serial.end();
  startNormal();
}
static void serviceCalBoot(uint32_t now) {
  if(digitalRead(PIN_MODE_BUTTON)!=HIGH||digitalRead(PIN_MANUAL_BUTTON)!=HIGH) {
    calBootPending=false;startNormal();return;
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
  voiceStop();pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);pinMode(A5,INPUT);
  pinMode(PIN_MODE_BUTTON,INPUT);pinMode(PIN_MANUAL_BUTTON,INPUT);pinMode(PIN_CV_INPUT,INPUT);
  pinMode(PIN_INPUT_LED,OUTPUT);pinMode(PIN_IO_LED,OUTPUT);
  for(uint8_t i=0;i<4;i++)pinMode(PIN_LED_MODE_1+i,OUTPUT);
  pinMode(PIN_DAC_CS,OUTPUT);digitalWrite(PIN_DAC_CS,HIGH);SPI.begin();
  digitalWrite(PIN_DAC_LDAC,HIGH);pinMode(PIN_DAC_LDAC,OUTPUT);
  SPI.beginTransaction(SPISettings(8000000UL,MSBFIRST,SPI_MODE0));SPI.endTransaction();
  modeBtn.raw=modeBtn.stable=digitalRead(PIN_MODE_BUTTON)==HIGH;
  manualBtn.raw=manualBtn.stable=digitalRead(PIN_MANUAL_BUTTON)==HIGH;
  modeBtn.rawSinceMs=manualBtn.rawSinceMs=millis();
  pot1=analogRead(PIN_POT_1);pot2=analogRead(PIN_POT_2);pot3=analogRead(PIN_POT_3);
  loadCalibration();loadPatch();
  dacWriteRaw(0,DAC_ZERO_CODE[0]);dacWriteRaw(1,DAC_ZERO_CODE[1]);latchDac();
  if(modeBtn.stable&&manualBtn.stable){calBootPending=true;calBootStart=millis();}
  else startNormal();
}
void loop(){
  uint32_t now=millis();scanButton(modeBtn,now);scanButton(manualBtn,now);
  if(calBootPending||calibrationActive){
    if(elapsedMs(now,lastAdcMs,12)){lastAdcMs=now;pot1=analogRead(PIN_POT_1);pot2=analogRead(PIN_POT_2);}
    if(calBootPending)serviceCalBoot(now);else serviceCalibration(now);return;
  }
  bool high=digitalRead(PIN_OUT3)==HIGH;
  if(high&&!ioGate){pitchCv=normalizeCv(analogRead(PIN_CV_INPUT));lastPitchMs=now;triggerEnvelope(now);}
  ioGate=high;serviceUI(now);servicePatchSave(now);
  if(elapsedMs(now,lastPitchMs,2)){lastPitchMs=now;pitchCv=normalizeCv(analogRead(PIN_CV_INPUT));}
  // Spread pot conversions over time instead of blocking on three ADC reads.
  if(elapsedMs(now,lastAdcMs,4)){
    lastAdcMs=now;uint8_t i=adcSlot;adcSlot=(adcSlot+1)%3;
    uint16_t value=analogRead(i==0?PIN_POT_1:i==1?PIN_POT_2:PIN_POT_3);
    if(i==0)pot1=value;else if(i==1)pot2=value;else pot3=value;
    applyPot(i,value,now);
  }
  if(elapsedMs(now,lastEnvMs,1)){lastEnvMs=now;advanceEnvelope(now);publishControls();displayPage(now);}
}
