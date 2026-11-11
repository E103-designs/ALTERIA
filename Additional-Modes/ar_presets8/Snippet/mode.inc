PresetSlot presets[8];
uint8_t selectedPreset = 0;
bool presetEditing = false, presetDirty = false;
bool pickup[3] = {false,false,false};
uint16_t previousPot[3];
const uint16_t MODE_HOLD_MS = 650;
const uint16_t EEPROM_BASE = 0;
const uint8_t RECORD_BYTES = 50, RECORD_STRIDE = 56;
uint8_t activeBank = 1, saveBank = 0;
uint16_t presetSequence = 0;
uint8_t saveRecord[RECORD_BYTES];
int8_t saveStep = -1;

bool presetSaveError=false;
static uint16_t presetCrcN(const uint8_t *p,uint8_t n){
  uint16_t c=0xFFFF;
  for(uint8_t i=0;i<n;i++){c^=(uint16_t)p[i]<<8;for(uint8_t b=0;b<8;b++)c=c&0x8000?(uint16_t)((c<<1)^0x1021):(uint16_t)(c<<1);}
  return c;
}
static bool writeRecordStep(uint16_t address,const uint8_t *record,uint8_t size,int8_t &step){
  if(!eeprom_is_ready())return false;
  if(step>size){step=-1;return true;}
  // Invalidate the destination, write the body, then commit last.
  uint8_t offset=step>0&&step<size?step-1:size-1;
  EEPROM.update(address+offset,step?record[offset]:0);
  step++;return false;
}
static bool recordMatches(uint16_t address,const uint8_t *record,uint8_t size){
  for(uint8_t i=0;i<size;i++)if(EEPROM.read(address+i)!=record[i])return false;
  return true;
}
static uint16_t recordCrc(const uint8_t *p){return presetCrcN(p,47);}
static bool presetValidN(const uint8_t *r,uint8_t count,uint8_t version){
  uint8_t end=7+count*5;uint16_t crc=presetCrcN(r,end);
  if(r[0]!='A'||r[1]!='D'||r[2]!='M'||r[3]!='O'||r[4]!=version||r[end+2]!=0xA5)return false;
  if(r[end]!=(uint8_t)crc||r[end+1]!=(uint8_t)(crc>>8))return false;
  for(uint8_t i=0;i<count;i++){uint8_t p=7+i*5;if(r[p+1]>15||r[p+3]>15||r[p+4]>1)return false;}
  return true;
}
static bool validRecord(const uint8_t *r){return presetValidN(r,8,2);}
static void loadPresets(){
  for(uint8_t i=0;i<8;i++){presets[i].a=i*4095U/7U;presets[i].b=4095-presets[i].a;presets[i].gate=i&1;}
  uint8_t r[2][RECORD_BYTES];
  for(uint8_t b=0;b<2;b++)for(uint8_t i=0;i<RECORD_BYTES;i++)r[b][i]=EEPROM.read(EEPROM_BASE+b*RECORD_STRIDE+i);
  bool a=validRecord(r[0]),b=validRecord(r[1]);activeBank=1;presetSequence=0;presetSaveError=false;
  uint8_t count=8;
  if(!a&&!b){
    for(uint8_t bank=0;bank<2;bank++)for(uint8_t i=0;i<30;i++)r[bank][i]=EEPROM.read(128+bank*32+i);
    a=presetValidN(r[0],4,1);b=presetValidN(r[1],4,1);count=4;
    if(!a&&!b)return;
  }
  uint16_t sa=r[0][5]|((uint16_t)r[0][6]<<8),sb=r[1][5]|((uint16_t)r[1][6]<<8);
  uint8_t bank=!a?1:!b?0:((int16_t)(sb-sa)>0?1:0);
  if(count==8){activeBank=bank;presetSequence=bank?sb:sa;}
  for(uint8_t i=0;i<count;i++){uint8_t p=7+i*5;presets[i].a=r[bank][p]|((uint16_t)r[bank][p+1]<<8);presets[i].b=r[bank][p+2]|((uint16_t)r[bank][p+3]<<8);presets[i].gate=r[bank][p+4];}
}
static void startPresetSave(){
  if(!presetDirty||saveStep>=0||presetSaveError)return;
  saveBank=activeBank^1;uint16_t seq=presetSequence+1;
  saveRecord[0]='A';saveRecord[1]='D';saveRecord[2]='M';saveRecord[3]='O';saveRecord[4]=2;saveRecord[5]=seq;saveRecord[6]=seq>>8;
  for(uint8_t i=0;i<8;i++){uint8_t p=7+i*5;saveRecord[p]=presets[i].a;saveRecord[p+1]=presets[i].a>>8;saveRecord[p+2]=presets[i].b;saveRecord[p+3]=presets[i].b>>8;saveRecord[p+4]=presets[i].gate;}
  uint16_t crc=recordCrc(saveRecord);saveRecord[47]=crc;saveRecord[48]=crc>>8;saveRecord[49]=0xA5;
  presetDirty=false;saveStep=0;
}
static void servicePresetSave(){
  if(saveStep<0){if(!presetEditing)startPresetSave();return;}
  uint16_t addr=EEPROM_BASE+saveBank*RECORD_STRIDE;
  if(writeRecordStep(addr,saveRecord,RECORD_BYTES,saveStep)){
    if(recordMatches(addr,saveRecord,RECORD_BYTES)){activeBank=saveBank;presetSequence=saveRecord[5]|((uint16_t)saveRecord[6]<<8);}
    else{presetSaveError=true;presetDirty=true;}
  }
}

static void resetPickup() {
  uint16_t target[2]={(uint16_t)((uint32_t)presets[selectedPreset].a*1023/4095),
                      (uint16_t)((uint32_t)presets[selectedPreset].b*1023/4095)};
  previousPot[0]=pot1;previousPot[1]=pot2;previousPot[2]=pot3;
  pickup[0]=abs((int)pot1-(int)target[0])<=8;
  pickup[1]=abs((int)pot2-(int)target[1])<=8;
  pickup[2]=presets[selectedPreset].gate ? pot3>604 : pot3<420;
}
static void servicePresetMode() {
  if (!presetEditing) return;
  uint16_t values[2]={pot1,pot2};
  uint16_t *dest[2]={&presets[selectedPreset].a,&presets[selectedPreset].b};
  for(uint8_t i=0;i<2;i++) {
    uint16_t target=(uint32_t)*dest[i]*1023/4095;
    if (!pickup[i] && (abs((int)values[i]-(int)target)<=8 ||
        (previousPot[i]<target && values[i]>=target) || (previousPot[i]>target && values[i]<=target))) pickup[i]=true;
    if (pickup[i] && values[i]!=previousPot[i]) {
      uint16_t v=(uint32_t)values[i]*4095/1023;
      if (v!=*dest[i]) { *dest[i]=v;presetDirty=true; }
    }
    previousPot[i]=values[i];
  }
  uint8_t &gate=presets[selectedPreset].gate;
  if (!pickup[2] && (gate ? pot3>604 : pot3<420)) pickup[2]=true;
  if (pickup[2]) {
    uint8_t next=pot3>604 ? 1 : pot3<420 ? 0 : gate;
    if(next!=gate) {gate=next;presetDirty=true;}
  }
  previousPot[2]=pot3;
}
static void outputPreset() {
  out1(presets[selectedPreset].a);out2(presets[selectedPreset].b);setGate(presets[selectedPreset].gate);
}
