#include <cassert>
#include <cstdio>
#include "../ALTERIA_ALLROUNDER_2_2_1.ino"
static void tick(uint32_t us){clockUs=us;loop();}
int main(){
  setup();adc[PIN_POT_1]=1023;adc[PIN_POT_2]=1023;adc[PIN_POT_3]=0;
  pot1=1023;pot2=1023;pot3=0;ctrl1Val=pot1;ctrl2Val=pot2;ctrl3Val=pot3;
  cvGate=false;manualBtn.stable=false;enterMode(MODE_TURING);
  resetTriggered=false;modeTuring(0);modeJustChanged=false;
  assert(output1Q12==0&&output2Q12==0&&!pins[PIN_OUT3]&&dirs[PIN_OUT3]==OUTPUT);
  uint16_t expected=0xACE1;int highBits=0,lowBits=0,consecutiveHigh=0;bool previousBit=false;
  for(int n=0;n<64;n++){
    expected=(expected>>1)|((expected&1U)?0x8000:0);
    cvGate=true;resetTriggered=true;modeTuring(n*100);
    assert(output1Q12==((uint32_t)expected*4095/65535));
    assert(output2Q12==((expected&1U)?4095:0));assert(pins[PIN_OUT3]);
    assert(dacLastB==((expected&1U)?DAC_PLUS5_CODE[1]:DAC_ZERO_CODE[1]));
    if(expected&1U){highBits++;if(previousBit)consecutiveHigh++;}else lowBits++;
    previousBit=expected&1U;
    uint16_t a=output1Q12,b=output2Q12;
    for(int k=0;k<3;k++){modeTuring(n*100+10+k);assert(output1Q12==a&&output2Q12==b&&pins[PIN_OUT3]);}
    cvGate=false;modeTuring(n*100+50);assert(!pins[PIN_OUT3]&&output2Q12==b);
  }
  assert(highBits&&lowBits&&consecutiveHigh);
  // I/O follows the OR of levels regardless of the held register bit.
  manualBtn.stable=true;modeTuring(7000);assert(pins[PIN_OUT3]);
  cvGate=true;manualBtn.stable=false;modeTuring(7010);assert(pins[PIN_OUT3]);
  cvGate=false;modeTuring(7020);assert(!pins[PIN_OUT3]);
  // Real loop paths: one register advance per edge, no repeat on held input.
  manualBtn.raw=manualBtn.stable=false;cvGate=false;adc[PIN_CV_INPUT]=0;
  enterMode(MODE_TURING);tick(100000);expected=0xACE1;
  for(int n=0;n<32;n++){
    uint32_t t=200000+n*10000;
    adc[PIN_CV_INPUT]=1023;tick(t);expected=(expected>>1)|((expected&1U)?0x8000:0);
    uint16_t a=output1Q12,b=output2Q12;assert(a==((uint32_t)expected*4095/65535)&&pins[2]);
    tick(t+1000);tick(t+2000);assert(output1Q12==a&&output2Q12==b&&pins[2]);
    adc[PIN_CV_INPUT]=300;tick(t+3000);assert(pins[2]); // Schmitt deadband
    adc[PIN_CV_INPUT]=0;tick(t+5000);assert(!pins[2]&&output2Q12==b);
  }
  // MANUAL is a held gate with a single advance after debounce.
  pins[PIN_MANUAL_BUTTON]=HIGH;tick(600000);tick(620000);
  expected=(expected>>1)|((expected&1U)?0x8000:0);
  assert(pins[2]&&output1Q12==((uint32_t)expected*4095/65535));
  uint16_t a=output1Q12;tick(650000);assert(output1Q12==a&&pins[2]);
  adc[PIN_CV_INPUT]=1023;tick(660000);expected=(expected>>1)|((expected&1U)?0x8000:0);
  pins[PIN_MANUAL_BUTTON]=LOW;tick(670000);tick(690000);assert(pins[2]);
  adc[PIN_CV_INPUT]=0;tick(700000);assert(!pins[2]);
  // Gate copying works even when less than one old 1ms dispatch tick elapsed.
  lastUpdateUs=clockUs;adc[PIN_CV_INPUT]=1023;tick(700600);assert(pins[2]);
  assert(output1Q12==((uint32_t)((expected>>1)|((expected&1U)?0x8000:0))*4095/65535));
  enterMode(MODE_AD);assert(!pins[2]&&output2Q12==0);
  puts("PASS: Turing CV preserved, raw held bit OUT2, I/O OR gate, clock/manual single edges, held levels, ADC hysteresis, fast dispatch, calibrated bit range, safe exit");
}
