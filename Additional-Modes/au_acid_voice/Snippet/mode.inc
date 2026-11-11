const uint16_t VOICE_RATE=6400;
const uint8_t VOICE_FILTER_BASE=14;
const int8_t VOICE_WAVES[2][4][128] PROGMEM={
 {
  {0,72,95,80,70,76,80,73,68,72,73,68,65,67,67,63,60,62,61,57,55,56,56,52,51,51,50,47,46,46,45,42,41,41,40,37,36,36,34,32,30,31,29,26,25,25,24,21,20,20,18,16,15,15,13,11,10,10,8,6,5,5,3,0,0,0,-3,-5,-5,-6,-8,-10,-10,-11,-13,-15,-15,-16,-18,-20,-20,-21,-24,-25,-25,-26,-29,-31,-30,-32,-34,-36,-36,-37,-40,-41,-41,-42,-45,-46,-46,-47,-50,-51,-51,-52,-56,-56,-55,-57,-61,-62,-60,-63,-67,-67,-65,-68,-73,-72,-68,-73,-80,-76,-70,-80,-95,-72},
  {0,41,73,91,95,88,78,69,66,69,73,75,75,71,65,61,60,61,63,63,62,58,54,52,51,51,52,52,50,47,44,42,41,41,42,41,39,36,33,31,31,31,31,30,28,25,22,21,21,21,20,19,16,14,12,11,10,10,10,8,5,3,1,0,0,0,-1,-3,-5,-8,-10,-10,-10,-11,-12,-14,-16,-19,-20,-21,-21,-21,-22,-25,-28,-30,-31,-31,-31,-31,-33,-36,-39,-41,-42,-41,-41,-42,-44,-47,-50,-52,-52,-51,-51,-52,-54,-58,-62,-63,-63,-61,-60,-61,-65,-71,-75,-75,-73,-69,-66,-69,-78,-88,-95,-91,-73,-41},
  {0,22,43,61,76,86,93,95,94,90,84,77,71,66,62,59,59,59,61,62,64,65,65,63,61,58,54,50,47,44,42,41,41,41,42,42,42,42,40,38,36,33,30,27,25,23,22,21,21,21,21,21,20,19,17,15,12,9,6,4,2,1,0,0,0,0,0,-1,-2,-4,-6,-9,-12,-15,-17,-19,-20,-21,-21,-21,-21,-21,-22,-23,-25,-27,-30,-33,-36,-38,-40,-42,-42,-42,-42,-41,-41,-41,-42,-44,-47,-50,-54,-58,-61,-63,-65,-65,-64,-62,-61,-59,-59,-59,-62,-66,-71,-77,-84,-90,-94,-95,-93,-86,-76,-61,-43,-22},
  {0,7,14,21,28,35,42,48,54,60,65,70,74,79,82,86,88,91,92,94,95,95,95,94,93,92,90,88,86,83,80,77,73,69,66,62,58,54,50,46,42,38,34,30,27,24,21,18,15,13,11,9,7,5,4,3,2,1,1,1,0,0,0,0,0,0,0,0,0,-1,-1,-1,-2,-3,-4,-5,-7,-9,-11,-13,-15,-18,-21,-24,-27,-30,-34,-38,-42,-46,-50,-54,-58,-62,-66,-69,-73,-77,-80,-83,-86,-88,-90,-92,-93,-94,-95,-95,-95,-94,-92,-91,-88,-86,-82,-79,-74,-70,-65,-60,-54,-48,-42,-35,-28,-21,-14,-7}
 },
 {
  {0,70,95,82,73,80,86,81,76,80,84,81,78,80,83,81,78,80,83,81,79,81,82,81,79,81,82,81,79,81,82,81,79,81,82,81,79,81,82,81,79,81,82,81,79,81,83,80,78,81,83,80,78,81,84,80,76,81,86,80,73,82,95,70,0,-70,-95,-82,-73,-80,-86,-81,-76,-80,-84,-81,-78,-80,-83,-81,-78,-80,-83,-81,-79,-81,-82,-81,-79,-81,-82,-81,-79,-81,-82,-81,-79,-81,-82,-81,-79,-81,-82,-81,-79,-81,-82,-81,-79,-81,-83,-80,-78,-81,-83,-80,-78,-81,-84,-80,-76,-81,-86,-80,-73,-82,-95,-70},
  {0,39,70,89,95,91,82,75,72,75,80,84,86,85,81,77,76,77,80,83,84,83,81,78,77,78,80,83,84,83,81,78,77,78,81,83,84,83,80,78,77,78,81,83,84,83,80,77,76,77,81,85,86,84,80,75,72,75,82,91,95,89,70,39,0,-39,-70,-89,-95,-91,-82,-75,-72,-75,-80,-84,-86,-85,-81,-77,-76,-77,-80,-83,-84,-83,-81,-78,-77,-78,-80,-83,-84,-83,-81,-78,-77,-78,-81,-83,-84,-83,-80,-78,-77,-78,-81,-83,-84,-83,-80,-77,-76,-77,-81,-85,-86,-84,-80,-75,-72,-75,-82,-91,-95,-89,-70,-39},
  {0,20,39,56,70,81,89,94,95,94,91,87,82,78,74,72,72,72,74,77,80,83,85,86,87,86,85,83,80,78,76,74,74,74,76,78,80,83,85,86,87,86,85,83,80,77,74,72,72,72,74,78,82,87,91,94,95,94,89,81,70,56,39,20,0,-20,-39,-56,-70,-81,-89,-94,-95,-94,-91,-87,-82,-78,-74,-72,-72,-72,-74,-77,-80,-83,-85,-86,-87,-86,-85,-83,-80,-78,-76,-74,-74,-74,-76,-78,-80,-83,-85,-86,-87,-86,-85,-83,-80,-77,-74,-72,-72,-72,-74,-78,-82,-87,-91,-94,-95,-94,-89,-81,-70,-56,-39,-20},
  {0,10,20,29,38,47,55,63,70,76,81,85,89,92,94,95,95,95,94,92,90,88,86,83,80,78,75,73,71,69,68,67,67,67,68,69,71,73,75,78,80,83,86,88,90,92,94,95,95,95,94,92,89,85,81,76,70,63,55,47,38,29,20,10,0,-10,-20,-29,-38,-47,-55,-63,-70,-76,-81,-85,-89,-92,-94,-95,-95,-95,-94,-92,-90,-88,-86,-83,-80,-78,-75,-73,-71,-69,-68,-67,-67,-67,-68,-69,-71,-73,-75,-78,-80,-83,-86,-88,-90,-92,-94,-95,-95,-95,-94,-92,-89,-85,-81,-76,-70,-63,-55,-47,-38,-29,-20,-10}
 },
};
const uint32_t VOICE_PITCH[13] PROGMEM={21946743UL,23251764UL,24634386UL,26099223UL,27651164UL,29295387UL,31037382UL,32882961UL,34838283UL,36909875UL,39104651UL,41429934UL,43893486UL};

