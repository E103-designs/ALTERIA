/*
  ALTERIA ACPM 1.0.0 - Addressable Clocked Preset Manager
  REV5-2 / Arduino Nano ATmega328P 16 MHz / MCP4822
  CV IN = clock or address (0..5 V nominal); I/O always gate OUTPUT.
  LED1..4 left to right: indices 0..15 (step 1 = 0000, step 16 = 1111).
  MODE short = PLAY/EDIT; MODE long = save; MODE then MANUAL = SETUP.
  Setup: P1 length, P2 range, P3 CV IN mode. ACAL is read only.
*/
#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>
#include <avr/eeprom.h>

#if defined(__AVR__) && (!defined(__AVR_ATmega328P__) || F_CPU!=16000000UL)
#error "ACPM requires an ATmega328P at 16 MHz."
#endif

const uint8_t PIN_IO=2,PIN_CV_LED=3,PIN_MODE=4,PIN_IO_LED=5,PIN_LED1=6;
const uint8_t PIN_CS=10,PIN_LDAC=12,PIN_CV=A2,PIN_MANUAL=A3;
const uint8_t POT_PINS[3]={A0,A1,A6};
const uint16_t CV_POLL_US=550;
const uint8_t POT_POLL_MS=10,POT_MOVE=5;
const uint16_t CHORD_MS=400,SAVE_MS=1000,LENGTH_NOTICE_MS=1000;
const uint8_t RANGE_VOLTS[3]={1,2,5};

enum Page : uint8_t {PLAY,EDIT,SETUP};
struct Step {uint16_t a,b;bool gate;};
struct Button {bool raw,stable,rise,fall;uint32_t changed;};
Step steps[16];
Page page=PLAY,returnPage=PLAY;
uint8_t length=16,rangeIndex=0,selected=0,cvAddress=0;
bool addressMode=false,cvHigh=false,gateOutput=false;
uint16_t cvRaw=0;
uint32_t lastCvUs=0,lastPotMs=0,lengthNoticeAt=0,cvNoticeAt=0;
bool lengthNotice=false,cvNotice=false;
uint16_t priorPot[3],motionPot[3];
uint8_t acquired=0;
Button modeButton,manualButton;
uint8_t chord=0;
bool modeArmed=false,modeLong=false;
uint32_t chordAt=0,modeAt=0;

uint16_t zeroCode[2]={2048,2048},plusCode[2]={87,87},minusCode[2]={4008,4008};
uint16_t lastDac[2]={65535,65535};

static bool elapsed(uint32_t now,uint32_t then,uint32_t interval){return (uint32_t)(now-then)>=interval;}
static uint16_t wordAt(const uint8_t *r){return r[0]|((uint16_t)r[1]<<8);}
static uint16_t crc16(const uint8_t *r,uint8_t n){
  uint16_t crc=0xffff;
  while(n--){
    crc^=(uint16_t)*r++<<8;
    for(uint8_t b=0;b<8;b++)crc=(uint16_t)((crc<<1)^((crc&0x8000)?0x1021:0));
  }
  return crc;
}

