ClockLane clockLane[3];
const uint8_t CLOCK_DIV[11]={8,6,4,3,2,1,1,1,1,1,1};
const uint8_t CLOCK_MUL[11]={1,1,1,1,1,1,2,3,4,6,8};
uint32_t clockPeriodUs=500000UL,clockMasterLast=0,clockLastExt=0,clockLastTap=0;
uint32_t clockTapIntervals[2]={0,0};
uint8_t clockBeat=0,clockTapCount=0,clockTapSlot=0;
bool clockExternal=false;
static uint8_t clockIndex(uint16_t p){return (uint32_t)p*11/1024;}
static void clockWrite(uint8_t ch,bool on) {
  clockLane[ch].high=on;
  if(ch==0)dacSetA(on?4095:0);else if(ch==1)dacSetB(on?4095:0);else setGate(on);
}
static void clockAlign(uint8_t ch,uint32_t now) {
  uint16_t p=ch==0?pot1:ch==1?pot2:pot3;
  uint8_t i=clockIndex(p);clockLane[ch].ratio=i;
  uint32_t cycle=clockPeriodUs*CLOCK_DIV[i]/CLOCK_MUL[i];
  uint32_t offset=(clockPeriodUs*(clockBeat%CLOCK_DIV[i])+(uint32_t)(now-clockMasterLast))%cycle;
  clockLane[ch].halfUs=cycle/2;
  bool on=offset<cycle/2;clockWrite(ch,on);
  clockLane[ch].nextUs=now+(on?cycle/2-offset:cycle-offset);
}
static void clockAlignAll(uint32_t now){for(uint8_t ch=0;ch<3;ch++)clockAlign(ch,now);}
static void clockStart(uint32_t now) {
  clockExternal=false;clockBeat=0;clockMasterLast=now;clockTapCount=0;
  clockAlignAll(now);
}
static void serviceClock3(uint32_t now,bool externalRise,bool tap) {
  if(modeJustChanged){clockStart(now);modeJustChanged=false;}
  if(clockExternal&&(uint32_t)(now-clockLastExt)>clockPeriodUs*3) {
    clockExternal=false;clockTapCount=0;
    uint32_t n=(uint32_t)(now-clockMasterLast)/clockPeriodUs;
    clockBeat=(clockBeat+n)%24;clockMasterLast+=n*clockPeriodUs;
  }
  if(externalRise&&clockExternal) {
    uint32_t interval=now-clockLastExt;
    if(interval<20000UL)externalRise=false;
    else if(interval>4000000UL)clockExternal=false;
  }
  if(externalRise) {
    if(clockExternal) {
      uint32_t interval=now-clockLastExt;
      if(interval>=20000UL&&interval<=4000000UL)clockPeriodUs=interval;
      clockBeat=(clockBeat+1)%24;
    } else clockBeat=0;
    clockExternal=true;clockLastExt=clockMasterLast=now;clockTapCount=0;
    clockAlignAll(now);
  } else if(!clockExternal) {
    uint32_t n=(uint32_t)(now-clockMasterLast)/clockPeriodUs;
    if(n){clockBeat=(clockBeat+n)%24;clockMasterLast+=n*clockPeriodUs;}
  }
  if(tap&&!clockExternal) {
    uint32_t interval=now-clockLastTap;
    if(!clockTapCount||interval>4000000UL){clockTapCount=1;clockTapSlot=0;clockLastTap=now;}
    else if(interval>=80000UL) {
      clockLastTap=now;clockTapIntervals[clockTapSlot]=interval;clockTapSlot^=1;
      if(clockTapCount<3)clockTapCount++;
      if(clockTapCount>=3){clockPeriodUs=(clockTapIntervals[0]+clockTapIntervals[1])/2;clockBeat=0;clockMasterLast=now;clockAlignAll(now);}
    }
  }
  for(uint8_t ch=0;ch<3;ch++) {
    uint16_t p=ch==0?pot1:ch==1?pot2:pot3;
    if(clockIndex(p)!=clockLane[ch].ratio){clockAlign(ch,now);continue;}
    uint32_t half=clockLane[ch].halfUs;
    if((int32_t)(now-clockLane[ch].nextUs)>=0) {
      uint32_t missed=(uint32_t)(now-clockLane[ch].nextUs)/half+1;
      if(missed&1)clockWrite(ch,!clockLane[ch].high);
      clockLane[ch].nextUs+=missed*half;
    }
  }
}
