#include <cstdio>
#include "../ALTERIA_MACRO_VOICE.ino"

static void word(FILE *f,uint32_t x,uint8_t n){
  while(n--){fputc(x&255,f);x>>=8;}
}

int main(int argc,char **argv){
  if(argc!=2)return 1;
  FILE *f=fopen(argv[1],"wb");if(!f)return 2;
  const uint32_t section=4UL*AUDIO_RATE,bytes=section*5*4;
  fwrite("RIFF",1,4,f);word(f,bytes+36,4);fwrite("WAVEfmt ",1,8,f);word(f,16,4);
  word(f,1,2);word(f,2,2);word(f,AUDIO_RATE,4);word(f,AUDIO_RATE*4UL,4);
  word(f,4,2);word(f,16,2);fwrite("data",1,4,f);word(f,bytes,4);
  setup();
  const uint8_t notes[8]={0,0,3,7,0,10,12,7};
  for(uint8_t m=0;m<MODEL_COUNT;m++){
    pots[0]=m==WAVETABLE?450:300;pots[1]=200;pots[2]=230;
    selectModel(m);
    for(uint32_t i=0;i<section;i++){
      if(!(i%32)){
        uint8_t beat=i/(AUDIO_RATE/2);uint16_t sweep=i*1023/(section-1);
        pitchCv=(uint32_t)(12+notes[beat])*4095/60;
        switch(m){
          case FOLD: pots[0]=250+(sweep>>1);pots[1]=sweep;pots[2]=sweep>>1;break;
          case FM: pots[0]=(beat/2)*128+192;pots[1]=sweep;pots[2]=sweep>>1;break;
          case CHORD: pots[0]=(beat/2)*256+64;pots[1]=(beat%2)*300;pots[2]=sweep;break;
          case WAVETABLE: pots[1]=sweep;pots[2]=1023-sweep;break;
          case KARPLUS: pots[0]=100+(sweep>>1);pots[1]=875;pots[2]=sweep;break;
        }
        publish(i%(AUDIO_RATE/2)==0);
      }
      audioTick();
      for(uint8_t c=0;c<2;c++)word(f,(uint16_t)(((int)zeroCode[c]-written[c])*27),2);
    }
  }
  return fclose(f)!=0;
}
