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
  calManualArmed=calModeArmed=false;modePressArmed=false;
  cvLevelQ12=normalizeCv(analogRead(PIN_CV_INPUT));
  cvGate=cvLevelQ12>=CV_GATE_ON_Q12;cvRising=cvFalling=false;lastCvPollUs=micros();
#if !ALTERIA_DEBUG_SERIAL
  Serial.end();
#endif
  enterSlot(0);
}
static void serviceCalBoot(uint32_t now) {
  if(digitalRead(PIN_MODE_BUTTON)!=HIGH||digitalRead(PIN_MANUAL_BUTTON)!=HIGH) {
    calBootPending=false;enterSlot(0);return;
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
