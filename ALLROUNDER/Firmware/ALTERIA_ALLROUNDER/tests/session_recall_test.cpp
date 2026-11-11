#include <cassert>
#include <cstdio>
#include "../ALTERIA_ALLROUNDER.ino"

static void boot(uint16_t a=0,uint16_t b=0,uint16_t c=0,bool bypass=false,bool manual=false) {
  // Reset the RAM fields normally reset by the MCU before calling setup().
  mode=MODE_AD;modeJustChanged=true;
  sessionStep=-1;sessionChord=restoredPots=sessionFeedback=0;
  clockPeriodUs=500000;
  modePressArmed=modeLongHandled=calibrationActive=calBootPending=false;
  adc[PIN_POT_1]=a;adc[PIN_POT_2]=b;adc[PIN_POT_3]=c;
  adc[PIN_CV_INPUT]=0;pins[PIN_MODE_BUTTON]=bypass;pins[PIN_MANUAL_BUTTON]=manual;
  clockUs=0;setup();
}
static void run(uint32_t from,uint32_t to){
  for(uint32_t ms=from;ms<=to;ms++){clockUs=ms*1000;loop();}
}
static void finish(){
  for(uint8_t i=0;i<SESSION_BYTES+2;i++)serviceSession(i);
  assert(sessionStep<0);
}
static void snapshot(){startSessionSave();finish();assert(sessionFeedback==1);}

int main(){
  boot(1023,1023,0);
  assert(mode==MODE_AD&&EEPROM.writes==0);
  enterMode(MODE_TURING);run(1,5);
  const uint16_t before=turingRegister;
  // Short chord cancels both normal button actions without saving.
  pins[PIN_MODE_BUTTON]=HIGH;run(10,50);pins[PIN_MANUAL_BUTTON]=HIGH;
  run(51,350);assert(!pins[PIN_OUT3]&&turingRegister==before);
  pins[PIN_MODE_BUTTON]=pins[PIN_MANUAL_BUTTON]=LOW;run(351,410);
  assert(mode==MODE_TURING&&EEPROM.writes==0&&turingRegister==before);
  // Genuine save holds neither a Turing step nor a MANUAL gate.
  pins[PIN_MODE_BUTTON]=HIGH;run(420,461);pins[PIN_MANUAL_BUTTON]=HIGH;
  run(462,1600);
  assert(sessionFeedback==1&&turingRegister==before&&!pins[PIN_OUT3]);
  unsigned writes=EEPROM.writes;
  run(1601,2700);assert(EEPROM.writes==writes);
  for(uint16_t i=0;i<1024;i++)if(i<SESSION_BASE||i>=SESSION_BASE+SESSION_BYTES)
    assert(EEPROM.bytes[i]==255);
  pins[PIN_MODE_BUTTON]=pins[PIN_MANUAL_BUTTON]=LOW;run(2701,2760);
  assert(mode==MODE_TURING);
  uint8_t stableImage[1024];memcpy(stableImage,EEPROM.bytes,1024);

  boot(10,100,777);assert(mode==MODE_TURING&&turingRegister==before);
  assert(pot1==1023&&pot2==1023&&pot3==0&&restoredPots==7);
  assert(output1Q12==(uint32_t)before*4095/65535&&output2Q12==4095&&!pins[PIN_OUT3]);
  run(1,30);assert(restoredPots==7&&pot1==1023&&pot3==0);
  adc[PIN_POT_1]=1020;run(31,48);assert(pot1==1020&&restoredPots==6);
  adc[PIN_POT_1]=900;run(49,60);assert(pot1==900&&restoredPots==6);
  adc[PIN_POT_3]=0;run(61,75);assert(pot3==0&&restoredPots==2);
  // Next mode goes back to the physical knobs, even if one is still frozen.
  pins[PIN_MODE_BUTTON]=HIGH;run(76,119);pins[PIN_MODE_BUTTON]=LOW;run(120,165);
  assert(mode==MODE_HUMANIZER&&!restoredPots&&pot2==100);
  boot(30,30,30,true);assert(mode==MODE_AD&&restoredPots==0);
  boot();assert(mode==MODE_TURING); // bypass did not erase the snapshot
  boot(0,0,0,true,true);assert(calBootPending);run(1,120);assert(calibrationActive);

  // Interrupt every update in a second-bank write, then boot from actual bytes.
  for(uint8_t cut=0;cut<=SESSION_BYTES+1;cut++){
    memcpy(EEPROM.bytes,stableImage,1024);boot();
    enterMode(MODE_DUAL_DECAY);pot1=123;pot2=456;pot3=789;
    startSessionSave();
    for(uint8_t i=0;i<cut;i++)serviceSession(i);
    boot();
    if(cut<=SESSION_BYTES)assert(mode==MODE_TURING&&pot1==1023);
    else assert(mode==MODE_DUAL_DECAY&&pot1==123&&pot2==456&&pot3==789&&dirs[PIN_OUT3]==INPUT);
  }
  // CRC damage and failed writes keep the older record.
  EEPROM.bytes[SESSION_BASE+SESSION_STRIDE+4]^=1;
  boot();assert(mode==MODE_TURING);
  enterMode(MODE_BURST);startSessionSave();EEPROM.blockWrites=true;
  finish();EEPROM.blockWrites=false;assert(sessionFeedback==2);boot();assert(mode==MODE_TURING);
  // Options survive a reboot; running phases/gates are not replayed.
  enterMode(MODE_TREMOLO);bipolarOutputs=true;clockPeriodUs=620000;
  pot1=333;pot2=444;pot3=555;snapshot();
  boot();assert(mode==MODE_TREMOLO&&bipolarOutputs&&clockPeriodUs==620000);
  assert(pot1==333&&pot2==444&&pot3==555);
  enterMode(MODE_TREMOLO);assert(!bipolarOutputs);
  enterMode(MODE_ACID);acidGenerate();
  for(uint8_t i=0;i<16;i++)acidNotes[i]=i==2?255:i;
  acidAccents=0x1234;acidSlides=0xABCD;acidStep=9;acidGate=true;snapshot();
  boot();assert(mode==MODE_ACID&&!acidStep&&!acidGate&&acidAccents==0x1234&&acidSlides==0xABCD);
  for(uint8_t i=0;i<16;i++)assert(acidNotes[i]==(i==2?255:i));
  for (EngineMode engine : {MODE_EUCLIDEAN_TRIGS, MODE_SH_GLIDE}) {
    enterMode(engine);pot1=193;pot2=399;pot3=883;snapshot();
    boot();assert(mode==engine&&restoredPots==7&&pot1==193&&pot2==399&&pot3==883&&!pins[PIN_OUT3]);
    // Model a record saved by 2.2.4 in the same numeric slot.
    auto *old = reinterpret_cast<SessionRecord *>(&EEPROM.bytes[SESSION_BASE+sessionBank*SESSION_STRIDE]);
    old->options &= ~SESSION_CURRENT_MODES;old->crc=sessionCrc(old);
    boot();assert(mode==MODE_AD&&!restoredPots&&!pins[PIN_OUT3]);
  }
  puts("PASS: save chord isolation, one write, boot restore/pickup/bypass, calibration priority, every interrupted write, CRC/failure fallback, tempo/polarity and pattern recall");
}
