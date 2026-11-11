#include <cassert>
#include <cstdio>
#include <cstring>
#include "../ALTERIA_FULL_VOICE.ino"
static void at(uint32_t ms){clockUs=ms*1000;}
static void cleanButtons(){modeBtn.rising=modeBtn.falling=manualBtn.rising=manualBtn.falling=false;}
static void releaseStartup(uint32_t ms){manualBtn.stable=modeBtn.stable=false;cleanButtons();serviceCalibration(ms);assert(!calNeedRelease);}
static void finishCalSave(uint32_t ms){for(int i=0;i<30&&calWriteStep>=0;i++)serviceCalSave(ms+i);assert(calWriteStep<0);}
static void testData(uint16_t delta){
  calWork[0][0]=2068+delta;calWork[0][1]=2064+delta;
  calWork[1][0]=114+delta;calWork[1][1]=126+delta;
  calWork[2][0]=4032-delta;calWork[2][1]=3996-delta;
}
int main(){
  setup();assert(!calBootPending&&!calibrationActive&&voiceActive&&page==0&&!calHasStored);
  uint8_t presetsBefore[64];for(int i=0;i<64;i++)presetsBefore[i]=EEPROM.bytes[128+i];
  // A single held button must not enter calibration.
  pins[PIN_MODE_BUTTON]=HIGH;pins[PIN_MANUAL_BUTTON]=LOW;setup();assert(!calBootPending);
  // Both held at power-on start a 100 ms qualification with I/O still INPUT.
  at(0);pins[PIN_MODE_BUTTON]=pins[PIN_MANUAL_BUTTON]=HIGH;setup();assert(calBootPending&&dirs[PIN_OUT3]==INPUT);
  serviceCalBoot(99);assert(!calibrationActive);serviceCalBoot(100);assert(calibrationActive&&calNeedRelease);
  // Releasing the boot chord never confirms point zero.
  pins[PIN_MODE_BUTTON]=pins[PIN_MANUAL_BUTTON]=LOW;releaseStartup(120);assert(calStage==0);
  pot1=0;pot2=1023;serviceCalibration(150);assert(!calReady[0]&&!calReady[1]);assert(calWork[0][0]==2048);
  pot1=pot2=512;serviceCalibration(160);assert(calReady[0]&&calReady[1]);
  // CW raises physical voltage: one code lower on this inverting hardware.
  pot1=1023;serviceCalibration(500);assert(calWork[0][0]==2047);
  for(uint32_t t=600;t<30000;t+=50)serviceCalibration(t);assert(calWork[0][0]==calAnchor[0][0]-256);
  uint16_t held=calWork[0][0];pot1=512;serviceCalibration(30100);assert(calWork[0][0]==held);
  // Stage confirmation requires both knobs at rest in the center deadband.
  pot1=1023;manualBtn.falling=true;calManualArmed=true;serviceCalibration(30200);assert(calStage==0);cleanButtons();
  pot1=pot2=512;manualBtn.falling=true;calManualArmed=true;serviceCalibration(30300);assert(calStage==1);cleanButtons();
  // Back a point, without EEPROM writes.
  unsigned writes=EEPROM.writes;modeBtn.falling=true;calModeArmed=true;serviceCalibration(30400);assert(calStage==0&&EEPROM.writes==writes);cleanButtons();
  // Cancel restores active coefficients and returns to normal mode 1.
  modeBtn.rising=true;modeBtn.stable=true;serviceCalibration(31000);cleanButtons();serviceCalibration(32200);
  assert(!calibrationActive&&DAC_ZERO_CODE[0]==2048&&voiceActive&&page==0&&EEPROM.writes==writes);
  // First confirmed commit persists six points and never touches preset space.
  startCalibration(33000);releaseStartup(33010);testData(0);calStage=3;
  manualBtn.rising=true;manualBtn.stable=true;serviceCalibration(33100);cleanButtons();serviceCalibration(34099);assert(calWriteStep<0&&!calSaved);
  serviceCalibration(34100);assert(calWriteStep==0);finishCalSave(34200);assert(calSaved&&calHasStored&&DAC_ZERO_CODE[0]==2068);
  uint8_t firstBank=calBank;loadCalibration();assert(DAC_PLUS5_CODE[1]==126&&DAC_MINUS5_CODE[0]==4032);
  for(int i=0;i<64;i++)assert(EEPROM.bytes[128+i]==presetsBefore[i]);
  // Reloaded calibration changes every normal output conversion endpoint.
  startNormal();assert(voiceZeroA==2068&&voicePos==(2068-114)/2&&voiceEnvSpan==2064-126);
  assert(voiceZeroA==2068&&voiceZeroB==2064);
  // A second deliberate chord resets working defaults only, not stored data.
  startCalibration(34500);releaseStartup(34510);modeBtn.stable=manualBtn.stable=true;serviceCalibration(34600);serviceCalibration(37600);
  assert(calNeedRelease&&calWork[0][0]==2048&&DAC_ZERO_CODE[0]==2068);releaseStartup(37610);calExit();loadCalibration();assert(DAC_ZERO_CODE[0]==2068);
  // A failed write is not applied to active coefficients.
  startCalibration(35000);releaseStartup(35010);testData(10);calStage=3;EEPROM.blockWrites=true;
  calStartSave(35020);finishCalSave(35030);assert(calError&&!calSaved&&DAC_ZERO_CODE[0]==2068);EEPROM.blockWrites=false;
  // Interrupt a new save at every pre-commit byte boundary: old bank survives.
  uint8_t snapshot[1024];memcpy(snapshot,EEPROM.bytes,1024);
  for(int interrupted=0;interrupted<=22;interrupted++){
    memcpy(EEPROM.bytes,snapshot,1024);loadCalibration();calSaved=false;testData(10);calStartSave(36000);
    for(int k=0;k<interrupted;k++)serviceCalSave(36001+k);
    calWriteStep=-1;loadCalibration();assert(DAC_ZERO_CODE[0]==2068&&calBank==firstBank);
  }
  // Second committed bank wins; corrupted newest record falls back to first.
  testData(10);calSaved=false;calStartSave(37000);finishCalSave(37010);assert(calBank!=firstBank&&DAC_ZERO_CODE[0]==2078);
  uint8_t secondBank=calBank;loadCalibration();assert(DAC_ZERO_CODE[0]==2078);
  EEPROM.bytes[CAL_BASE+secondBank*CAL_STRIDE+7]^=1;loadCalibration();assert(DAC_ZERO_CODE[0]==2068);
  // Confirming an unchanged stored calibration causes no EEPROM writes.
  for(uint8_t s=0;s<3;s++)for(uint8_t ch=0;ch<2;ch++)calWork[s][ch]=calActivePoint(s,ch);
  writes=EEPROM.writes;calSaved=false;calStartSave(38000);assert(calSaved&&calWriteStep<0&&EEPROM.writes==writes);
  // Invalid or erased records recover theoretical defaults, with no auto-save.
  EEPROM.bytes[CAL_BASE+firstBank*CAL_STRIDE+21]=0;loadCalibration();assert(!calHasStored&&DAC_ZERO_CODE[0]==2048);
  // A short boot chord abandoned before 100ms returns to normal, not hidden UI.
  calibrationActive=false;calBootPending=true;at(40000);pins[PIN_MODE_BUTTON]=LOW;serviceCalBoot(40000);assert(!calBootPending&&!calibrationActive);
  // Exercise the complete three-point UI without skipping any stage.
  startCalibration(41000);releaseStartup(41010);pot1=pot2=512;
  for(uint8_t point=0;point<3;point++) {
    uint32_t t=42000+point*100;
    cleanButtons();serviceCalibration(t);assert(calReady[0]&&calReady[1]);
    assert(dacLastA==CAL_FACTORY[point][0]&&dacLastB==CAL_FACTORY[point][1]);
    manualBtn.rising=true;manualBtn.stable=true;serviceCalibration(t+10);cleanButtons();
    manualBtn.falling=true;manualBtn.stable=false;serviceCalibration(t+20);cleanButtons();
    assert(calStage==point+1);
  }
  manualBtn.rising=true;manualBtn.stable=true;serviceCalibration(43000);cleanButtons();serviceCalibration(44000);
  assert(calWriteStep==0);finishCalSave(44010);serviceCalibration(45000);assert(!calibrationActive&&voiceActive&&page==0);
  puts("PASS: boot chord, input-safe calibration, no-jump pots, rate/bounds/direction, three points, review/hold-save, cancel/back, six-point persistence, readback failure, every interrupted write, CRC fallback, preset isolation, no-change wear avoidance");
}
