bool pendingAcidGenerate=false;
uint8_t acidNotes[16],acidStep=0;
uint16_t acidAccents=0,acidSlides=0,acidValue=0,acidTarget=0,acidFrom=0;
uint32_t acidLastClock=0,acidPeriod=125,acidOff=0,acidGlideStart=0;
uint16_t acidGlideMs=0;
bool acidGate=false,acidPreviousSlide=false,acidHaveClock=false;
const uint8_t ACID_SCALE[7] PROGMEM={0,1,3,5,7,8,10}; // Phrygian
static void acidGenerate() {
  uint8_t positions[16],degrees[7];for(uint8_t i=0;i<16;i++){positions[i]=i;acidNotes[i]=255;}
  for(uint8_t i=0;i<7;i++)degrees[i]=i;
  for(uint8_t i=15;i>0;i--){uint8_t j=random(i+1);uint8_t v=positions[i];positions[i]=positions[j];positions[j]=v;}
  for(uint8_t i=6;i>0;i--){uint8_t j=random(i+1);uint8_t v=degrees[i];degrees[i]=degrees[j];degrees[j]=v;}
  uint8_t maxNotes=((uint32_t)pot1*16+511)/1023;
  uint8_t count=maxNotes?random((maxNotes+1)/2,maxNotes+1):0;
  uint8_t pool=(uint32_t)pot2*7/1023,maxOct=(uint32_t)pot2*3/1024;
  acidAccents=acidSlides=0;
  for(uint8_t i=0;i<count;i++) {
    uint8_t pos=positions[i];uint8_t degree=(pool&&random(2))?degrees[random(pool)]:0;
    acidNotes[pos]=pgm_read_byte(&ACID_SCALE[degree])+12*random(maxOct+1);
    if(random(1024)<pot3/2)acidAccents|=(uint16_t)1<<pos;
    if(random(1024)<pot3/2)acidSlides|=(uint16_t)1<<pos;
  }
  acidStep=0;acidPreviousSlide=false;acidGate=false;setGate(false);dacSetB(0);
}
static void modeAcid(uint32_t now,bool trigger) {
  if(modeJustChanged){acidValue=acidTarget=0;acidHaveClock=false;acidPeriod=125;acidGlideMs=0;acidGenerate();dacSetA(0);}
  if(pendingAcidGenerate){pendingAcidGenerate=false;acidGenerate();acidGlideMs=0;}
  if(trigger) {
    if(acidHaveClock){acidPeriod=now-acidLastClock;if(acidPeriod<10)acidPeriod=10;if(acidPeriod>4000)acidPeriod=4000;}
    acidHaveClock=true;acidLastClock=now;
    uint8_t note=acidNotes[acidStep];bool slide=(acidSlides>>acidStep)&1;
    if(note!=255) {
      acidTarget=((uint32_t)note*4095+30)/60; // 12 semitones = 1 V over 0..5 V
      bool glide=acidPreviousSlide&&acidGate;
      acidFrom=acidValue;acidGlideStart=now;acidGlideMs=glide?(uint16_t)(acidPeriod/3):0;
      if(acidGlideMs>120)acidGlideMs=120;
      if(!acidGlideMs)acidValue=acidTarget;
      acidGate=true;setGate(true);acidOff=now+(slide?acidPeriod+2:acidPeriod/2);
      dacSetB(((acidAccents>>acidStep)&1)?4095:0);
    } else {acidGate=false;setGate(false);dacSetB(0);acidGlideMs=0;}
    acidPreviousSlide=note!=255&&slide;acidStep=(acidStep+1)&15;
  }
  if(acidGlideMs){uint32_t dt=now-acidGlideStart;if(dt>=acidGlideMs){acidValue=acidTarget;acidGlideMs=0;}else acidValue=(int32_t)acidFrom+((int32_t)acidTarget-acidFrom)*(int32_t)dt/acidGlideMs;}
  if(acidGate&&(int32_t)(now-acidOff)>=0){acidGate=false;setGate(false);dacSetB(0);}
  dacSetA(acidValue);
}
// Mode 10: melodic CV/gate sequencer, no oscillator or audio ISR.
// ORBIT: integer motif grammar, bounded walk and phrase-boundary evolution.
