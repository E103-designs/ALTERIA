#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "../ALTERIA_MACRO_VOICE.ino"

static void tick(){
  audioTick();
  for(uint8_t c=0;c<2;c++)
    assert(written[c]>=zeroCode[c]-posSpan[c]&&written[c]<=zeroCode[c]+negSpan[c]);
}

static void testCalibration(){
  for(uint8_t bank=0;bank<2;bank++){
    uint8_t r[22]={'A','C','A','L',1,bank,0,0,8,32,8,87,0,110,0,168,15,140,15,0,0,0xa5};
    if(bank)r[9]=40;
    uint16_t crc=calCrc(r);r[19]=crc;r[20]=crc>>8;
    for(uint8_t i=0;i<22;i++)EEPROM.update(256+24*bank+i,r[i]);
  }
  loadCalibration();assert(zeroCode[0]==2048&&zeroCode[1]==2088);
  EEPROM.update(256+24+21,0);
  loadCalibration();assert(zeroCode[1]==2080);
  EEPROM.update(256+21,0);
  zeroCode[1]=2048;
  posSpan[1]=negSpan[1]=980;
}

int main(){
  setup();testCalibration();
  uint32_t prev=0;
  for(uint16_t cv=0;cv<4096;cv++){
    uint32_t step=pitchStep((uint32_t)cv*15360/4095);
    assert(step>prev);
    double hz=step*(8000.0/4294967296.0),ideal=32.7031956626*pow(2.,cv*5./4095);
    assert(fabs(1200*log2(hz/ideal))<1.);
    prev=step;
  }
  for(uint8_t m=0;m<MODEL_COUNT;m++){
    for(uint16_t cv:{0,819,2048,4095}){
      for(uint16_t p1:{0,150,300,450,600,750,900,1023})
        for(uint16_t p2:{0,256,512,768,1023})for(uint16_t p3:{0,512,1023}){
          pots[0]=p1;pots[1]=p2;pots[2]=p3;pitchCv=cv;
          selectModel(m);publish(true);
          uint32_t root=config.inc[0];
          if(m==CHORD){
            assert(root==pitchStep((uint32_t)cv*15360/4095));
            for(uint8_t i=2;i<5;i++)assert(config.inc[i]>config.inc[i-1]);
            assert(config.inc[4]<0x80000000UL);
          }
          if(m==KARPLUS)assert(config.stringDelay>=6*256&&config.stringDelay<254*256);
          for(unsigned i=0;i<1000;i++)tick();
        }
    }
  }
  // Noise-picked and pure body plucks; silence on entry, retrigger, finite tail.
  selectModel(FOLD);publish(false);tick();
  for(uint16_t cv:{0,819,1638,2457,3276,4095}){
    pitchCv=cv;pots[0]=0;pots[1]=850;pots[2]=730;
    selectModel(KARPLUS);publish(false);tick();
    assert(written[0]==zeroCode[0]&&written[1]==zeroCode[1]);
    publish(true);for(int i=0;i<1000;i++)tick();
    double periods=0,previous=-1,sum=0;
    uint16_t prior=written[0];
    for(int i=0;i<1600;i++){
      tick();
      if(prior>=zeroCode[0]&&written[0]<zeroCode[0]){
        double cross=i-1.+(prior-zeroCode[0])/(double)(prior-written[0]);
        if(previous>=0){sum+=cross-previous;periods++;}
        previous=cross;
      }
      prior=written[0];
    }
    assert(periods>=4);
    double measured=8000.*periods/sum;
    double expected=config.inc[0]*(8000.0/4294967296.0);
    double cents=1200*log2(measured/expected);
    printf("Karplus %.2f Hz: simulated deviation %+.1f cent\n",expected,cents);
    assert(fabs(cents)<22);
    // CV and tap motion alone cannot move a ringing delay line.
    uint8_t length=liveLength;
    int8_t allpass=liveAllpass;
    uint16_t tap=liveTap;
    pitchCv=4095-cv;pots[2]=1;publish(false);tick();
    assert(liveLength==length&&liveAllpass==allpass&&liveTap==tap);
    pots[1]=0;publish(true);tick();
    assert(liveLength==config.stringLength&&liveAllpass==config.stringAllpass&&liveTap==config.stringTap);
    for(int i=0;i<16000;i++)tick();
    assert(written[0]==zeroCode[0]&&written[1]==zeroCode[1]);
    selectModel(FOLD);publish(false);tick();
  }
  Button b={false,false,0};
  assert(!pressed(b,true,0xfffffff0U));
  assert(!pressed(b,true,8));
  assert(pressed(b,true,9));assert(!pressed(b,true,300));
  assert(!pressed(b,false,301));assert(!pressed(b,false,326));
  puts("PASS: pitch, ACAL read/fallback, 2400 DSP corners, string pitch/retrigger/decay, debounce rollover");
}
