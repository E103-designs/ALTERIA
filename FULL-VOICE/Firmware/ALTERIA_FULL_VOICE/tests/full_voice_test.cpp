#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "../ALTERIA_FULL_VOICE.ino"
static void ticks(unsigned n){for(unsigned i=0;i<n;i++){voiceAudioTick();assert(voiceEnv<=32768);assert(voiceWritten[0]<=4095&&voiceWritten[1]<=4095);assert(abs(voiceLow)<=8192&&abs(voiceBandState)<=8192);}}
static void clean(){modeBtn.rising=modeBtn.falling=manualBtn.rising=manualBtn.falling=false;}
static void press(Button &b,uint32_t now){clean();b.rising=b.stable=true;serviceUI(now);clean();}
static void release(Button &b,uint32_t now){clean();b.falling=true;b.stable=false;serviceUI(now);clean();}
static void completeSave(){for(int n=0;saveStep>=0&&n<40;n++)servicePatchSave(100+n);assert(saveStep<0);}
int main(){
  setup();assert(voiceActive&&page==0&&dirs[PIN_OUT3]==INPUT&&!calibrationActive);
  assert(voiceWritten[0]==2048&&voiceWritten[1]==2048);ticks(100);assert(voiceWritten[0]==2048);
  // Nominal pitch math, independent of the physical ADC/reference accuracy.
  uint32_t prev=0;
  for(int cv=0;cv<=4095;cv++){
    uint32_t inc=pitchIncrement(cv,512);assert(inc>prev);prev=inc;
    double expected=440*pow(2.,(24-69)/12.)*pow(2.,cv*5./4095.);
    assert(fabs(1200*log2((inc*6400./4294967296.)/expected))<1);
  }
  assert(pitchIncrement(0,0)<pitchIncrement(0,512));assert(pitchIncrement(4095,1023)>pitchIncrement(4095,512));
  // Soft pickup preserves each page, with acquisition on crossing or proximity.
  selectPage(PAGE_FILTER);uint16_t saved=patch[1][0];lastPot[0]=pots[0]=0;
  applyPot(0,20,10);assert(!acquired[0]&&patch[1][0]==saved);
  applyPot(0,600,20);assert(acquired[0]&&patch[1][0]==600);
  selectPage(PAGE_ENV);assert(!acquired[0]&&patch[1][0]==600);
  // Buttons operate on release; holds suppress page changes and trigger once.
  selectPage(0);press(modeBtn,100);assert(page==0);release(modeBtn,200);assert(page==1);
  press(manualBtn,250);release(manualBtn,350);assert(page==0);
  unsigned hits=voiceHits;press(manualBtn,400);serviceUI(899);assert(voiceHits==hits);
  serviceUI(900);assert(voiceHits==hits+1);serviceUI(1500);assert(voiceHits==hits+1);release(manualBtn,1600);assert(page==0);
  // AD is finite, ignores gate duration, retriggers from its current level.
  patch[3][0]=100;patch[3][1]=300;patch[3][2]=512;
  envStage=ENV_IDLE;envLevel=0;triggerEnvelope(2000);uint16_t a=envAttackMs,d=envDecayMs;
  advanceEnvelope(2000+a/2);assert(envLevel>12000&&envLevel<20000);uint16_t old=envLevel;
  triggerEnvelope(2000+a/2);assert(envFrom==old&&envLevel==old);
  advanceEnvelope(2000+a/2+a);assert(envLevel==32768&&envStage==ENV_DECAY);
  advanceEnvelope(2000+a/2+a+d);assert(envLevel==0&&envStage==ENV_IDLE);
  // Gate input, never driven on any page. CV alone never triggers.
  startNormal();hits=voiceHits;adc[PIN_CV_INPUT]=700;clockUs=3000000;loop();assert(voiceHits==hits);
  pins[PIN_OUT3]=HIGH;clockUs+=5000;loop();assert(voiceHits==hits+1);
  for(int i=0;i<400;i++){clockUs+=1000;loop();ticks(6);}assert(voiceHits==hits+1);
  pins[PIN_OUT3]=LOW;clockUs+=1000;loop();assert(voiceHits==hits+1);
  pins[PIN_OUT3]=HIGH;startNormal();clockUs+=1000;loop();assert(voiceHits==0);pins[PIN_OUT3]=LOW;
  for(int p=0;p<4;p++){selectPage(p);assert(dirs[PIN_OUT3]==INPUT);}
  // All waves, PWM widths, filter corners, dry/enveloped VCA and drive.
  for(int wave=0;wave<4;wave++)for(int width=0;width<3;width++)for(int corner=0;corner<32;corner++){
    patch[0][0]=wave*256+128;patch[0][1]=width*511;patch[0][2]=512;
    patch[1][0]=corner&1?1023:0;patch[1][1]=corner&2?1023:0;patch[1][2]=1023;
    patch[2][0]=corner&4?1023:300;patch[2][1]=corner&8?1023:0;patch[2][2]=corner&16?1023:0;
    for(int cv: {0,800,2400,4095}){
      pitchCv=cv;envLevel=32768;publishControls();ticks(600);
      assert(voiceWritten[1]>=DAC_PLUS5_CODE[1]&&voiceWritten[1]<=DAC_ZERO_CODE[1]);
      assert(voiceWritten[0]>=voiceZeroA-voicePos&&voiceWritten[0]<=voiceZeroA+voiceNeg);
      assert(voiceWritten[1]==DAC_PLUS5_CODE[1]);
    }
  }
  // Full envelope and oscillator attenuation both close the audio exactly.
  patch[2][1]=1023;envLevel=0;publishControls();ticks(1000);assert(voiceWritten[0]==DAC_ZERO_CODE[0]&&voiceWritten[1]==DAC_ZERO_CODE[1]);
  patch[2][0]=0;patch[2][1]=0;publishControls();ticks(1000);assert(voiceWritten[0]==DAC_ZERO_CODE[0]);
  // Save snapshot, interrupted writes, CRC fallback and no cross-firmware writes.
  loadPatch();patch[0][2]=333;startPatchSave();completeSave();assert(patchStored&&!saveError);loadPatch();assert(patch[0][2]==333);
  uint8_t snapshot[1024];memcpy(snapshot,EEPROM.bytes,1024);uint8_t first=patchBank;
  for(int stop=0;stop<=34;stop++){
    memcpy(EEPROM.bytes,snapshot,1024);loadPatch();patch[0][2]=444;startPatchSave();
    for(int i=0;i<stop;i++)servicePatchSave(500+i);
    saveStep=-1;loadPatch();assert(patch[0][2]==333&&patchBank==first);
  }
  patch[0][2]=444;startPatchSave();completeSave();assert(!saveError);loadPatch();assert(patch[0][2]==444);
  EEPROM.bytes[PATCH_BASE+patchBank*PATCH_STRIDE+7]^=1;loadPatch();assert(patch[0][2]==333);
  patch[0][2]=555;EEPROM.blockWrites=true;startPatchSave();completeSave();assert(saveError);EEPROM.blockWrites=false;
  for(int i=0;i<1024;i++)if(i<PATCH_BASE||i>=PATCH_BASE+PATCH_STRIDE+PATCH_BYTES)assert(EEPROM.bytes[i]==snapshot[i]);
  saveError=false;press(modeBtn,8000);serviceUI(9200);assert(saveStep==0);release(modeBtn,9300);assert(page==3);completeSave();
  // Envelope curve continuity, extrema and rollover.
  for(int shape: {0,256,512,768,1023}){uint16_t last=0;for(int x=0;x<=32768;x++){uint16_t v=curve15(x,shape);assert(v>=last&&v<=32768);last=v;}assert(last==32768);}
  patch[3][0]=100;patch[3][1]=200;envStage=ENV_IDLE;envLevel=0;triggerEnvelope(0xfffffff0U);
  advanceEnvelope((uint32_t)(0xfffffff0U+envAttackMs+envDecayMs));assert(envStage==ENV_IDLE);
  puts("PASS: pitch, four pages/pickup, buttons/holds, finite AD/retrigger/curve/rollover, gate/CV routing, 1536 DSP corners, calibrated ranges, snapshot save/interruption/CRC/failure/isolation");
}
