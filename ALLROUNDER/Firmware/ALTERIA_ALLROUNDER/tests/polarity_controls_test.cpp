#include <cassert>
#include <cstdio>
#include "../ALTERIA_ALLROUNDER.ino"

static void tick(uint32_t us) {clockUs=us;loop();}
static void noProcessTick(uint32_t us) {lastUpdateUs=us;tick(us);}
static void manualPress(uint32_t us) {
  pins[PIN_MANUAL_BUTTON]=HIGH;noProcessTick(us);noProcessTick(us+20000);
}
static void manualRelease(uint32_t us) {
  pins[PIN_MANUAL_BUTTON]=LOW;noProcessTick(us);noProcessTick(us+20000);
}

static unsigned xorEdges(uint16_t p,uint8_t ch) {
  pot1=pot2=p;pot3=0;enterMode(MODE_LOGIC_XOR);
  unsigned edges=0;uint16_t previous=0;
  for(uint32_t t=0;t<=2000;t++){
    clockUs=t*1000;resetTriggered=false;modeLogicXOR(t);modeJustChanged=false;
    uint16_t value=ch?output2Q12:output1Q12;
    if(value!=previous)edges++;
    previous=value;
  }
  return edges;
}

static uint16_t burstSpacing(uint16_t p) {
  pot1=p;pot2=1023;pot3=512;enterMode(MODE_BURST);
  bool previous=false;int first=-1;
  for(uint32_t t=0;t<1300;t++){
    clockUs=t*1000;resetTriggered=t==0;modeBurst(t);modeJustChanged=false;
    bool value=pins[PIN_OUT3];
    if(value&&!previous){
      if(first>=0)return t-first;
      first=t;
    }
    previous=value;
  }
  assert(false);return 0;
}