static bool validCal(const uint8_t *r){
  if(r[0]!='A'||r[1]!='C'||r[2]!='A'||r[3]!='L'||r[4]!=1||r[21]!=0xa5||
     wordAt(r+19)!=crc16(r,19))return false;
  for(uint8_t c=0;c<2;c++){
    uint16_t z=wordAt(r+7+2*c),p=wordAt(r+11+2*c),n=wordAt(r+15+2*c);
    if(z<1024||z>3072||p>=z-512||n<=z+512||n>4095)return false;
  }
  return true;
}
static void loadCalibration(){
  uint8_t r[2][22];
  for(uint8_t b=0;b<2;b++)for(uint8_t i=0;i<22;i++)r[b][i]=EEPROM.read(256+24*b+i);
  bool a=validCal(r[0]),b=validCal(r[1]);
  if(!a&&!b)return;
  uint16_t diff=wordAt(r[1]+5)-wordAt(r[0]+5);
  uint8_t bank=!a||(b&&diff&&diff<32768);
  for(uint8_t c=0;c<2;c++){
    zeroCode[c]=wordAt(r[bank]+7+2*c);
    plusCode[c]=wordAt(r[bank]+11+2*c);
    minusCode[c]=wordAt(r[bank]+15+2*c);
  }
}
static void dacWriteRaw(uint8_t c,uint16_t v){
  uint16_t w=(c?0x9000U:0x1000U)|v;
  SPI.beginTransaction(SPISettings(8000000UL,MSBFIRST,SPI_MODE0));
  digitalWrite(PIN_CS,LOW);SPI.transfer(w>>8);SPI.transfer(w&255);digitalWrite(PIN_CS,HIGH);
  SPI.endTransaction();
}
static uint16_t dacCode(uint8_t c,uint16_t value){
  uint16_t z=zeroCode[c],r=RANGE_VOLTS[rangeIndex];
  // Step values are normalized, with a true zero and both endpoints.
  if(value<=512)return z+(uint32_t)(z<minusCode[c]?minusCode[c]-z:0)*(512-value)*r/(512UL*5);
  return z-(uint32_t)(z-plusCode[c])*(value-512)*r/(511UL*5);
}
static void outputStep(){
  const Step &s=steps[selected];
  uint16_t a=dacCode(0,s.a),b=dacCode(1,s.b);
  bool dirty=false;
  if(a!=lastDac[0]){lastDac[0]=a;dacWriteRaw(0,a);dirty=true;}
  if(b!=lastDac[1]){lastDac[1]=b;dacWriteRaw(1,b);dirty=true;}
  if(dirty){
    digitalWrite(PIN_LDAC,LOW);
#if defined(__AVR__)
    __builtin_avr_delay_cycles(F_CPU/1000000UL);
#endif
    digitalWrite(PIN_LDAC,HIGH);
  }
  // A new high gate must never arrive before its CVs have settled on the DAC.
  gateOutput=s.gate;digitalWrite(PIN_IO,gateOutput?HIGH:LOW);
}

// Each step takes 21 bits (10 + 10 + gate), packed in three EEPROM bytes.
// Banks 190..248 and 940..998 do not overlap the other ALTERIA firmwares.
const uint16_t BANK_ADDR[2]={190,940};
const uint8_t RECORD_BYTES=59;
uint8_t writeRecord[RECORD_BYTES],bank=1,sequence=0,feedback=0;
int8_t writeStep=-1;
uint32_t feedbackAt=0;

static bool validRecord(const uint8_t *r){
  if(r[0]!='A'||r[1]!='P'||r[2]!=1||r[58]!=0xa5||r[4]<1||r[4]>16||
     r[5]>2||r[6]>1||r[7]>=r[4]||wordAt(r+56)!=crc16(r,56))return false;
  for(uint8_t i=0;i<16;i++)if(r[10+3*i]&0xe0)return false;
  return true;
}
static void loadBank(){
  for(uint8_t i=0;i<16;i++)steps[i]={512,512,false};
  uint8_t r[2][RECORD_BYTES];
  for(uint8_t b=0;b<2;b++)for(uint8_t i=0;i<RECORD_BYTES;i++)r[b][i]=EEPROM.read(BANK_ADDR[b]+i);
  bool a=validRecord(r[0]),b=validRecord(r[1]);
  if(!a&&!b)return;
  uint8_t diff=r[1][3]-r[0][3];bank=!a||(b&&diff&&diff<128);
  const uint8_t *p=r[bank];sequence=p[3];length=p[4];rangeIndex=p[5];addressMode=p[6];selected=p[7];
  for(uint8_t i=0;i<16;i++){
    const uint8_t *q=p+8+3*i;
    steps[i]={(uint16_t)(q[0]|((uint16_t)(q[1]&3)<<8)),
              (uint16_t)((q[1]>>2)|((uint16_t)(q[2]&15)<<6)),(bool)(q[2]&16)};
  }
}
static void startSave(){
  if(writeStep>=0)return;
  uint8_t *r=writeRecord;
  r[0]='A';r[1]='P';r[2]=1;r[3]=sequence+1;r[4]=length;
  r[5]=rangeIndex;r[6]=addressMode;r[7]=selected;
  for(uint8_t i=0;i<16;i++){
    uint8_t *p=r+8+3*i;const Step &s=steps[i];
    p[0]=s.a;p[1]=(s.a>>8)|((s.b&63)<<2);p[2]=(s.b>>6)|(s.gate?16:0);
  }
  uint16_t crc=crc16(r,56);r[56]=crc;r[57]=crc>>8;r[58]=0xa5;
  writeStep=0;
}
static void serviceSave(uint32_t now){
  if(writeStep<0||!eeprom_is_ready())return;
  uint16_t addr=BANK_ADDR[bank^1];
  if(writeStep<=RECORD_BYTES){
    uint8_t i=writeStep>0&&writeStep<RECORD_BYTES?writeStep-1:RECORD_BYTES-1;
    EEPROM.update(addr+i,writeStep?writeRecord[i]:0);writeStep++;return;
  }
  bool same=true;
  for(uint8_t i=0;i<RECORD_BYTES;i++)if(EEPROM.read(addr+i)!=writeRecord[i])same=false;
  if(same){bank^=1;sequence=writeRecord[3];}
  writeStep=-1;feedback=same?1:2;feedbackAt=now;
}

