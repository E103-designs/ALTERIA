#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>
#include "../ALTERIA_ALLROUNDER_2_1_0.ino"
static void at(uint32_t us){clockUs=us;}
static void clockAt(uint32_t us,bool ext=false,bool tap=false){at(us);serviceClock3(us,ext,tap);}
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

 puts("PASS: unchanged clock/tap/rollover, Acid sequencer, Burst");
}
