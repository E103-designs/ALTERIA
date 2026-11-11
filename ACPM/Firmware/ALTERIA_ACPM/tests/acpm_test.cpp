#include <cassert>
#include <cstdio>
#include "../ALTERIA_ACPM.ino"

static uint8_t leds(){
  uint8_t v=0;for(uint8_t i=0;i<4;i++)v=(v<<1)|pins[PIN_LED1+i];return v;
}
static void run(uint32_t start,uint32_t stop){
  for(uint32_t i=start;i<=stop;i++){clockUs=i*1000;loop();}
}
static void finish(){
  for(uint8_t i=0;i<RECORD_BYTES+2;i++)serviceSave(i);
  assert(writeStep<0);
}
static void bankDefaults(){
  length=16;rangeIndex=selected=sequence=0;addressMode=false;bank=1;writeStep=-1;
}
static void calFixture(){
  uint8_t r[22]={'A','C','A','L',1,1,0,0,8,32,8,87,0,110,0,168,15,140,15,0,0,0xa5};
  uint16_t crc=crc16(r,19);r[19]=crc;r[20]=crc>>8;
  for(uint8_t i=0;i<22;i++)EEPROM.bytes[256+i]=r[i];
}
int main(){
  calFixture();
  adc[PIN_CV]=900;adc[A0]=1023;setup();
  assert(length==16&&selected==0&&leds()==0&&dirs[PIN_IO]==OUTPUT&&!gateOutput);
  assert(lastDac[0]==2048&&lastDac[1]==2080&&EEPROM.writes==0);
  run(1,9);assert(selected==0); // No advance on high-at-boot.
  adc[PIN_CV]=0;run(10,12);adc[PIN_CV]=600;run(13,15);
  assert(selected==1&&leds()==1);run(16,24);assert(selected==1);
  adc[PIN_CV]=300;run(25,28);adc[PIN_CV]=600;run(29,32);assert(selected==1);
  adc[PIN_CV]=0;run(33,36);

  for(uint8_t n=1;n<=16;n++){
    length=n;page=PLAY;lengthNotice=false;feedback=0;
    for(uint8_t i=0;i<n;i++){selected=i;display(0);assert(leds()==i);}
    nextStep();assert(selected==0);
    for(uint16_t v=0;v<1024;v++)assert(sector(v,n)<n);
    assert(sector(0,n)==0&&sector(1023,n)==n-1);
    for(uint8_t i=1;i<n;i++){
      uint16_t edge=boundary(i,n);
      assert(stableSector(edge,n,i-1)==i-1);
      assert(stableSector(edge+4,n,i-1)==i);
      assert(stableSector(edge-1,n,i)==i);
      assert(stableSector(edge-5,n,i)==i-1);
    }
  }
  // Every range is centered on ACAL zero and monotone in physical voltage.
  for(uint8_t r=0;r<3;r++){
    rangeIndex=r;
    for(uint8_t c=0;c<2;c++){
      assert(dacCode(c,512)==zeroCode[c]);
      assert(dacCode(c,0)==zeroCode[c]+(minusCode[c]-zeroCode[c])*RANGE_VOLTS[r]/5);
      assert(dacCode(c,1023)==zeroCode[c]-(zeroCode[c]-plusCode[c])*RANGE_VOLTS[r]/5);
      for(uint16_t v=1;v<=1023;v++)assert(dacCode(c,v)<=dacCode(c,v-1));
    }
  }
  length=16;selected=0;rangeIndex=0;addressMode=false;page=PLAY;steps[15]={123,987,true};
  pins[PIN_MODE]=HIGH;run(40,75);pins[PIN_MANUAL]=HIGH;run(76,490);
  assert(page==SETUP&&selected==0&&leds()==8);
  pins[PIN_MODE]=pins[PIN_MANUAL]=LOW;run(491,527);
  adc[A0]=480;run(528,550);assert(length==8&&leds()==7);
  run(551,1580);assert(leds()==8);
  adc[A1]=450;run(1581,1600);assert(rangeIndex==1&&leds()==4);
  adc[A6]=800;run(1601,1620);assert(addressMode&&leds()==5&&dirs[PIN_IO]==OUTPUT);
  adc[A1]=1023;run(1621,1640);assert(rangeIndex==2&&leds()==3);
  adc[A0]=1023;run(1641,1670);assert(length==16&&leds()==15);
  assert(steps[15].a==123&&steps[15].b==987&&steps[15].gate);
  adc[A0]=480;run(1671,1700);assert(length==8&&leds()==7);
  pins[PIN_MODE]=HIGH;run(1701,1740);pins[PIN_MODE]=LOW;run(1741,1775);
  assert(page==PLAY&&selected==0&&leds()==0);
  adc[PIN_CV]=500;run(1776,1785);assert(selected==3&&leds()==3);
  pins[PIN_MANUAL]=HIGH;run(1786,1820);assert(selected==4);
  run(1821,1880);assert(selected==4); // Same CV band, no snap back or repeat.
  pins[PIN_MANUAL]=LOW;run(1881,1915);
  adc[PIN_CV]=900;run(1916,1925);assert(selected==7);
  pins[PIN_MODE]=HIGH;run(1926,1960);pins[PIN_MODE]=LOW;run(1961,1995);
  assert(page==EDIT&&selected==7&&pins[PIN_CV_LED]);
  adc[PIN_CV]=0;run(1996,2005);assert(selected==7); // No external step while editing.
  // Pickup keeps original values until the pot crosses them.
  assert(steps[7].a==512&&!steps[7].gate);
  adc[A0]=320;run(2006,2025);assert(steps[7].a==512);
  adc[A0]=650;run(2026,2045);assert(steps[7].a==650);
  adc[A6]=0;run(2046,2065);assert(!steps[7].gate);
  adc[A6]=900;run(2066,2085);assert(steps[7].gate&&gateOutput&&pins[PIN_IO]);
  pins[PIN_MODE]=HIGH;run(2086,3300);
  assert(feedback==1&&page==EDIT&&bank==0);
  unsigned writes=EEPROM.writes;
  run(3301,4300);assert(EEPROM.writes==writes);
  pins[PIN_MODE]=LOW;run(4301,4340);assert(page==EDIT);
  // Save both the active length and the 16 stored steps, not just visible ones.
  uint8_t first[1024];memcpy(first,EEPROM.bytes,1024);
  bankDefaults();loadBank();
  assert(length==8&&selected==7&&rangeIndex==2&&addressMode);
  assert(steps[7].a==650&&steps[7].gate&&steps[15].b==987);
  for(uint8_t cut=0;cut<=RECORD_BYTES+1;cut++){
    memcpy(EEPROM.bytes,first,1024);bankDefaults();loadBank();
    length=16;selected=15;rangeIndex=0;addressMode=false;startSave();
    for(uint8_t i=0;i<cut;i++)serviceSave(i);
    bankDefaults();loadBank();
    assert(length==(cut<=RECORD_BYTES?8:16));
  }
  EEPROM.bytes[BANK_ADDR[1]+8]^=1;bankDefaults();loadBank();assert(length==8);
  length=4;selected=0;startSave();EEPROM.blockWrites=true;finish();EEPROM.blockWrites=false;
  assert(feedback==2);bankDefaults();loadBank();assert(length==8);
  for(uint16_t i=0;i<1024;i++)
    if((i<190||i>248)&&(i<940||i>998))assert(EEPROM.bytes[i]==first[i]);
  // Chord/preview time arithmetic remains valid across millis rollover.
  assert(elapsed(4,0xfffffff0,20));
  puts("PASS: 16 binary steps, LED setup/preview, CV clock/address with hysteresis, pickup, DAC ranges/ACAL, save/reload/interrupted writes, EEPROM isolation");
}