static uint8_t sector(uint16_t adc,uint8_t count){return adc*count/1024;}
static uint16_t boundary(uint8_t i,uint8_t count){return (1024U*i+count-1)/count;}
static uint8_t stableSector(uint16_t adc,uint8_t count,uint8_t current){
  uint8_t next=sector(adc,count);
  if(next>current&&adc<boundary(current+1,count)+4)return current;
  if(next<current&&adc+4>=boundary(current,count))return current;
  return next;
}
static void syncCv(){
  cvAddress=sector(cvRaw,length);cvHigh=cvRaw>=388;
}
static void armPots(){
  acquired=0;
  for(uint8_t i=0;i<3;i++)priorPot[i]=motionPot[i]=analogRead(POT_PINS[i]);
}
static void selectStep(uint8_t step){
  selected=step;outputStep();
  if(page==EDIT)armPots();
}
static void nextStep(){selectStep(selected+1==length?0:selected+1);}
static void setPage(uint8_t next){
  page=(Page)next;armPots();syncCv();lengthNotice=false;
}
static void pollCv(uint32_t nowUs,uint32_t nowMs,bool manual){
  bool change=false;uint8_t next=0;
  if(elapsed(nowUs,lastCvUs,CV_POLL_US)){
    lastCvUs=nowUs;cvRaw=analogRead(PIN_CV);
    if(addressMode){
      next=stableSector(cvRaw,length,cvAddress);
      change=next!=cvAddress;cvAddress=next;
    }else{
      if(!cvHigh&&cvRaw>=388){cvHigh=true;change=true;}
      if(cvHigh&&cvRaw<=225)cvHigh=false;
    }
  }
  if(page==PLAY&&change){
    if(addressMode)selectStep(next);else nextStep();
    cvNoticeAt=nowMs;cvNotice=true;
  }else if(manual&&page!=SETUP)nextStep();
}
static bool crossed(uint16_t previous,uint16_t next,uint16_t target){
  return (previous<target&&next>=target)||(previous>target&&next<=target);
}
static void pollPots(uint32_t now){
  if(page==PLAY||!elapsed(now,lastPotMs,POT_POLL_MS))return;
  lastPotMs=now;
  bool setupMotion[3]={false,false,false},dirty=false;
  for(uint8_t i=0;i<3;i++){
    uint16_t raw=analogRead(POT_PINS[i]),prior=priorPot[i];
    priorPot[i]=raw;
    bool moved=abs((int)raw-(int)motionPot[i])>=POT_MOVE;
    if(moved)motionPot[i]=raw;
    uint8_t bit=1U<<i;
    if(page==SETUP){
      uint8_t count=i==0?16:i==1?3:2;
      uint8_t value=i==0?length-1:i==1?rangeIndex:addressMode;
      uint8_t target=sector(raw,count);
      if(target==value||crossed(sector(prior,count),target,value))acquired|=bit;
      setupMotion[i]=moved;
      if(!(acquired&bit)||!moved)continue;
      uint8_t next=stableSector(raw,count,value);
      if(next==value)continue;
      if(i==0){length=next+1;if(selected>=length)selected=length-1;syncCv();dirty=true;}
      if(i==1){rangeIndex=next;dirty=true;}
      if(i==2){addressMode=next;syncCv();}
    }else{
      Step &s=steps[selected];
      if(i==2){
        if(s.gate?raw>604:raw<420)acquired|=bit;
        if(acquired&bit){
          bool next=raw>604?true:raw<420?false:s.gate;
          if(s.gate!=next){s.gate=next;dirty=true;}
        }
      }else{
        uint16_t &value=i==0?s.a:s.b;
        if(abs((int)raw-(int)value)<=4||crossed(prior,raw,value))acquired|=bit;
        if((acquired&bit)&&moved&&raw!=value){value=raw;dirty=true;}
      }
    }
  }
  if(setupMotion[0]){lengthNoticeAt=now;lengthNotice=true;}
  if(setupMotion[1]||setupMotion[2])lengthNotice=false;
  if(dirty)outputStep();
}

