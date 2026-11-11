// Execute the compiled ATmega328P firmware, including Timer0/1/2, ADC, SPI,
// UART and erased EEPROM. avr8js simulates peripherals, not the analog PCB.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');
const avr=require(process.env.AVR8JS_PATH||'avr8js');
const build=process.argv[2];if(!build)throw Error('Usage: node avr_firmware_test.cjs BUILD_DIR');
const stem=path.join(build,'ALTERIA_FULL_VOICE.ino');
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
let bytes=[],frames=0,changes=[0,0],last=[0,0],irq=0,irqStart=0,irqMax=0,inAudioISR=false,spMin=2303;
spi.onTransfer=v=>{bytes.push(v);return 0;};
B.addListener((value,old)=>{
 if((value&4)&&!(old&4)&&bytes.length){assert.equal(bytes.length,2,'CS transaction must have 16 bits');const word=(bytes[0]<<8)|bytes[1],ch=word>>15,v=word&4095;if(v!==last[ch])changes[ch]++;last[ch]=v;frames++;bytes=[];}
});
const vector=symbols.get('__vector_11')/2;
function run(ms){const end=cpu.cycles+ms*16000;while(cpu.cycles<end){
 if(cpu.pc===vector){irq++;irqStart=cpu.cycles;inAudioISR=true;}
 const reti=cpu.progMem[cpu.pc]===0x9518;avr.avrInstruction(cpu);
 if(reti&&inAudioISR){irqMax=Math.max(irqMax,cpu.cycles-irqStart);inAudioISR=false;}
 cpu.tick();const sp=cpu.dataView.getUint16(0x5d,true);if(sp>=256&&sp<spMin)spMin=sp;
}}
const u16=name=>cpu.dataView.getUint16(addr(name),true);


const byte=n=>cpu.data[addr(n)];
const set16=(name,v)=>cpu.dataView.setUint16(addr(name),v,true);
function param(p,i,v){cpu.dataView.setUint16(addr('patch')+(p*3+i)*2,v,true);}
function button(port,pin,ms){port.setPin(pin,true);run(ms);port.setPin(pin,false);run(65);}
if(process.argv[3]==='--cal'){
 D.setPin(4,true);C.setPin(3,true);run(200);
 assert(byte('calibrationActive'));assert(!byte('voiceActive'));assert.equal(irq,0);assert(!(cpu.data[0x2a]&4));
 adc.channelValues[0]=adc.channelValues[1]=2.5;D.setPin(4,false);C.setPin(3,false);run(100);
 for(let stage=0;stage<3;stage++){assert.equal(byte('calStage'),stage);button(C,3,75);run(100);}
 assert.equal(byte('calStage'),3);button(C,3,1200);run(1000);
 assert(!byte('calibrationActive'));assert(byte('voiceActive'));assert(!(cpu.data[0x2a]&4));assert.equal(u16('voiceHits'),0);
 assert.equal(backend.memory[256+21],0xA5);for(let i=0;i<1024;i++)if(i<256||i>277)assert.equal(backend.memory[i],255);
 console.log(JSON.stringify({result:'PASS',scenario:'boot ACAL, three points, commit, safe audio restart',irq,frames},null,2));process.exit(0);
}
D.setPin(2,true);run(80);assert(byte('voiceActive'));assert.equal(u16('voiceHits'),0);assert(!(cpu.data[0x2a]&4),'I/O must be input');
D.setPin(2,false);run(5);
adc.channelValues[2]=2;run(40);assert.equal(u16('voiceHits'),0);
button(D,4,65);assert.equal(byte('page'),1);button(C,3,65);assert.equal(byte('page'),0);assert.equal(u16('voiceHits'),0);
C.setPin(3,true);run(600);C.setPin(3,false);run(65);assert.equal(u16('voiceHits'),1);assert.equal(byte('page'),0);
// Four pages with live control scanning, short gate capture and hold semantics.
for(let p=0;p<4;p++){
 assert.equal(byte('page'),p);assert(!(cpu.data[0x2a]&4));
 let hits=u16('voiceHits');D.setPin(2,true);run(5);D.setPin(2,false);run(40);assert.equal(u16('voiceHits'),(hits+1)&65535);
 button(D,4,65);
}
// Do not let physical knobs override DSP test patches via pickup.
adc.channelValues[0]=adc.channelValues[1]=adc.channelValues[6]=2.5;
cpu.data[addr('page')]=3;cpu.data.fill(0,addr('acquired'),addr('acquired')+3);
param(3,0,0);param(3,1,1023);param(3,2,512);
for(let wave=0;wave<4;wave++)for(const pitch of [0,1,2.5,5])for(let c=0;c<16;c++){
 param(0,0,wave*256+128);param(0,1,c&1?1023:0);param(0,2,512);
 param(1,0,c&2?1023:0);param(1,1,c&4?1023:0);param(1,2,1023);
 param(2,0,1023);param(2,1,1023);param(2,2,c&8?1023:0);
 adc.channelValues[2]=pitch;run(15);
 const increment=cpu.dataView.getUint32(addr('voiceIncrement'),true);
 const expected=440*Math.pow(2,(24-69)/12)*Math.pow(2,pitch)*4294967296/6400;
 assert(Math.abs(1200*Math.log2(increment/expected))<12,'Pitch input did not settle');
 let hits=u16('voiceHits');D.setPin(2,true);run(5);D.setPin(2,false);run(20);assert.equal(u16('voiceHits'),(hits+1)&65535,'Lost 5 ms gate');
 assert.equal(byte('targetWave'),wave);
 assert.equal(byte('targetDrive'),c&8?128:32);
 assert(last[0]>=1068&&last[0]<=3028);assert(last[1]>=87&&last[1]<=2048);
}
// Waveform crossfades, including pulse -> saw (three table lookups).
param(2,1,0);for(let w=0;w<12;w++){param(0,0,((w+3)%4)*256+128);run(40);}
const budget=cpu.dataView.getUint16(0x88,true)+1;
assert(irqMax<budget,'Audio overrun: '+irqMax+'/'+budget);
let before=irq;run(250);assert(Math.abs(irq-before-4000000/budget)<=2,'Sample rate drift / lost interrupts');
// Hold MODE saves without changing page or stopping audio; checked EEPROM.
let oldPage=byte('page'),framesBefore=frames,hitsBefore=u16('voiceHits');D.setPin(4,true);
for(let i=0;i<14;i++){D.setPin(2,true);run(5);D.setPin(2,false);run(95);}
D.setPin(4,false);run(300);assert.equal(u16('voiceHits'),(hitsBefore+14)&65535);
assert.equal(byte('page'),oldPage);assert.equal(cpu.dataView.getInt8(addr('saveStep')), -1);assert(!byte('saveError'));assert(frames>framesBefore+1000);
assert.equal(backend.memory[320+33],0xA5);for(let i=0;i<1024;i++)if(i<320||i>353)assert.equal(backend.memory[i],255,'Cross-firmware EEPROM write at '+i);
assert(!(cpu.data[0x2a]&4));assert(changes[0]>1000&&changes[1]>1000);
assert(spMin>addr('__bss_end')+128,'Insufficient observed stack margin');
console.log(JSON.stringify({result:'PASS',irq,irqMaxCycles:irqMax,sampleBudgetCycles:budget,frames,changes,observedStackBytes:2303-spMin,observedStaticToStackGap:spMin-addr('__bss_end')},null,2));
