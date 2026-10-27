// Software preview of the actual DSP, not a recording or electrical PCB model.
#include <cstdio>
#include <vector>
#include "../ALTERIA_DRUM_LAB.ino"
static std::vector<int16_t> pcm;
static void frame(bool mono=false){
 audioTick();
 int16_t a=((int)audioZero[0]-audioWritten[0])*16;
 int16_t b=mono?a:((int)audioZero[1]-audioWritten[1])*16;
 pcm.push_back(a);pcm.push_back(b);
}
static void word(FILE *f,uint32_t x,int n){while(n--){fputc(x&255,f);x>>=8;}}
int main(int argc,char **argv){
 if(argc!=2)return 1;setup();
 // 0..12 s: eight engines in order, two strikes of each.
 for(int e=0;e<8;e++){
  defaults();engine[0]=e;startAudio();
  for(int n=0;n<9600;n++){if(n==0||n==4800)trigger(1,0);frame(true);}
 }
 // 12..28 s: two bars each of four dual-voice combinations, 120 BPM.
 const uint8_t pairs[4][2]={{KICK,SNARE},{KICK,CLOSED_HAT},{TOM,FM_METAL},{PERC,CLAP}};
 for(int pair=0;pair<4;pair++){
  defaults();engine[0]=pairs[pair][0];engine[1]=pairs[pair][1];startAudio();
  for(int n=0;n<25600;n++){
   if(n%800==0){int step=n/800;uint8_t hits=0;
    if(step%4==0||((pair>=2)&&step%8==7))hits|=1;
    if(pair==1?(step%2==1):(step%8==4))hits|=2;
    trigger(hits,0);
   }
   frame();
  }
 }
 FILE *f=fopen(argv[1],"wb");if(!f)return 2;
 uint32_t bytes=pcm.size()*2;fwrite("RIFF",1,4,f);word(f,bytes+36,4);fwrite("WAVEfmt ",1,8,f);
 word(f,16,4);word(f,1,2);word(f,2,2);word(f,AUDIO_RATE,4);word(f,AUDIO_RATE*4,4);word(f,4,2);word(f,16,2);
 fwrite("data",1,4,f);word(f,bytes,4);for(int16_t v:pcm)word(f,(uint16_t)v,2);fclose(f);
 fprintf(stderr,"Rendered %zu stereo frames, 28 seconds, DSP only.\n",pcm.size()/2);
}