static void scanButton(Button &b,bool raw,uint32_t now){
  b.rise=b.fall=false;
  if(raw!=b.raw){b.raw=raw;b.changed=now;}
  if(raw!=b.stable&&elapsed(now,b.changed,25)){
    b.stable=raw;b.rise=raw;b.fall=!raw;
  }
}
static bool serviceButtons(uint32_t now){
  scanButton(modeButton,digitalRead(PIN_MODE)==HIGH,now);
  scanButton(manualButton,digitalRead(PIN_MANUAL)==HIGH,now);
  if(!chord&&modeButton.raw&&manualButton.raw){
    chord=1;chordAt=now;modeArmed=false;
  }
  if(chord){
    if(chord==1){
      if(!modeButton.raw||!manualButton.raw)chord=2;
      else if(modeButton.stable&&manualButton.stable&&elapsed(now,chordAt,CHORD_MS)){
        if(page==SETUP)setPage(returnPage);
        else {returnPage=page;setPage(SETUP);}
        chord=2;
      }
    }
    if(!modeButton.raw&&!manualButton.raw&&!modeButton.stable&&!manualButton.stable)chord=0;
    return false;
  }
  if(modeButton.rise){modeArmed=true;modeLong=false;modeAt=now;}
  if(modeArmed&&modeButton.stable&&!modeLong&&elapsed(now,modeAt,SAVE_MS)){modeLong=true;startSave();}
  if(modeArmed&&modeButton.fall){
    modeArmed=false;
    if(!modeLong)setPage(page==SETUP?returnPage:page==PLAY?EDIT:PLAY);
  }
  return manualButton.rise;
}

static uint8_t setupLeds(){return (8U>>rangeIndex)|(addressMode?1:0);}
static void display(uint32_t now){
  uint8_t bits=selected;
  if(page==SETUP)bits=lengthNotice&&!elapsed(now,lengthNoticeAt,LENGTH_NOTICE_MS)?length-1:setupLeds();
  if(writeStep>=0)bits=(now/100)&1?15:0;
  else if(feedback&&!elapsed(now,feedbackAt,650))bits=feedback==1||((now/100)&1)?15:0;
  for(uint8_t i=0;i<4;i++)digitalWrite(PIN_LED1+i,(bits>>(3-i))&1);
  digitalWrite(PIN_CV_LED,page==EDIT||(page==PLAY&&cvNotice&&!elapsed(now,cvNoticeAt,35)));
  digitalWrite(PIN_IO_LED,gateOutput);
}

void setup(){
  pinMode(PIN_MODE,INPUT);pinMode(PIN_MANUAL,INPUT);pinMode(PIN_CV,INPUT);
  pinMode(PIN_CV_LED,OUTPUT);pinMode(PIN_IO_LED,OUTPUT);
  for(uint8_t i=0;i<4;i++)pinMode(PIN_LED1+i,OUTPUT);
  digitalWrite(PIN_IO,LOW);pinMode(PIN_IO,OUTPUT);
  pinMode(PIN_CS,OUTPUT);digitalWrite(PIN_CS,HIGH);SPI.begin();
  digitalWrite(PIN_LDAC,HIGH);pinMode(PIN_LDAC,OUTPUT);
  uint32_t now=millis();
  bool m=digitalRead(PIN_MODE)==HIGH,n=digitalRead(PIN_MANUAL)==HIGH;
  modeButton={m,m,false,false,now};manualButton={n,n,false,false,now};
  chord=m&&n?2:0;
  loadCalibration();loadBank();armPots();
  cvRaw=analogRead(PIN_CV);syncCv();lastCvUs=micros();lastPotMs=now;
  outputStep();display(now);
}

void loop(){
  uint32_t now=millis();bool manual=serviceButtons(now);
  pollCv(micros(),now,manual);pollPots(now);serviceSave(now);display(now);
}
