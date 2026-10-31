// DSP-only audition, not a recording or an analog simulation of the module.
#include <cstdio>
#include <vector>
#include "../ALTERIA_ALLROUNDER_2_1_0.ino"
static void word(FILE *f,uint32_t x,uint8_t n){while(n--){fputc(x&255,f);x>>=8;}}
int main(int argc,char **argv){
 if(argc!=2)return 1;setup();enterMode(MODE_ACID_VOICE);std::vector<int16_t> pcm;
 const uint8_t notes[16]={0,0,12,7,3,0,10,7,0,12,3,7,10,0,7,12};
 for(int step=0;step<64;step++){
  pot1=250;pot2=250+(step/16)*230;pot3=500+(step%4)*100;
  cvLevelQ12=819+(uint32_t)notes[step&15]*4095/60;voiceControls(step,true);
  voiceEvents(true,step==32);
  for(int n=0;n<750;n++){voiceAudioTick();pcm.push_back(((int)voiceZeroA-voiceWritten[0])*16);}
 }
 FILE *f=fopen(argv[1],"wb");if(!f)return 2;uint32_t bytes=pcm.size()*2;
 fwrite("RIFF",1,4,f);word(f,bytes+36,4);fwrite("WAVEfmt ",1,8,f);word(f,16,4);word(f,1,2);
 word(f,1,2);word(f,VOICE_RATE,4);word(f,VOICE_RATE*2,4);word(f,2,2);word(f,16,2);
 fwrite("data",1,4,f);word(f,bytes,4);for(int16_t s:pcm)word(f,(uint16_t)s,2);fclose(f);
 fprintf(stderr,"Rendered 7.5 seconds: saw, then square; software DSP only.\n");
}
