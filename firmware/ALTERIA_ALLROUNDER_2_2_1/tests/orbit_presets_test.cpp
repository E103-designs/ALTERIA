#include <cassert>
#include <cstring>
#include <cstdio>
#include "../ALTERIA_ALLROUNDER_2_2_1.ino"
static void at(uint32_t ms){clockUs=ms*1000;}
static void finish(){for(int i=0;i<60&&saveStep>=0;i++)servicePresetSave();assert(saveStep<0);}
static void legacy(uint8_t bank,uint16_t seq){
  uint8_t r[30]={'A','D','M','O',1,(uint8_t)seq,(uint8_t)(seq>>8)};
  for(int i=0;i<4;i++){uint16_t a=400+i*200+bank,b=3000-i*200;int p=7+i*5;r[p]=a;r[p+1]=a>>8;r[p+2]=b;r[p+3]=b>>8;r[p+4]=i&1;}
  uint16_t crc=presetCrcN(r,27);r[27]=crc;r[28]=crc>>8;r[29]=0xA5;
  memcpy(EEPROM.bytes+128+bank*32,r,30);
}
int main(){
  setup();pot1=0;pot2=512;pot3=0;at(100);enterMode(MODE_ORBIT);
  assert(dirs[PIN_OUT3]==OUTPUT&&!bipolarOutputs&&orbitStep==0);modeOrbit(100,false);assert(orbitStep==0&&!pins[PIN_OUT3]);
  uint8_t original[16];memcpy(original,orbitNotes,16);uint16_t rhythm=orbitRhythm,accents=orbitAccents;
  for(int n=0;n<128;n++){
    uint32_t now=200+n*100;at(now);modeOrbit(now,true);assert(orbitStep==((n+1)&15));
    assert(!pins[PIN_OUT3]);assert(output1Q12<=4095&&output2Q12<=4095);
    at(now+2);modeOrbit(now+2,false);assert(bool(pins[PIN_OUT3])==bool(rhythm&((uint16_t)1<<(n&15))));
    at(now+90);modeOrbit(now+90,false);assert(!pins[PIN_OUT3]);
  }
  assert(!memcmp(original,orbitNotes,16)&&rhythm==orbitRhythm&&accents==orbitAccents);
  // Pitch belongs to each scale at every root/range, including last ADC sector.
  for(int scale=0;scale<4;scale++)for(int root=0;root<12;root++)for(int range: {0,512,1023})for(int degree=0;degree<=14;degree++){
    orbitScale=scale;orbitRoot=root;pot2=range;uint16_t v=orbitPitch(degree);int semi=((uint32_t)v*60+2047)/4095;
    int pitch=(semi-root)%12;bool found=false;for(int d=0;d<(scale<2?5:7);d++)if(pgm_read_byte(&ORBIT_SCALES[scale][d])==pitch)found=true;
    assert(found&&v<=4095);
  }
  // Many generations/evolutions remain bounded and preserve the cadence.
  pot3=1023;bool changed=false;
  for(int i=0;i<1000;i++){orbitGenerate();memcpy(original,orbitNotes,16);orbitEvolve();if(memcmp(original,orbitNotes,16))changed=true;for(int j=0;j<16;j++)assert(orbitNotes[j]<=14);assert(orbitNotes[0]==0&&orbitNotes[15]==0&&(orbitRhythm&0x8001)==0x8001);}
  assert(changed);
  // Short MANUAL queues a theme, long changes scale without queuing a theme.
  at(20000);enterMode(MODE_ORBIT);orbitStep=4;pot3=0;
  manualBtn.rising=manualBtn.stable=true;modeOrbit(20000,false);manualBtn.rising=false;
  manualBtn.falling=true;manualBtn.stable=false;modeOrbit(20100,false);manualBtn.falling=false;assert(orbitNewTheme);
  at(20200);modeOrbit(20200,true);assert(orbitNewTheme);orbitStep=0;at(20300);modeOrbit(20300,true);assert(!orbitNewTheme);
  uint8_t before=orbitNextScale;manualBtn.rising=manualBtn.stable=true;modeOrbit(21000,false);manualBtn.rising=false;modeOrbit(21650,false);assert(orbitNextScale==((before+1)&3));
  manualBtn.falling=true;manualBtn.stable=false;modeOrbit(21700,false);manualBtn.falling=false;assert(!orbitNewTheme);
  // Rollover: first clock near millis wrap, second after it.
  orbitHaveClock=false;orbitStep=4;orbitRhythm=0;at(0xfffffff0);modeOrbit(0xfffffff0,true);at(84);modeOrbit(84,true);assert(orbitPeriod==100);
  memset(EEPROM.bytes,255,1024);legacy(0,65534);legacy(1,65535);unsigned writes=EEPROM.writes;loadPresets();assert(presets[0].a==401&&presets[3].a==1001&&EEPROM.writes==writes);
  uint8_t old[1024];memcpy(old,EEPROM.bytes,1024);
  // Interrupt first migration save at every boundary before commit.
  for(int stop=0;stop<=50;stop++){
    memcpy(EEPROM.bytes,old,1024);saveStep=-1;loadPresets();presets[7].a=2222;presetDirty=true;startPresetSave();
    for(int k=0;k<stop;k++)servicePresetSave();saveStep=-1;loadPresets();assert(presets[0].a==401&&presets[7].a==4095);
  }
  presets[7].a=2222;presetDirty=true;startPresetSave();finish();assert(!presetSaveError);loadPresets();assert(presets[7].a==2222&&presets[0].a==401);
  uint8_t snapshot[1024];memcpy(snapshot,EEPROM.bytes,1024);uint8_t firstBank=activeBank;
  for(int stop=0;stop<=50;stop++){
    memcpy(EEPROM.bytes,snapshot,1024);saveStep=-1;loadPresets();presets[7].a=1111;presetDirty=true;startPresetSave();for(int i=0;i<stop;i++)servicePresetSave();saveStep=-1;loadPresets();assert(presets[7].a==2222);
  }
  presets[7].a=1111;presetDirty=true;startPresetSave();finish();assert(activeBank!=firstBank);loadPresets();assert(presets[7].a==1111);
  EEPROM.bytes[EEPROM_BASE+activeBank*RECORD_STRIDE+7]^=1;loadPresets();assert(presets[7].a==2222);
  EEPROM.blockWrites=true;presets[7].a=123;presetDirty=true;startPresetSave();finish();assert(presetSaveError&&presetDirty);unsigned w=EEPROM.writes;servicePresetSave();assert(saveStep==-1&&w==EEPROM.writes);EEPROM.blockWrites=false;
  for(int i=112;i<1024;i++)assert(EEPROM.bytes[i]==old[i]);
  // All eight slot identities, edit flag does not erase slot bits.
  presetSaveError=false;enterMode(MODE_PRESETS);
  for(int i=0;i<8;i++){
    selectedPreset=i;presetEditing=false;displayModes(0);int bits=0;for(int j=0;j<4;j++)bits|=pins[6+j]<<j;assert(bits==(8|i));
    presetEditing=true;displayModes(140);assert(!pins[9]);for(int j=0;j<3;j++)assert(pins[6+j]==((i>>j)&1));
  }
  puts("PASS: ORBIT scale/root/range, locked motifs, bounded evolution, cadence, manual actions, gate ordering/timeout, rollover; eight presets, legacy migration, 102 interrupted writes, CRC/readback, LED identities and EEPROM isolation");
}