volatile bool voiceActive=false;
volatile uint8_t voiceCommands=0;
volatile uint32_t voiceIncrement=0;
volatile uint16_t voiceDecay=1000,voiceDamping=250;
volatile uint8_t voiceModulation=125;
volatile bool voiceSquare=false;
volatile uint16_t voiceFrames=0,voiceHits=0;
uint32_t voicePhase=0,voiceControlsAt=0,voicePitchAt=0;
uint16_t voiceEnv=0,voiceAttackStep=0;
uint8_t voiceF=VOICE_FILTER_BASE;
uint8_t voiceAttack=0,voiceWaveMix=0;
volatile uint8_t voiceBand=0;
int16_t voiceLow=0,voiceBandState=0;
uint16_t voiceZeroA=2048,voiceZeroB=2048,voicePos=980,voiceNeg=980,voiceEnvSpan=1961;
#if !defined(__AVR__)
uint16_t voiceWritten[2];
#endif
static int16_t voiceLimit(int16_t v,int16_t limit){return v>limit?limit:v<-limit?-limit:v;}
static int16_t voiceTable(bool square,uint8_t band,uint32_t phase){
  // Byte-aligned extraction avoids slow 25/17-bit shift loops on AVR -Os.
  uint16_t top=phase>>16;
  uint8_t i=(uint8_t)(top>>8)>>1,frac=top>>1;
  int16_t a=(int8_t)pgm_read_byte(&VOICE_WAVES[square][band][i]);
  int16_t delta=(int8_t)pgm_read_byte(&VOICE_WAVES[square][band][(i+1)&127])-a;
  uint16_t amount=((uint16_t)(delta<0?-delta:delta)*frac)>>8;
  return a+(delta<0?-(int16_t)amount:(int16_t)amount);
}
static uint32_t voicePitchIncrement(uint16_t cv){
  if(cv>4095)cv=4095;
  uint16_t semitone=(uint32_t)cv*(60U*256U)/4095U;
  uint8_t note=semitone>>8,octave=note/12;note%=12;
  uint32_t lo=pgm_read_dword(&VOICE_PITCH[note]);
  uint32_t hi=pgm_read_dword(&VOICE_PITCH[note+1]);
  return (lo+(((hi-lo)*(semitone&255))>>8))<<octave;
}
static uint16_t voiceAudioCode(int16_t sample){
  uint16_t span=sample>=0?voicePos:voiceNeg;
  uint8_t magnitude=sample>=0?sample:-sample;
  uint16_t delta=magnitude*(span>>7)+((magnitude*(span&127))>>7);
  return sample>=0?voiceZeroA-delta:voiceZeroA+delta;
}
static void voiceWrite(uint16_t a,uint16_t b){
#if defined(__AVR__)
  // Keep the GPIO/SPI/LDAC path confirmed working on this module.
  digitalWrite(PIN_DAC_CS,LOW);
  SPI.transfer((uint8_t)((0x1000|a)>>8));SPI.transfer((uint8_t)a);
  digitalWrite(PIN_DAC_CS,HIGH);
  digitalWrite(PIN_DAC_CS,LOW);
  SPI.transfer((uint8_t)((0x9000|b)>>8));SPI.transfer((uint8_t)b);
  digitalWrite(PIN_DAC_CS,HIGH);
  digitalWrite(PIN_DAC_LDAC,LOW);
  __builtin_avr_delay_cycles(F_CPU/1000000UL);
  digitalWrite(PIN_DAC_LDAC,HIGH);
#else
  voiceWritten[0]=a;voiceWritten[1]=b;
#endif
  voiceFrames++;
}
static void voiceAudioTick(){
  uint8_t commands=voiceCommands;voiceCommands=0;
  if(commands&2)voiceSquare=!voiceSquare;
  if(commands&1){voiceHits++;voiceAttack=16;voiceAttackStep=(32768U-voiceEnv)>>4;}
  if(voiceWaveMix<(voiceSquare?32:0))voiceWaveMix++;
  else if(voiceWaveMix>(voiceSquare?32:0))voiceWaveMix--;
  voicePhase+=voiceIncrement;
  if(voiceAttack){
    voiceEnv+=voiceAttackStep;if(!--voiceAttack)voiceEnv=32768;
  }else if(!(voiceFrames&3)&&voiceEnv){
    uint16_t drop=((uint32_t)voiceEnv*voiceDecay)>>16;
    voiceEnv-=drop?drop:1;
  }
  int16_t sample=0;
  if(voiceEnv){
    if(!(voiceFrames&3))voiceF=VOICE_FILTER_BASE+(((voiceEnv>>8)*(uint16_t)voiceModulation)>>7);
    int16_t wave=voiceTable(voiceWaveMix==32,voiceBand,voicePhase);
    if(voiceWaveMix&&voiceWaveMix<32){
      int16_t square=voiceTable(true,voiceBand,voicePhase);
      wave+=((square-wave)*voiceWaveMix)>>5;
    }
    int16_t input=wave*32;
    uint8_t f=voiceF;uint16_t damping=voiceDamping;
    // Q8 coefficients avoid costly wide bit-shifts. State bounds keep all
    // sums within int16_t even at maximum damping (469/256) and resonance.
    for(uint8_t i=0;i<2;i++){
      int16_t lowStep=((int32_t)voiceBandState*f)>>8;
      voiceLow=voiceLimit(voiceLow+lowStep,8192);
      int16_t feedback=((int32_t)voiceBandState*damping)>>8;
      int16_t high=voiceLimit(input-voiceLow-feedback,16000);
      int16_t bandStep=((int32_t)high*f)>>8;
      voiceBandState=voiceLimit(voiceBandState+bandStep,8192);
    }
    int16_t filtered=voiceLimit(voiceLow>>5,127);
    sample=(filtered*(int16_t)(voiceEnv>>8))>>7;
  }else {voiceLow=voiceBandState=0;voiceF=VOICE_FILTER_BASE;}
  uint16_t envelope=voiceZeroB-(((uint32_t)voiceEnv*voiceEnvSpan)>>15);
  voiceWrite(voiceAudioCode(sample),envelope);
}
#if defined(__AVR__)

