// Execute the compiled ATmega328P firmware, including Timer0/1/2, ADC, SPI,
// UART and erased EEPROM. avr8js simulates peripherals, not the analog PCB.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');
const avr=require(process.env.AVR8JS_PATH||'avr8js');
const build=process.argv[2];if(!build)throw Error('Usage: node avr_firmware_test.cjs BUILD_DIR');
const stem=path.join(build,'ALTERIA_ALLROUNDER_2_1_0.ino');
const symbols=new Map(cp.execFileSync(process.env.AVR_NM||'avr-nm',['-C',stem+'.elf'],{encoding:'utf8'}).trim().split('\n').map(l=>{const m=l.match(/^([0-9a-f]+)\s+\w\s+(.+)$/);return m?[m[2],parseInt(m[1],16)]:['',0];}));
function addr(name){assert(symbols.has(name),'Missing ELF symbol '+name);return symbols.get(name)&65535;}
const flash=new Uint8Array(32768);let upper=0;
for(const line of fs.readFileSync(stem+'.hex','utf8').trim().split(/\r?\n/)){
 const n=parseInt(line.slice(1,3),16),offset=parseInt(line.slice(3,7),16),type=parseInt(line.slice(7,9),16);
 if(type===4)upper=parseInt(line.slice(9,13),16)<<16;
 if(type===0)for(let i=0;i<n;i++)flash[upper+offset+i]=parseInt(line.slice(9+2*i,11+2*i),16);
}
const cpu=new avr.CPU(new Uint16Array(flash.buffer),2048);
const B=new avr.AVRIOPort(cpu,avr.portBConfig),C=new avr.AVRIOPort(cpu,avr.portCConfig),D=new avr.AVRIOPort(cpu,avr.portDConfig);
new avr.AVRTimer(cpu,avr.timer0Config);new avr.AVRTimer(cpu,avr.timer1Config);new avr.AVRTimer(cpu,avr.timer2Config);
const adc=new avr.AVRADC(cpu,avr.adcConfig);adc.channelValues.fill(0);adc.channelValues[0]=adc.channelValues[1]=2.5;
const backend=new avr.EEPROMMemoryBackend(1024);backend.memory.fill(255);new avr.AVREEPROM(cpu,backend);
const uart=new avr.AVRUSART(cpu,avr.usart0Config,16000000);const lines=[];uart.onLineTransmit=l=>lines.push(l);
const spi=new avr.AVRSPI(cpu,avr.spiConfig,16000000);
let bytes=[],frames=0,changes=[0,0],last=[0,0],irq=0,irqStart=0,irqMax=0,inAudioISR=false;
spi.onTransfer=v=>{bytes.push(v);return 0;};
B.addListener((value,old)=>{
 if((value&4)&&!(old&4)&&bytes.length){assert.equal(bytes.length,2,'CS transaction must have 16 bits');const word=(bytes[0]<<8)|bytes[1],ch=word>>15,v=word&4095;if(v!==last[ch])changes[ch]++;last[ch]=v;frames++;bytes=[];}
});
const vector=symbols.get('__vector_11')/2;
function run(ms){const end=cpu.cycles+ms*16000;while(cpu.cycles<end){
 if(cpu.pc===vector){irq++;irqStart=cpu.cycles;inAudioISR=true;}
 const reti=cpu.progMem[cpu.pc]===0x9518;avr.avrInstruction(cpu);
 if(reti&&inAudioISR){irqMax=Math.max(irqMax,cpu.cycles-irqStart);inAudioISR=false;}
 cpu.tick();
}}
const u16=name=>cpu.dataView.getUint16(addr(name),true);

run(50);assert.equal(cpu.data[addr('mode')],0);
for(let n=0;n<9;n++){D.setPin(4,true);run(55);D.setPin(4,false);run(55);}
assert.equal(cpu.data[addr('mode')],9);assert(cpu.data[addr('voiceActive')]);
let count=u16('voiceHits'),initial=[...changes];
// Pitch and MANUAL must never trigger the voice by themselves.
adc.channelValues[2]=2;run(30);assert.equal(u16('voiceHits'),count);assert.equal(last[0],2048);assert.equal(last[1],2048);
C.setPin(3,true);run(30);C.setPin(3,false);run(30);assert.equal(cpu.data[addr('voiceSquare')],1);assert.equal(u16('voiceHits'),count);
D.setPin(2,true);run(5);D.setPin(2,false);run(100);assert.equal(u16('voiceHits'),count+1);assert(changes[0]>initial[0]+20&&changes[1]>initial[1]+20);
// Real ADC/control path: both waves, minimum/maximum pitch and control corners.
for(let wave=0;wave<2;wave++){
 if(wave){C.setPin(3,true);run(30);C.setPin(3,false);run(30);}
 for(const pitch of [0,1,2.5,5])for(let corner=0;corner<8;corner++){
  adc.channelValues[0]=corner&1?5:0;adc.channelValues[1]=corner&2?5:0;adc.channelValues[6]=corner&4?5:0;adc.channelValues[2]=pitch;
  run(25);
  assert.equal(u16('voiceDecay'),corner&1?122:12288);
  assert.equal(u16('voiceDamping'),corner&2?55:469);
  assert.equal(cpu.data[addr('voiceModulation')],corner&4?172:0);
  const increment=cpu.dataView.getUint32(addr('voiceIncrement'),true);
  const expected=440*Math.pow(2,(24-69)/12)*Math.pow(2,pitch)*4294967296/6400;
  assert(Math.abs(1200*Math.log2(increment/expected))<12,'Pitch did not settle to the input CV');
  D.setPin(2,true);run(5);D.setPin(2,false);run(30);
 }
}
// Simultaneous gate/wave commands stress the complete crossfade during attack.
for(const pitch of [0,1,2.5,5]){
 adc.channelValues[0]=0;adc.channelValues[1]=5;adc.channelValues[6]=5;adc.channelValues[2]=pitch;run(30);
 cpu.data[addr('voiceCommands')]=3;run(20);
}
const sampleBudget=cpu.dataView.getUint16(0x88,true)+1;
assert(irqMax<sampleBudget,'Audio ISR exceeds budget: '+irqMax);
const before=irq;run(250);assert(Math.abs(irq-before-4000000/sampleBudget)<=2);
D.setPin(4,true);run(55);D.setPin(4,false);run(55);assert.equal(cpu.data[addr('mode')],10);assert(!cpu.data[addr('voiceActive')]);
const stopped=irq;run(50);assert.equal(irq,stopped);assert(cpu.data[0x2a]&4);
console.log(JSON.stringify({result:'PASS',irq,irqMaxCycles:irqMax,sampleBudgetCycles:sampleBudget,frames,changes},null,2));
