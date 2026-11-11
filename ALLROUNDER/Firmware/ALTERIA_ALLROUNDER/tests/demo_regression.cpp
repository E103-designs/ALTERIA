#include <cassert>
#include <cstdio>
#include "../ALTERIA_ALLROUNDER.ino"
static void timeAt(uint32_t us){clockUs=us;}
static void tick(uint32_t us){timeAt(us);loop();}
static void edgeMode(bool rising,bool falling,bool held,uint32_t ms){
  modeBtn.rising=rising;modeBtn.falling=falling;modeBtn.stable=held;serviceModeButton(ms);
}
int main(){
  setup();
  assert(DAC_ZERO_CODE[0]==2048 && pins[12]==HIGH);
  for(int m=0;m<16;m++){
    bool bipolar=false;
    enterMode((EngineMode)m);assert(bipolarOutputs==bipolar);
    assert(dirs[2]==((m==2)?INPUT:OUTPUT));
    for(uint8_t ch=0;ch<2;ch++) {
      if(!bipolar){assert(outputToDacCode(ch,0)==2048);assert(outputToDacCode(ch,4095)==87);}
      else{assert(outputToDacCode(ch,0)==4008);assert(outputToDacCode(ch,2048)==2048);assert(outputToDacCode(ch,4095)==87);}
    }
    displayModes(0);int bits=0;for(int i=0;i<4;i++)bits=(bits<<1)|(pins[6+i]?1:0);
    assert(bits==m);
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
  edgeMode(false,true,false,200);assert(mode==MODE_SH_GLIDE);
  edgeMode(true,false,true,300);edgeMode(false,false,true,951);assert(mode==MODE_SH_GLIDE);
  edgeMode(false,true,false,1000);assert(mode==MODE_SH_GLIDE);
  edgeMode(true,false,true,1900);edgeMode(false,true,false,2000);assert(mode==MODE_AD);
  enterMode(MODE_LOGIC_XOR);edgeMode(true,false,true,2100);edgeMode(false,false,true,2800);edgeMode(false,true,false,2900);assert(mode==MODE_LOGIC_XOR);
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
  enterMode(MODE_TREMOLO);bipolarOutputs=true;out1(0);out2(0);
  assert(dacLastA==DAC_MINUS5_CODE[0]);enterMode(MODE_GRIDS);
  assert(dacLastA==DAC_ZERO_CODE[0]&&dacLastB==DAC_ZERO_CODE[1]);
  puts("PASS: 16 modes/ranges, GPIO, LEDs, Envelope, UI, scheduler, 24 Trance patterns x 16 steps, unipolar rhythm transitions");
}