#endif
static void voiceStop(){
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
#if defined(__AVR__)
    TIMSK1&=(uint8_t)~_BV(OCIE1A);TCCR1B=0;
#endif
    voiceActive=false;voiceCommands=0;
  }
}
static void voiceControls(uint32_t now,bool force=false){
  if(force||elapsedMs(now,voicePitchAt,2)){
    voicePitchAt=now;
    uint32_t increment=voicePitchIncrement(cvLevelQ12);
    uint8_t band=increment<60397978UL?0:increment<120795955UL?1:increment<241591910UL?2:3;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){voiceIncrement=increment;voiceBand=band;}
  }
  if(!force&&!elapsedMs(now,voiceControlsAt,12))return;
  voiceControlsAt=now;
  uint16_t decayMs=20+(uint32_t)pot1*pot1*1980/(1023UL*1023UL);
  uint16_t decay=245760UL/decayMs;
  uint16_t damping=469-(uint32_t)pot2*414/1023;
  uint8_t modulation=(uint32_t)pot3*172/1023;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){voiceDecay=decay;voiceDamping=damping;voiceModulation=modulation;}
}
static void voiceStart(){
  voiceStop();voicePhase=0;voiceEnv=0;voiceAttack=0;voiceLow=voiceBandState=0;
  voiceF=VOICE_FILTER_BASE;voiceWaveMix=voiceSquare?32:0;voiceFrames=voiceHits=0;
  voiceZeroA=DAC_ZERO_CODE[0];voiceZeroB=DAC_ZERO_CODE[1];
  voicePos=(DAC_ZERO_CODE[0]-DAC_PLUS5_CODE[0])/2;
  voiceNeg=(DAC_MINUS5_CODE[0]-DAC_ZERO_CODE[0])/2;
  voiceEnvSpan=DAC_ZERO_CODE[1]-DAC_PLUS5_CODE[1];
  cvLevelQ12=normalizeCv(analogRead(PIN_CV_INPUT));voiceControls(millis(),true);
  SPI.beginTransaction(SPISettings(8000000UL,MSBFIRST,SPI_MODE0));SPI.endTransaction();
  voiceWrite(voiceZeroA,voiceZeroB);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    voiceActive=true;
#if defined(__AVR__)
    TCCR1A=0;TCCR1B=0;TCNT1=0;OCR1A=F_CPU/VOICE_RATE-1;
    TIFR1=_BV(OCF1A);TIMSK1|=_BV(OCIE1A);TCCR1B=_BV(WGM12)|_BV(CS10);
#endif
  }
}
static void voiceEvents(bool gate,bool toggle){
  uint8_t commands=(gate?1:0)|(toggle?2:0);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){voiceCommands|=commands;}
}


static void startModule(){::audioOwner=1;voiceStart();}
static void moduleTick(){voiceControls(millis());voiceEvents(ioRising,manualBtn.rising);digitalWrite(PIN_INPUT_LED,voiceSquare?HIGH:LOW);}
