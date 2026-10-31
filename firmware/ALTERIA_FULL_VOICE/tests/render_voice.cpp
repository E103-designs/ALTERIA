// Software audition only, not a hardware recording or analog PCB simulation.
#include <cstdio>
#include <vector>
#include "../ALTERIA_FULL_VOICE.ino"
static void word(FILE *f,uint32_t x,uint8_t n){while(n--){fputc(x&255,f);x>>=8;}}
int main(int argc,char **argv){
  if(argc!=2)return 1;setup();std::vector<int16_t> pcm;
  const uint8_t notes[8]={0,0,7,12,3,0,10,7};
  for(unsigned step=0;step<32;step++){
    patch[0][0]=(step/8)*256+128;patch[0][1]=256+(step%8)*73;
    patch[1][0]=350+(step%8)*35;patch[1][1]=200+(step/8)*160;
    patch[2][2]=step<16?0:150;patch[3][2]=step<16?512:256;
    pitchCv=819+(uint32_t)notes[step%8]*4095/60;
    triggerEnvelope(step*200);
    for(unsigned n=0;n<1280;n++){
      unsigned sample=step*1280+n;
      if(!(n%6)){advanceEnvelope(sample*1000/6400);publishControls();}
      voiceAudioTick();pcm.push_back(((int)voiceZeroA-voiceWritten[0])*16);
    }
  }
  FILE *f=fopen(argv[1],"wb");if(!f)return 2;uint32_t bytes=pcm.size()*2;
  fwrite("RIFF",1,4,f);word(f,bytes+36,4);fwrite("WAVEfmt ",1,8,f);word(f,16,4);word(f,1,2);
  word(f,1,2);word(f,VOICE_RATE,4);word(f,VOICE_RATE*2,4);word(f,2,2);word(f,16,2);
  fwrite("data",1,4,f);word(f,bytes,4);for(int16_t s:pcm)word(f,(uint16_t)s,2);fclose(f);
  puts("Rendered 6.4 seconds: sine, triangle, saw, pulse; software DSP only.");
}
