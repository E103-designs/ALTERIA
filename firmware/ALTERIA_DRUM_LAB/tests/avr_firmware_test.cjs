// Execute the compiled ATmega328P firmware, including Timer0/1/2, ADC, SPI,
// UART and erased EEPROM. avr8js simulates peripherals, not the analog PCB.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');
const avr=require(process.env.AVR8JS_PATH||'avr8js');
const build=process.argv[2];if(!build)throw Error('Usage: node avr_firmware_test.cjs BUILD_DIR');
const stem=path.join(build,'ALTERIA_DRUM_LAB.ino');
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
let bytes=[],frames=0,changes=[0,0],last=[0,0],irq=0,irqStart=0,irqMax=0,inAudioISR=false,scenario='',worst='',lowestSP=0x8ff;
spi.onTransfer=v=>{bytes.push(v);return 0;};
B.addListener((value,old)=>{
 if((value&4)&&!(old&4)&&bytes.length){assert.equal(bytes.length,2,'CS transaction must have 16 bits');const word=(bytes[0]<<8)|bytes[1],ch=word>>15,v=word&4095;if(v!==last[ch])changes[ch]++;last[ch]=v;frames++;bytes=[];}
});
const vector=symbols.get('__vector_11')/2;
function step(){
 if(cpu.pc===vector){irq++;irqStart=cpu.cycles;inAudioISR=true;}
 const reti=cpu.progMem[cpu.pc]===0x9518;avr.avrInstruction(cpu);
 if(reti&&inAudioISR){if(cpu.cycles-irqStart>irqMax){irqMax=cpu.cycles-irqStart;worst=scenario;}inAudioISR=false;}
 const sp=cpu.dataView.getUint16(0x5d,true);if(irq&&sp>=256)lowestSP=Math.min(lowestSP,sp);
 cpu.tick();
}
function run(ms){const end=cpu.cycles+ms*16000;while(cpu.cycles<end)step();}
const u16=name=>cpu.dataView.getUint16(addr(name),true);

if(process.argv[3]==='calibration'){
 D.setPin(4,true);C.setPin(3,true);run(200);
 assert(cpu.data[addr('calibrationActive')]);assert(!cpu.data[addr('audioActive')]);
 assert.equal(cpu.data[0x2a]&4,0);
 D.setPin(4,false);C.setPin(3,false);run(80);
 D.setPin(4,true);run(1400);D.setPin(4,false);run(80);
 assert(!cpu.data[addr('calibrationActive')]&&cpu.data[addr('audioActive')]);
 assert.equal(u16('hitCount'),0);assert.equal(u16('hitCount')+cpu.dataView.getUint16(addr('hitCount')+2,true),0);
 console.log('PASS AVR: hidden calibration boot, input-only IO, audio stopped, cancel/restart without false hits');process.exit(0);
}
run(60);assert.equal(cpu.data[addr('audioActive')],1);assert.equal(cpu.data[0x2a]&4,0);
let old=[...changes];adc.channelValues[2]=4;run(8);adc.channelValues[2]=0;run(30);
assert(changes[0]>old[0]+10&&changes[1]===old[1]);
D.setPin(2,true);run(8);D.setPin(2,false);run(30);assert(changes[1]>old[1]+10);
const hits=ch=>cpu.dataView.getUint16(addr('hitCount')+ch*2,true);
let countA=hits(0),countB=hits(1);
C.setPin(3,true);run(30);C.setPin(3,false);run(60);assert.equal(hits(0),countA+1);assert.equal(hits(1),countB);
D.setPin(4,true);run(60);D.setPin(4,false);run(60);assert.equal(cpu.data[addr('engine')],1);
D.setPin(4,true);run(750);D.setPin(4,false);run(60);assert.equal(cpu.data[addr('selectedVoice')],1);
countA=hits(0);countB=hits(1);
D.setPin(4,true);run(60);C.setPin(3,true);run(40);C.setPin(3,false);D.setPin(4,false);run(60);
assert.equal(cpu.data[addr('page')],1);assert.equal(hits(0),countA);assert.equal(hits(1),countB);
// Real asynchronous EEPROM write while audio interrupts continue.
C.setPin(3,true);run(2100);C.setPin(3,false);run(1000);
assert.equal(hits(1),countB+1);assert.equal(backend.memory[512+203],0xA5);
assert.equal(String.fromCharCode(...backend.memory.slice(512,516)),'DLAB');
assert(backend.memory.slice(0,512).every(x=>x===255),'Old calibration/preset region must stay untouched');
// SRAM injection is DSP stress coverage, not a substitute for the UI tests above.
function parameter(ch,bank,e,profile){
 const base=addr('parameters')+(ch*2+bank)*16;
 const hz=e===0?135:e===1?500:e===2?1400:e===3?255:1750;
 const inc=Math.floor(hz*65536/6400);
 const vals=[inc,Math.floor(Math.min(2850,hz*1.41)*65536/6400),Math.floor(Math.min(2900,hz*2.37)*65536/6400),profile?111:16384,e===0?inc*3:e<=2?inc:0];
 vals.forEach((v,i)=>cpu.dataView.setUint16(base+i*2,v,true));
 const controls=[[0,0,0,0],[255,255,255,255],[127,100,127,127],[128,128,0,0]][profile];
 cpu.data.set([e,...controls,160],base+10);
}
for(let a=0;a<8;a++)for(let b=0;b<8;b++)for(let profile=0;profile<4;profile++){
 scenario=a+','+b+','+profile;
 while(inAudioISR)step();
 for(let ch=0;ch<2;ch++)for(let bank=0;bank<2;bank++)parameter(ch,bank,ch?b:a,profile);
 cpu.data[addr('pendingReset')]=3;cpu.data[addr('pendingHits')]=3;run(25);
}
const budget=cpu.dataView.getUint16(0x88,true)+1;
console.log({irqMax,budget,worst});
assert(irqMax<budget,'Audio ISR exceeds sample budget: '+irqMax+' / '+budget);
const before=irq;run(250);assert(Math.abs(irq-before-16000000*0.25/budget)<=2,'Unexpected sample rate');
// Five-ms input pulses with two heavy engines, not just the default kick/snare.
while(inAudioISR)step();
for(let bank=0;bank<2;bank++){parameter(0,bank,2,1);parameter(1,bank,7,1);}
cpu.data[addr('pendingReset')]=cpu.data[addr('pendingHits')]=3;run(10);
countA=hits(0);countB=hits(1);
adc.channelValues[2]=4;run(5);adc.channelValues[2]=0;run(15);assert.equal(hits(0),countA+1);
D.setPin(2,true);run(5);D.setPin(2,false);run(15);assert.equal(hits(1),countB+1);
assert.equal(cpu.data[0x2a]&4,0,'IO must remain input in every configuration');
assert(lowestSP>addr('__bss_end'),'Stack overlaps statically allocated RAM');
assert(irqMax<budget,'Final ISR budget check');
console.log(JSON.stringify({result:'PASS',irq,irqMaxCycles:irqMax,sampleBudgetCycles:budget,frames,changes,observedStackBytes:0x8ff-lowestSP,stackGapBytes:lowestSP-addr('__bss_end')},null,2));
