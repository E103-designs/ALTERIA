const uint8_t ORBIT_SCALES[4][7] PROGMEM={{0,2,4,7,9,0,0},{0,3,5,7,10,0,0},{0,2,3,5,7,9,10},{0,2,3,5,7,8,10}};
uint8_t orbitNotes[16],orbitStep=0,orbitScale=0,orbitNextScale=0,orbitRoot=0,orbitLastDegree=0;
uint16_t orbitRhythm=0,orbitAccents=0;
uint32_t orbitLastClock=0,orbitPeriod=125,orbitManualSince=0,orbitNoticeAt=0;
bool orbitHaveClock=false,orbitNewTheme=false,orbitManualArmed=false,orbitManualLong=false,orbitNotice=false;
static uint8_t orbitBound(int8_t d){return d<0?0:d>14?14:d;}
static uint8_t orbitWalk(uint8_t previous){
  int8_t delta;uint8_t r=random(16);
  if(r<3)delta=0;else if(r<12)delta=random(2)?1:-1;else delta=random(2)?3:-3;
  if(previous>10&&delta>0)delta=-delta;
  if(previous<3&&delta<0)delta=-delta;
  return orbitBound((int8_t)previous+delta);
}
static void orbitGenerate(){
  uint8_t motif[4]={0,0,0,0};
  for(uint8_t i=1;i<4;i++)motif[i]=orbitWalk(motif[i-1]);
  orbitRhythm=orbitAccents=0;
  for(uint8_t i=0;i<16;i++){
    uint8_t block=i>>2,within=i&3;
    int8_t degree=motif[within];
    if(block==1&&within==3)degree=orbitWalk(degree);
    if(block==2)degree+=2;
    orbitNotes[i]=orbitBound(degree);
    if(!within||random(4))orbitRhythm|=(uint16_t)1<<i;
    if(!within||random(5)==0)orbitAccents|=(uint16_t)1<<i;
  }
  orbitNotes[0]=orbitNotes[15]=0;orbitRhythm|=0x8001;
}
static void orbitEvolve(){
  if(!pot3)return;
  uint8_t attempts=1+pot3/256;
  for(uint8_t i=0;i<attempts;i++)if(random(1024)<pot3){
    uint8_t step=random(1,15);orbitNotes[step]=orbitWalk(orbitNotes[step]);
    if(random(4)==0)orbitRhythm^=(uint16_t)1<<step;
    if(random(4)==0)orbitAccents^=(uint16_t)1<<step;
  }
}
static uint16_t orbitPitch(uint8_t degree){
  uint8_t size=orbitScale<2?5:7;
  uint8_t span=2+(uint32_t)pot2*(size*3-2)/1023;
  uint8_t d=(uint16_t)degree*span/14;
  uint8_t note=orbitRoot+12*(d/size)+pgm_read_byte(&ORBIT_SCALES[orbitScale][d%size]);
  return ((uint32_t)note*4095+30)/60; // five volts = sixty semitones
}
static void orbitStart(){
  orbitStep=0;orbitHaveClock=false;orbitNewTheme=false;orbitManualArmed=false;
  orbitManualLong=false;orbitNotice=false;orbitNextScale=orbitScale;orbitLastDegree=0;
  orbitRoot=(uint32_t)pot1*12/1024;orbitGenerate();out1(orbitPitch(0));out2(0);setGate(false);
}
static void modeOrbit(uint32_t now,bool clock){
  uint8_t candidate=(uint32_t)pot1*12/1024;
  if(candidate>orbitRoot&&pot1>=((uint16_t)(orbitRoot+1)*1024/12)+5)orbitRoot=candidate;
  if(candidate<orbitRoot&&pot1+5<(uint16_t)orbitRoot*1024/12)orbitRoot=candidate;
  if(manualBtn.rising){orbitManualArmed=true;orbitManualLong=false;orbitManualSince=now;}
  if(orbitManualArmed&&manualBtn.stable&&!orbitManualLong&&elapsedMs(now,orbitManualSince,650)){
    orbitManualLong=true;orbitNextScale=(orbitNextScale+1)&3;orbitNotice=true;orbitNoticeAt=now;
  }
  if(manualBtn.falling&&orbitManualArmed){orbitManualArmed=false;if(!orbitManualLong)orbitNewTheme=true;}
  if(clock){
    uint32_t interval=now-orbitLastClock;
    if(orbitHaveClock&&interval<5)clock=false;
    else {
      if(orbitHaveClock)orbitPeriod=interval>4000?125:interval;
      orbitLastClock=now;orbitHaveClock=true;
      if(!orbitStep){
        orbitScale=orbitNextScale;
        if(orbitNewTheme){orbitGenerate();orbitNewTheme=false;}
        else if(!modeJustChanged)orbitEvolve();
      }
      uint16_t bit=(uint16_t)1<<orbitStep;
      schClearChannel(CH_G);setGate(false);
      if(orbitRhythm&bit){
        orbitLastDegree=orbitNotes[orbitStep];
        uint16_t width=orbitPeriod*3/5;if(width<1)width=1;if(width>250)width=250;
        schPulseG(now,1,width); // pitch settles before every new gate edge
        out2((orbitAccents&bit)?4095:0);
      }else out2(0);
      orbitStep=(orbitStep+1)&15;
    }
  }
  out1(orbitPitch(orbitLastDegree));schService(now);modeJustChanged=false;
}
