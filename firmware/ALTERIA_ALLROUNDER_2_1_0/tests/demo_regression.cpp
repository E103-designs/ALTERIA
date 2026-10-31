#include <cassert>
#include <cstdio>
#include "../ALTERIA_ALLROUNDER_2_1_0.ino"
static void timeAt(uint32_t us){clockUs=us;}
static void tick(uint32_t us){timeAt(us);loop();}
static void finishSave(){for(int i=0;i<80;i++)servicePresetSave();assert(saveStep<0);}
static void edgeMode(bool rising,bool falling,bool held,uint32_t ms){
  modeBtn.rising=rising;modeBtn.falling=falling;modeBtn.stable=held;serviceModeButton(ms);
}
int main(){
  setup();
  assert(DAC_ZERO_CODE[0]==2048 && pins[12]==HIGH);
  for(int m=0;m<16;m++){
    bool bipolar=m==15;
    enterMode((EngineMode)m);assert(bipolarOutputs==bipolar);
    assert(dirs[2]==((m==2||m==9)?INPUT:OUTPUT));
    for(uint8_t ch=0;ch<2;ch++) {
      if(!bipolar){assert(outputToDacCode(ch,0)==2048);assert(outputToDacCode(ch,4095)==87);}
      else{assert(outputToDacCode(ch,0)==4008);assert(outputToDacCode(ch,2048)==2048);assert(outputToDacCode(ch,4095)==87);}
    }
    displayModes(0);int bits=0;for(int i=0;i<4;i++)bits|=(pins[6+i]?1:0)<<i;
    assert(bits==(m==15?1:((m+1)&15)));
    uint32_t t=1000000+(uint32_t)m*100000;
    for(int k=0;k<12;k++)tick(t+k*1000);
    assert(output1Q12<=4095 && output2Q12<=4095);
  }
  pot1=pot2=0;shapeSector=5;enterMode(MODE_AD);
  serviceAD(10000,true);serviceAD(12000,false);assert(adValue==4095 && adEor.active);
  serviceAD(14000,false);assert(adValue==0 && adEof.active);
  enterMode(MODE_AR);cvGate=true;serviceAR(20000,true);serviceAR(22000,false);assert(arValue==4095);
  cvGate=false;serviceAR(23000,false);serviceAR(25000,false);assert(arValue==0&&arEor.active);
  enterMode(MODE_DUAL_DECAY);serviceDualDecay(30000,true,false,1);assert(output1Q12>0&&output2Q12==0);
  serviceDualDecay(31000,false,true,1);assert(output2Q12>0);
  enterMode(MODE_DUCKING);cvLevelQ12=0;manualBtn.stable=false;serviceDucking(40000,1);assert(output1Q12==4095&&output2Q12==0);
  // Short MODE action on release; a long press must never also advance.
  enterMode(MODE_TRANCE);edgeMode(true,false,true,100);assert(mode==MODE_TRANCE);
  edgeMode(false,true,false,200);assert(mode==MODE_PRESETS);
  edgeMode(true,false,true,300);edgeMode(false,false,true,951);assert(presetEditing&&mode==MODE_PRESETS);
  edgeMode(false,true,false,1000);assert(mode==MODE_PRESETS&&presetEditing);
  edgeMode(true,false,true,1100);edgeMode(false,false,true,1750);assert(!presetEditing);
  edgeMode(false,true,false,1800);assert(mode==MODE_PRESETS);
  edgeMode(true,false,true,1900);edgeMode(false,true,false,2000);assert(mode==MODE_AD);
  enterMode(MODE_LOGIC_XOR);edgeMode(true,false,true,2100);edgeMode(false,false,true,2800);edgeMode(false,true,false,2900);assert(mode==MODE_LOGIC_XOR);
  // MANUAL advances exactly once per debounced press, in play and edit.
  enterMode(MODE_PRESETS);selectedPreset=0;
  manualBtn.raw=manualBtn.stable=manualBtn.rising=manualBtn.falling=false;cvGate=false;adc[PIN_CV_INPUT]=0;
  pins[PIN_MANUAL_BUTTON]=HIGH;tick(4000000);tick(4020000);assert(selectedPreset==1);
  tick(4050000);assert(selectedPreset==1);
  pins[PIN_MANUAL_BUTTON]=LOW;tick(4060000);tick(4080000);
  presetEditing=true;resetPickup();pins[PIN_MANUAL_BUTTON]=HIGH;tick(4100000);tick(4120000);assert(selectedPreset==2);
  adc[PIN_CV_INPUT]=1023;tick(4140000);assert(selectedPreset==2); // external CV ignored while editing
  presetEditing=false;adc[PIN_CV_INPUT]=0;tick(4160000);adc[PIN_CV_INPUT]=1023;tick(4180000);assert(selectedPreset==3);
  // ADC boundary 1023 makes every last bank reachable with useful width.
  assert((896UL*8/1024)==7 && (768UL*4/1024)==3);
  // Pickup prevents a jump on entry and releases on crossing the saved value.
  enterMode(MODE_PRESETS);presetEditing=true;selectedPreset=0;
  presets[0].a=2048;presets[0].b=2048;presets[0].gate=0;
  pot1=100;pot2=100;pot3=100;resetPickup();
  pot1=200;servicePresetMode();assert(presets[0].a==2048);
  pot1=600;servicePresetMode();assert(pickup[0]&&presets[0].a==((uint32_t)600*4095/1023));
  // First commit, second commit, corruption fallback and incomplete-write fallback.
  presetEditing=false;startPresetSave();finishSave();
  uint16_t first=presets[0].a;uint8_t firstBank=activeBank;
  presets[0].a=1234;presetDirty=true;startPresetSave();finishSave();assert(activeBank!=firstBank);
  loadPresets();assert(presets[0].a==1234);
  EEPROM.bytes[EEPROM_BASE+activeBank*32+7]^=1;
  loadPresets();assert(presets[0].a==first);
  presets[0].a=3333;presetDirty=true;startPresetSave();
  for(int i=0;i<12;i++)servicePresetSave();
  saveStep=-1;presetDirty=false;loadPresets();assert(presets[0].a==first);
  // Microsecond schedule is relative to micros(), not millis()*1000 (wrap domains).
  enterMode(MODE_GRIDS);timeAt(5000000);
  schPulseA(millis(),0,2,4095);schPulseB(millis(),0,2,4095);schPulseG(millis(),0,2);
  schService(millis());assert(schOn[0]&&schOn[1]&&schOn[2]);
  timeAt(5003000);schService(millis());assert(!schOn[0]&&!schOn[1]&&!schOn[2]);
  // All 24 banks are reachable, unique and contain playable steps.
  const uint16_t originalMain[8]={0xAAAA,0x8888,0xEEEE,0x8E8E,0xA5A5,0xF0F0,0xDCDC,0xBABA};
  const uint16_t originalAccent[8]={0x8888,0x8080,0x8888,0x8282,0x8484,0x8080,0x9090,0x8A28};
  bool reached[TRANCE_PATTERN_COUNT]={false};
  for(uint16_t p=0;p<=1023;p++) {uint8_t i=trancePatternIndex(p);assert(i<TRANCE_PATTERN_COUNT);reached[i]=true;}
  for(uint8_t i=0;i<TRANCE_PATTERN_COUNT;i++) {
    assert(reached[i]);
    uint16_t main=pgm_read_word(&TRANCE_MAIN[i]),accent=pgm_read_word(&TRANCE_ACCENT[i]);
    assert(main!=0);
    if(i<8){assert(main==originalMain[i]);assert(accent==originalAccent[i]);}
    else assert((accent & ~main)==0);
    for(uint8_t j=0;j<i;j++)assert(main!=pgm_read_word(&TRANCE_MAIN[j]));
    pot1=(uint16_t)(((uint32_t)(2*i+1)*1024)/(2*TRANCE_PATTERN_COUNT));pot2=0;pot3=1023;
    enterMode(MODE_TRANCE);
    for(uint8_t step=0;step<16;step++) {
      uint32_t at=6000000UL+i*2000000UL+step*100000UL;timeAt(at);
      modeJustChanged=(step==0);resetTriggered=true;modeTranceGate(millis());modeJustChanged=false;
      assert((output1Q12==4095)==((main>>step)&1));
      assert((pins[PIN_OUT3]==HIGH)==((accent>>step)&1));
      assert(outputToDacCode(0,output1Q12)<=DAC_ZERO_CODE[0]);
      assert(outputToDacCode(1,output2Q12)<=DAC_ZERO_CODE[1]);
      timeAt(at+90000);schService(millis());
      assert(output1Q12==0&&output2Q12==0&&pins[PIN_OUT3]==LOW);
    }
  }
  // Leaving a bipolar mode for a rhythmic mode clears held voltages to 0 V.
  enterMode(MODE_PRESETS);out1(0);out2(0);
  assert(dacLastA==DAC_MINUS5_CODE[0]);enterMode(MODE_GRIDS);
  assert(dacLastA==DAC_ZERO_CODE[0]&&dacLastB==DAC_ZERO_CODE[1]);
  puts("PASS: 16 modes/ranges, GPIO, LEDs, Envelope, UI, pickup, EEPROM, scheduler, 24 Trance patterns x 16 steps, unipolar rhythm transitions");
}
