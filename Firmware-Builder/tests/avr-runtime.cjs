// Execute the compiled ATmega328P firmware, including Timer0/1/2, ADC, SPI,
// UART and erased EEPROM. avr8js simulates peripherals, not the analog PCB.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');
const avr=require(process.env.AVR8JS_PATH||'avr8js');
const build=process.argv[2];if(!build)throw Error('Usage: node avr_firmware_test.cjs BUILD_DIR');
const stem=path.join(build,'firmware');
const symbols=new Map(cp.execFileSync(process.env.AVR_NM||'avr-nm',['-C',stem+'.elf'],{encoding:'utf8'}).trim().split('\n').map(l=>{const m=l.match(/^([0-9a-f]+)\s+\w\s+(.+)$/);return m?[m[2],parseInt(m[1],16)]:['',0];}));
function addr(name){const key=symbols.has(name)?name:[...symbols.keys()].find(k=>k.endsWith('::'+name));assert(key,'Missing ELF symbol '+name);return symbols.get(key)&65535;}
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
const vector=-1;
function run(ms){const end=cpu.cycles+ms*16000;while(cpu.cycles<end){
 if(cpu.pc===vector){irq++;irqStart=cpu.cycles;inAudioISR=true;}
 const reti=cpu.progMem[cpu.pc]===0x9518;avr.avrInstruction(cpu);
 if(reti&&inAudioISR){irqMax=Math.max(irqMax,cpu.cycles-irqStart);inAudioISR=false;}
 cpu.tick();const sp=cpu.dataView.getUint16(0x5d,true);if(sp>=256&&sp<spMin)spMin=sp;
}}
const boolGate=()=>!!(cpu.data[0x2b]&4);
const u16=name=>cpu.dataView.getUint16(addr(name),true);


run(70);for(let i=0;i<9;i++){D.setPin(4,true);run(55);D.setPin(4,false);run(55);}assert.equal(cpu.data[addr('slot')],9);assert(cpu.data[0x2a]&4);assert(!(cpu.data[0x6f]&2),'Audio IRQ should not be enabled');
adc.channelValues[0]=0;adc.channelValues[1]=2.5;adc.channelValues[6]=0;run(30);
let notes=[];for(let n=0;n<48;n++){adc.channelValues[2]=5;run(5);assert.equal(cpu.data[addr('orbitStep')],(n+1)&15);notes.push(last[0]);assert.equal(boolGate(),!!(u16('orbitRhythm')&(1<<(n&15))));adc.channelValues[2]=0;run(120);assert(!(cpu.data[0x2b]&4));}
assert.deepEqual(notes.slice(16,32),notes.slice(32,48),'Locked sequence changed');
C.setPin(3,true);run(80);C.setPin(3,false);run(80);assert(cpu.data[addr('orbitNewTheme')]);
C.setPin(3,true);run(750);C.setPin(3,false);run(80);assert.equal(cpu.data[addr('orbitNextScale')],1);
for(let i=0;i<2;i++){D.setPin(4,true);run(55);D.setPin(4,false);run(55);}assert.equal(cpu.data[addr('slot')],11);
adc.channelValues[0]=adc.channelValues[1]=5;adc.channelValues[6]=0;run(40);
let latched=[...last],gateEdges=0;
B.addListener((v,o)=>{if((v&16)&&!(o&16))latched=[...last];});
D.addListener((v,o)=>{if(cpu.data[addr('slot')]===11&&(v&4)&&!(o&4)){gateEdges++;assert.deepEqual(latched,last,'Gate rose before DAC latch');}});
let reg=0xACE1,seenHigh=false,seenLow=false;
const rotate=()=>{reg=(reg>>>1)|((reg&1)?32768:0);};
const checkRegister=()=>{let v=Math.floor(reg*4095/65535);assert.equal(last[0],2048+Math.trunc((87-2048)*v/4095));assert.equal(last[1],reg&1?87:2048);};
for(let n=0;n<32;n++){
 adc.channelValues[2]=5;run(5);rotate();checkRegister();assert(boolGate());
 if(reg&1)seenHigh=true;else seenLow=true;
 let held=[...last];run(15);assert.deepEqual(last,held);
 adc.channelValues[2]=0;run(5);assert(!boolGate());assert.deepEqual(last,held);
}
assert(seenHigh&&seenLow&&gateEdges===32);
C.setPin(3,true);run(30);rotate();checkRegister();assert(boolGate());let held=[...last];run(50);assert.deepEqual(last,held);
adc.channelValues[2]=5;run(5);rotate();checkRegister();assert(boolGate());C.setPin(3,false);run(35);assert(boolGate());adc.channelValues[2]=0;run(5);assert(!boolGate());
for(let i=0;i<4;i++){D.setPin(4,true);run(55);D.setPin(4,false);run(55);}assert.equal(cpu.data[addr('slot')],15);
for(let i=0;i<8;i++){C.setPin(3,true);run(40);C.setPin(3,false);run(40);assert.equal(cpu.data[addr('selectedPreset')],(i+1)&7);}
D.setPin(4,true);run(750);D.setPin(4,false);run(70);assert(cpu.data[addr('presetEditing')]);
// Acquire first slot knob and make a deliberate edit.
adc.channelValues[0]=0;run(40);adc.channelValues[0]=3;run(40);
D.setPin(4,true);run(750);D.setPin(4,false);run(400);assert(!cpu.data[addr('presetEditing')]);
assert.equal(cpu.dataView.getInt8(addr('saveStep')),-1);assert.equal(backend.memory[49],0xA5);
for(let i=112;i<1024;i++)assert.equal(backend.memory[i],255,'Unrelated EEPROM modified at '+i);
console.log(JSON.stringify({result:'PASS',scenario:'ORBIT, eight presets, Turing CV/held bit/clock OR MANUAL, DAC latch before gate, EEPROM isolation, no audio IRQ',gateEdges,frames,changes,observedStackBytes:2303-spMin,observedGap:spMin-addr('__bss_end')},null,2));
