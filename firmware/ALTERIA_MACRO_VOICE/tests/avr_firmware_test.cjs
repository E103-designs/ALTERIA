// Execute the compiled ATmega328P firmware, including Timer0/1/2, ADC, SPI,
// UART and erased EEPROM. avr8js simulates peripherals, not the analog PCB.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');
const avr=require(process.env.AVR8JS_PATH||'avr8js');
const build=process.argv[2];if(!build)throw Error('Usage: node avr_firmware_test.cjs BUILD_DIR');
const stem=path.join(build,'ALTERIA_MACRO_VOICE.ino');
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
let bytes=[],frames=0,changes=[0,0],last=[0,0],irq=0,firstIrq=0,irqStart=0,irqMax=0,inAudioISR=false,spMin=2303;
spi.onTransfer=v=>{bytes.push(v);return 0;};
B.addListener((value,old)=>{
 if((value&4)&&!(old&4)&&bytes.length){assert.equal(bytes.length,2,'CS transaction must have 16 bits');const word=(bytes[0]<<8)|bytes[1],ch=word>>15,v=word&4095;if(v!==last[ch])changes[ch]++;last[ch]=v;frames++;bytes=[];}
});
const vector=symbols.get('__vector_11')/2;
function run(ms){const end=cpu.cycles+ms*16000;while(cpu.cycles<end){
 if(cpu.pc===vector){if(!irq)firstIrq=cpu.cycles;irq++;irqStart=cpu.cycles;inAudioISR=true;}
 const reti=cpu.progMem[cpu.pc]===0x9518;avr.avrInstruction(cpu);
 if(reti&&inAudioISR){irqMax=Math.max(irqMax,cpu.cycles-irqStart);inAudioISR=false;}
 cpu.tick();const sp=cpu.dataView.getUint16(0x5d,true);if(sp>=256&&sp<spMin)spMin=sp;
}}
const u16=name=>cpu.dataView.getUint16(addr(name),true);
const byte=n=>cpu.data[addr(n)];
function button(port,pin,ms){port.setPin(pin,true);run(ms);port.setPin(pin,false);run(40);}
function gate(){
 const before=u16('hits');
 D.setPin(2,true);run(5);
 D.setPin(2,false);run(4);
 assert.equal(u16('hits'),(before+1)&65535,'Lost 5 ms trigger');
}
let lastLatch=0,maxLatchGap=0,latches=0;
B.addListener((value,old)=>{
 if(!(value&16)&&(old&16)){
  assert.equal(frames,(latches+1)*2,'Two DAC words required before each LDAC pulse');
  if(latches>=2)maxLatchGap=Math.max(maxLatchGap,cpu.cycles-lastLatch);
  lastLatch=cpu.cycles;latches++;
 }
});
D.setPin(2,true);run(100);
assert.equal(u16('hits'),0,'High at boot must not retrigger');
assert.equal(byte('runningModel'),0);
assert(!(cpu.data[0x2a]&4));
assert(changes[0]>10&&changes[1]>10,'Fold should free-run');
D.setPin(2,false);run(5);
button(C,3,140);assert.equal(u16('hits'),1,'MANUAL held repeats or did not fire');
gate();
adc.channelValues[2]=2.5;run(45);assert.equal(u16('hits'),2,'Pitch is not trigger');
const corner=[0,1.25,2.5,3.75,5],irqByModel=[];
for(let m=0;m<5;m++){
 assert.equal(byte('runningModel'),m);
 assert(!(cpu.data[0x2a]&4),'I/O must stay INPUT');
 if(m===4)assert.deepEqual(last,[2048,2048],'String must wait for a pluck');
 irqMax=0;
 for(let p=0;p<corner.length;p++)for(let k=0;k<8;k++){
  adc.channelValues[2]=corner[p];
  adc.channelValues[0]=k*5/7;
  adc.channelValues[1]=corner[(p+k)%5];
  adc.channelValues[6]=corner[(2*p+k)%5];
  run(30);gate();run(16);
  assert(last.every(v=>v>=1068&&v<=3028),'DAC out of calibrated audio range');
 }
 irqByModel.push(irqMax);
 if(m===4){
  const length=byte('liveLength');
  adc.channelValues[2]=0;run(30);assert.equal(byte('liveLength'),length,'String pitch must be held');
  gate();assert.notEqual(byte('liveLength'),length);
 }
 // An already high gate must not produce another hit when changing model.
 D.setPin(2,true);run(5);
 const count=u16('hits');
 button(D,4,60);
 assert.equal(u16('hits'),count);
 D.setPin(2,false);run(5);
}
assert.equal(byte('runningModel'),0,'MODE must wrap');
const budget=cpu.dataView.getUint16(0x88,true)+1;
assert.equal(budget,2000);
assert(irqByModel.every(n=>n<budget),'Audio ISR overrun: '+irqByModel);
const before=irq;run(500);assert(Math.abs(irq-before-4000)<=1,'Lost sample interrupts');
assert.equal(Math.round((irqStart-firstIrq)/budget)+1,irq,'Missed compare interrupts in the full run');
assert(maxLatchGap<budget*2,'Excessive LDAC gap '+maxLatchGap);
assert(frames>=2*latches&&frames<=2*latches+2,'Unexpected DAC words after final latch');
assert(backend.memory.every(x=>x===255),'Firmware wrote to EEPROM');
assert(spMin>addr('__bss_end')+128);
console.log(JSON.stringify({result:'PASS',irq,irqByModel,sampleBudgetCycles:budget,latches,maxLatchGap,changes,observedStackBytes:2303-spMin,observedStaticToStackGap:spMin-addr('__bss_end')},null,2));