int main() {
  setup();
  // Full ADC sweeps: rates increase CW, numeric duration values increase CW.
  uint16_t previousXor=2000,previousBurst=600;
  uint32_t previousDuration=0,previousLfo=0;
  uint8_t previousRatio=0,previousFlip=100;
  for(uint16_t p=0;p<=1023;p++){
    uint16_t x=periodFromSpeedPotMs(p,10,2000);
    uint16_t b=periodFromSpeedPotMs(p,8,600);
    uint32_t duration=timeFromPotMs(p,2,5000);
    uint32_t lfo=mapQuadraticTo(p,2,2200);
    uint8_t ratio=clockIndex(p),flip=turingFlipPct(p);
    assert(x<=previousXor&&x>=10);
    assert(b<=previousBurst&&b>=8);
    assert(duration>=previousDuration&&duration<=5000);
    assert(lfo>=previousLfo&&lfo<=2200);
    assert(ratio>=previousRatio&&ratio<=10);
    assert(flip<=previousFlip&&flip<=100);
    previousXor=x;previousBurst=b;previousDuration=duration;previousLfo=lfo;
    previousRatio=ratio;previousFlip=flip;
  }
  assert(periodFromSpeedPotMs(0,10,2000)==2000);
  assert(periodFromSpeedPotMs(1023,10,2000)==10);
  assert(burstSpacing(0)==600&&burstSpacing(1023)==8);
  assert(xorEdges(0,0)<xorEdges(512,0)&&xorEdges(512,0)<xorEdges(1023,0));
  assert(xorEdges(0,1)<xorEdges(512,1)&&xorEdges(512,1)<xorEdges(1023,1));
  assert(turingFlipPct(511)==50&&turingFlipPct(512)==50);
  assert(turingFlipPct(205)>50&&turingFlipPct(205)<100);
  assert(turingFlipPct(818)>0&&turingFlipPct(818)<50);
  // CW recirculates the same bits; CCW recirculates their complements.
  for(uint16_t p: {uint16_t(0),uint16_t(1023)}){
    pot1=p;pot2=1023;pot3=0;enterMode(MODE_TURING);
    uint16_t reference=0xACE1;
    for(uint8_t i=0;i<64;i++){
      bool bit=reference&1;
      if(!p)bit=!bit;
      reference=(reference>>1)|(bit?0x8000:0);
      resetTriggered=true;modeTuring(i);modeJustChanged=false;
      assert(output1Q12==(uint32_t)reference*4095/65535);
      assert(output2Q12==((reference&1)?4095:0));
    }
  }
  for(uint16_t p=0;p<=1023;p++){
    if(p&&map(p,0,1023,1,16)==map(p-1,0,1023,1,16))continue;
    pot1=0;pot2=p;pot3=0;enterMode(MODE_TURING);
    uint8_t length=map(p,0,1023,1,16);
    uint16_t mask=(1UL<<length)-1,initial=0xACE1&mask;
    for(uint8_t i=1;i<=2*length;i++){
      resetTriggered=true;modeTuring(i);modeJustChanged=false;
      if(i==length)assert((turingRegister&mask)==(initial^mask));
      if(i==2*length)assert((turingRegister&mask)==initial);
    }
  }
  // Real input scanning/debounce, no processing tick during range toggle.
  adc[PIN_POT_1]=pot1=700;adc[PIN_POT_2]=pot2=1023;adc[PIN_POT_3]=pot3=0;
  adc[PIN_CV_INPUT]=0;cvGate=false;manualBtn={PIN_MANUAL_BUTTON,18,false,false,false,false,0};
  pins[PIN_MANUAL_BUTTON]=LOW;pins[PIN_MODE_BUTTON]=LOW;
  clockUs=1000000;enterMode(MODE_TREMOLO);tick(1001000);
  assert(!bipolarOutputs&&dirs[PIN_OUT3]==OUTPUT);
  out1(0);out2(4095);latchDac();
  assert(dacLastA==DAC_ZERO_CODE[0]&&dacLastB==DAC_PLUS5_CODE[1]);
  unsigned writes=EEPROM.writes;
  manualPress(1010000);
  assert(bipolarOutputs&&output1Q12==0&&output2Q12==4095);
  assert(dacLastA==DAC_MINUS5_CODE[0]&&dacLastB==DAC_PLUS5_CODE[1]);
  assert(!pendingTrigger&&!schOn[CH_G]&&!schCount[CH_G]);
  noProcessTick(1060000);assert(bipolarOutputs); // Holding does not repeat.
  manualRelease(1070000);assert(bipolarOutputs);
  // P2=0: unipolar midpoint is +2.5 V; bipolar midpoint is calibrated zero.
  out1(2048);out2(2048);assert(dacLastA==DAC_ZERO_CODE[0]&&dacLastB==DAC_ZERO_CODE[1]);
  manualPress(1100000);assert(!bipolarOutputs&&output1Q12==2048&&output2Q12==2048);
  assert(dacLastA==physical5toDacCode(0,2048)&&dacLastB==physical5toDacCode(1,2048));
  manualRelease(1130000);
  // Sub-debounce bounce cannot switch range.
  pins[PIN_MANUAL_BUTTON]=HIGH;noProcessTick(1160000);
  pins[PIN_MANUAL_BUTTON]=LOW;noProcessTick(1165000);noProcessTick(1190000);
  assert(!bipolarOutputs);
  manualPress(1200000);manualRelease(1230000);assert(bipolarOutputs);
  // CV reset is still available and never changes the selected range.
  adc[PIN_CV_INPUT]=1023;tick(1270000);
  assert(bipolarOutputs&&output1Q12==0&&output2Q12==4095);
  assert(schOn[CH_G]);
  adc[PIN_CV_INPUT]=0;tick(1271000);
  bool negativeA=false,positiveA=false,negativeB=false,positiveB=false;
  for(uint32_t t=1272000;t<1472000;t+=1000){
    tick(t);
    negativeA|=dacLastA>DAC_ZERO_CODE[0];positiveA|=dacLastA<DAC_ZERO_CODE[0];
    negativeB|=dacLastB>DAC_ZERO_CODE[1];positiveB|=dacLastB<DAC_ZERO_CODE[1];
    assert(output1Q12<=4095&&output2Q12<=4095);
    assert(dacLastA>=DAC_PLUS5_CODE[0]&&dacLastA<=DAC_MINUS5_CODE[0]);
    assert(dacLastB>=DAC_PLUS5_CODE[1]&&dacLastB<=DAC_MINUS5_CODE[1]);
  }
  assert(negativeA&&positiveA&&negativeB&&positiveB);
  assert(EEPROM.writes==writes);
  enterMode(MODE_GRIDS);assert(!bipolarOutputs);
  assert(dacLastA==DAC_ZERO_CODE[0]&&dacLastB==DAC_ZERO_CODE[1]);
  enterMode(MODE_TREMOLO);assert(!bipolarOutputs);
  tick(1480000);assert(!bipolarOutputs);
  // Entering with MANUAL already held cannot toggle.
  manualBtn.raw=manualBtn.stable=true;pins[PIN_MANUAL_BUTTON]=HIGH;
  enterMode(MODE_TREMOLO);tick(1490000);assert(!bipolarOutputs);
  manualRelease(1500000);manualPress(1530000);assert(bipolarOutputs);
  enterMode(MODE_SH_GLIDE);assert(!bipolarOutputs);
  enterMode(MODE_AD);assert(!bipolarOutputs&&dacLastA==DAC_ZERO_CODE[0]);
  puts("PASS: pot sweeps, Burst/XOR speeds, Turing double-lock/50% midpoint/CW lock, 16 lengths, Tremolo range toggle and safe mode transitions");
}
