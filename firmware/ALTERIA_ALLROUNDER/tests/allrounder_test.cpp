#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>
#include "../ALTERIA_ALLROUNDER.ino"
static void at(uint32_t us){clockUs=us;}
static void clockAt(uint32_t us,bool ext=false,bool tap=false){at(us);serviceClock3(us,ext,tap);}
static std::vector<int> drumTake(uint8_t mutation,uint8_t channel){
 pot1=200;pot2=600;pot3=(uint16_t)mutation*4;drumsStart();drumRng=0xACE1;drumsTrigger(true,true);
 std::vector<int> samples;
 for(int i=0;i<2048;i++){drumAudioTick();samples.push_back((int)drumWritten[channel]-drumZero[channel]);}
 return samples;
}
int main(){
 setup();
 // Ratios /8, x1, x8, all three positive and at 50% duty.
 pot1=0;pot2=512;pot3=1023;clockPeriodUs=500000;enterMode(MODE_DIVMULT);clockAt(0);
 assert(clockLane[0].halfUs==2000000&&clockLane[1].halfUs==250000&&clockLane[2].halfUs==31250);
 assert(clockLane[0].high&&clockLane[1].high&&clockLane[2].high);
 clockAt(31250);assert(!clockLane[2].high&&clockLane[1].high);
 clockAt(62500);assert(clockLane[2].high);
 clockAt(250000);assert(!clockLane[1].high&&clockLane[0].high);
 clockAt(2000000);assert(!clockLane[0].high);
 // No tempo update before three taps; mean of last two intervals afterwards.
 enterMode(MODE_DIVMULT);clockAt(3000000);clockAt(3100000,false,true);clockAt(3600000,false,true);assert(clockPeriodUs==500000);
 clockAt(4200000,false,true);assert(clockPeriodUs==550000&&clockTapCount==3);
 // External priority, divider phase across beats and continuation after stop.
 clockAt(5000000,true);clockAt(5500000,true);assert(clockExternal&&clockPeriodUs==500000&&clockBeat==1);
 clockAt(5510000,true);assert(clockBeat==1); // reject a spuriously fast double edge
 clockAt(5600000,false,true);assert(clockTapCount==0&&clockPeriodUs==500000);
 clockAt(7100000);assert(!clockExternal&&clockBeat==4&&clockPeriodUs==500000);
 // Micros rollover and uniform pot bins.
 modeJustChanged=true;clockAt(0xfff00000U);clockAt(0xfff40000U);clockAt(0x00100000U);assert(clockPeriodUs==500000);
 assert(clockIndex(0)==0&&clockIndex(1023)==10);
 // Acid empty/full ranges, notes in Phrygian, bounded octaves, repeatability.
 enterMode(MODE_ACID);pot1=0;pot2=pot3=1023;acidGenerate();for(auto n:acidNotes)assert(n==255);
 pot1=1023;pot2=0;acidGenerate();for(auto n:acidNotes)assert(n==255||n==0);
 pot2=1023;acidGenerate();uint8_t copy[16];memcpy(copy,acidNotes,16);bool any=false;
 for(auto n:acidNotes)if(n!=255){any=true;assert(n<=34);bool scale=false;for(int i=0;i<7;i++)if(n%12==ACID_SCALE[i])scale=true;assert(scale);}assert(any);
 modeJustChanged=false;acidStep=0;acidValue=0;acidHaveClock=false;
 for(uint32_t t=1000;t<3000;t+=125){modeAcid(t,true);assert(output1Q12<=4095&&output2Q12<=4095);}
 assert(!memcmp(copy,acidNotes,16));modeAcid(8000,false);assert(!acidGate);
 // Explicit slide between adjacent notes holds the gate and glides pitch.
 for(auto &n:acidNotes)n=255;acidNotes[0]=0;acidNotes[1]=12;acidSlides=1;acidAccents=2;acidStep=0;acidPreviousSlide=false;acidGate=false;acidHaveClock=false;acidValue=0;
 modeAcid(10000,true);assert(acidGate&&acidValue==0);modeAcid(10125,true);assert(acidGlideMs>0&&acidGate&&acidValue==0&&output2Q12==4095);
 modeAcid(10145,false);assert(acidValue>0&&acidValue<acidTarget);
 modeAcid(10170,false);assert(acidValue==acidTarget);
 // Burst 16 pulses, complementary ramps, no forced bipolar output.
 enterMode(MODE_BURST);pot1=0;pot2=1023;pot3=512;resetTriggered=true;at(20000000);modeBurst(millis());modeJustChanged=false;
 assert(!bipolarOutputs&&output1Q12==4095&&output2Q12==0);
 at(20140000);modeBurst(millis());assert(output1Q12==0&&output2Q12==4095);
 // Drum start/stop and two truly independent voices.
 enterMode(MODE_DRUMS);assert(drumAudioActive&&dirs[2]==INPUT);drumAudioTick();assert(drumWritten[0]==DAC_ZERO_CODE[0]&&drumWritten[1]==DAC_ZERO_CODE[1]);
 pot1=200;pot2=600;pot3=0;drumsControls(30000,true);drumsTrigger(true,false);bool kickMoved=false;
 for(int i=0;i<1000;i++){drumAudioTick();kickMoved|=drumWritten[0]!=drumZero[0];assert(drumWritten[1]==drumZero[1]);}assert(kickMoved);
 drumsStart();drumsTrigger(false,true);bool snareMoved=false;
 for(int i=0;i<1000;i++){drumAudioTick();snareMoved|=drumWritten[1]!=drumZero[1];assert(drumWritten[0]==drumZero[0]);}assert(snareMoved);
 for(int i=0;i<30000;i++)drumAudioTick();assert(drumKAmp==0&&drumSAmp==0&&drumWritten[0]==drumZero[0]&&drumWritten[1]==drumZero[1]);
 // Mutation, overlapping retriggers, calibrated bounds and signed arithmetic.
 for(int m=0;m<=255;m+=17){pot3=m*4;drumsControls(31000,true);drumsTrigger(true,true);for(int i=0;i<2000;i++){drumAudioTick();for(int ch=0;ch<2;ch++)assert(drumWritten[ch]>=drumZero[ch]-drumPos[ch]&&drumWritten[ch]<=drumZero[ch]+drumNeg[ch]);}}
 // All P3 steps must vary decay smoothly, including the old 127/128 switch.
 uint16_t previousK=0,previousS=0;
 for(int m=0;m<=255;m++){
  pot3=m*4;drumsStart();drumsTrigger(true,true);drumAudioTick();
  if(m){assert(drumKAmp>=previousK&&drumKAmp-previousK<=1);assert(drumSAmp>=previousS&&drumSAmp-previousS<=1);}
  previousK=drumKAmp;previousS=drumSAmp;
 }
 // Deterministic renders: no waveform discontinuity at former drive thresholds.
 for(int ch=0;ch<2;ch++)for(int boundary: {64,128,192}){
  const auto a=drumTake(boundary-1,ch),b=drumTake(boundary,ch);double energy=0,error=0;
  for(size_t i=0;i<a.size();i++){energy+=(double)a[i]*a[i];error+=(double)(b[i]-a[i])*(b[i]-a[i]);}
  assert(energy>0&&error<energy*0.01); // Relative RMS difference below 10%.
 }
 // Snare body always ends before the noise tail; P3 never detunes its resonators.
 for(int tone: {0,512,1023}){
  pot2=tone;pot3=0;drumsStart();uint16_t f1=drumSnareInc,f2=drumSnareInc2;
  pot3=1023;drumsControls(32000,true);assert(drumSnareInc==f1&&drumSnareInc2==f2);
  drumsTrigger(false,true);int ticks=0;do{drumAudioTick();assert(++ticks<5000);}while(drumSBodyAmp);
  assert(drumSAmp>0);
  for(int i=0;i<30000;i++)drumAudioTick();assert(drumSAmp==0&&drumWritten[1]==drumZero[1]);
 }
 enterMode(MODE_GRIDS);assert(!drumAudioActive&&dirs[2]==OUTPUT&&dacLastA==DAC_ZERO_CODE[0]);
 // A changed calibration is used on starting audio, including silence.
 DAC_ZERO_CODE[0]=2068;DAC_PLUS5_CODE[0]=114;DAC_MINUS5_CODE[0]=4032;enterMode(MODE_DRUMS);drumAudioTick();assert(drumWritten[0]==2068&&drumPos[0]==977);
 puts("PASS: clocks/tap/rollover, Acid, Burst, independent drum voices/range/calibration, smooth P3/drive thresholds, short snare body/noise tail, safe transitions");
}
