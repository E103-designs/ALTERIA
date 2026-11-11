// Execute the compiled ATmega328P firmware, including Timer0/1/2, ADC, SPI,
// UART and erased EEPROM. avr8js simulates peripherals, not the analog PCB.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');
const avr=require(process.env.AVR8JS_PATH||'avr8js');
const build=process.argv[2];if(!build)throw Error('Usage: node avr_firmware_test.cjs BUILD_DIR');
const stem=path.join(build,'ALTERIA_ALLROUNDER.ino');
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
const vector=-1;
function run(ms){const end=cpu.cycles+ms*16000;while(cpu.cycles<end){
 if(cpu.pc===vector){irq++;irqStart=cpu.cycles;inAudioISR=true;}
 const reti=cpu.progMem[cpu.pc]===0x9518;avr.avrInstruction(cpu);
 if(reti&&inAudioISR){irqMax=Math.max(irqMax,cpu.cycles-irqStart);inAudioISR=false;}
 cpu.tick();const sp=cpu.dataView.getUint16(0x5d,true);if(sp>=256&&sp<spMin)spMin=sp;
}}
const boolGate=()=>!!(cpu.data[0x2b]&4);
const u16=name=>cpu.dataView.getUint16(addr(name),true);

if(process.argv[3]==='--session'||process.argv[3]==='--recall'){
 const byte=name=>cpu.data[addr(name)];
 const button=(port,pin)=>{port.setPin(pin,true);run(55);port.setPin(pin,false);run(55);};
 if(process.argv[3]==='--recall'){
   const record=Buffer.from(process.argv[4],'base64');
   backend.memory.set(record,400);
   adc.channelValues[0]=2.5;adc.channelValues[1]=5;adc.channelValues[6]=4;
   run(70);
   assert.equal(byte('mode'),11);
   assert.equal(u16('turingRegister'),record.readUInt16LE(31));
   assert.equal(u16('pot1'),record.readUInt16LE(4));
   assert.equal(u16('pot2'),record.readUInt16LE(6));
   assert.equal(u16('pot3'),record.readUInt16LE(8));
   assert.equal(byte('restoredPots'),7);
   assert(!boolGate(),'Recall replayed a gate');
   adc.channelValues[0]=0;run(30);assert.equal(byte('restoredPots'),6);
   adc.channelValues[0]=1;run(30);assert(u16('pot1')>180);
   button(D,4);assert.equal(byte('mode'),12);assert.equal(byte('restoredPots'),0);
   assert.equal(u16('pot2'),1023);
   return;
 }
 run(70);
 adc.channelValues[0]=0;adc.channelValues[1]=1.4;adc.channelValues[6]=0;
 for(let n=0;n<11;n++)button(D,4);
 assert.equal(byte('mode'),11);
 const base=u16('turingRegister')&31;
 for(let n=1;n<=10;n++){
   adc.channelValues[2]=5;run(5);adc.channelValues[2]=0;run(20);
   if(n===5)assert.equal(u16('turingRegister')&31,base^31);
   if(n===10)assert.equal(u16('turingRegister')&31,base);
 }
 // Move one more step so recall cannot pass by restoring the factory seed.
 adc.channelValues[2]=5;run(5);adc.channelValues[2]=0;run(20);
 const savedRegister=u16('turingRegister');
 assert.notEqual(savedRegister,0xACE1);
 D.setPin(4,true);run(55);C.setPin(3,true);run(1250);
 assert.equal(byte('mode'),11);assert.equal(byte('sessionFeedback'),1);
 assert.equal(u16('turingRegister'),savedRegister);assert(!boolGate());
 assert.equal(backend.memory[437],0xA5);
 for(let i=0;i<1024;i++)if(i<400||i>437)assert.equal(backend.memory[i],255);
 const record=Buffer.from(backend.memory.slice(400,438));
 assert.equal(record.readUInt16LE(31),savedRegister);
 C.setPin(3,false);D.setPin(4,false);run(60);
 assert.equal(byte('mode'),11);
 cp.execFileSync(process.execPath,[__filename,build,'--recall',record.toString('base64')]);
 console.log(JSON.stringify({result:'PASS',scenario:'Turing double-lock, physical save chord, isolated EEPROM, fresh AVR boot with pots displaced, pickup and exit',frames,observedStackBytes:2303-spMin,observedGap:spMin-addr('__bss_end')},null,2));
 process.exit(0);
}

if(process.argv[3]==='--controls'){
 const flag=name=>!!cpu.data[addr(name)];
 const button=(port,pin)=>{port.setPin(pin,true);run(55);port.setPin(pin,false);run(55);};
 function goTo(target){
   for(let n=0;cpu.data[addr('mode')]!==target;n++){
     assert(n<16);button(D,4);
   }
 }
 let latched=[2048,2048],positive=false,negative=false;
 let captureRange=false,burstEdges=[];
 B.addListener((v,o)=>{
   if((v&16)&&!(o&16)){
     latched=[...last];
     if(captureRange){
       assert(latched.every(x=>x>=87&&x<=4008));
       positive|=latched[0]<2048;negative|=latched[0]>2048;
     }
   }
 });
 D.addListener((v,o)=>{
   if(cpu.data[addr('mode')]===4&&(v&4)&&!(o&4))burstEdges.push(cpu.cycles);
 });
 run(70);goTo(6);
 adc.channelValues[0]=2.5;adc.channelValues[1]=5;adc.channelValues[6]=0;run(50);
 assert(!flag('bipolarOutputs'));assert(latched.every(x=>x>=87&&x<=2048));
 C.setPin(3,true);run(40);assert(flag('bipolarOutputs'));
 captureRange=true;run(500);captureRange=false;
 assert(positive&&negative,'Bipolar LFO did not cross zero');
 assert(flag('bipolarOutputs'),'Held MANUAL repeated');
 C.setPin(3,false);run(35);assert(flag('bipolarOutputs'));
 button(C,3);assert(!flag('bipolarOutputs'));
 C.setPin(3,true);run(5);C.setPin(3,false);run(35);
 assert(!flag('bipolarOutputs'),'Button bounce toggled range');
 button(C,3);assert(flag('bipolarOutputs'));
 adc.channelValues[2]=5;run(5);adc.channelValues[2]=0;run(5);
 assert(flag('bipolarOutputs'),'CV sync changed range');
 button(D,4);assert.equal(cpu.data[addr('mode')],7);
 assert(!flag('bipolarOutputs'));
 goTo(6);assert(!flag('bipolarOutputs'),'Tremolo entry must default to unipolar');
 assert(cpu.data[0x2a]&4,'I/O must remain an output');
 // Physical P1 endpoints reach the slow/fast Burst spacing.
 goTo(4);adc.channelValues[0]=0;adc.channelValues[1]=5;run(40);
 burstEdges=[];adc.channelValues[2]=5;run(5);adc.channelValues[2]=0;run(650);
 assert(burstEdges.length>=2);
 const slowMs=(burstEdges[1]-burstEdges[0])/16000;
 assert(slowMs>597&&slowMs<605,'CCW Burst is not slow: '+slowMs);
 adc.channelValues[0]=5;run(40);burstEdges=[];
 adc.channelValues[2]=5;run(5);adc.channelValues[2]=0;run(40);
 assert(burstEdges.length>=2);
 const fastMs=(burstEdges[1]-burstEdges[0])/16000;
 assert(fastMs>=7&&fastMs<11,'CW Burst is not fast: '+fastMs);
 assert(backend.memory.every(x=>x===255),'Controls wrote EEPROM');
 console.log(JSON.stringify({result:'PASS',scenario:'Tremolo default/toggle/hold/bounce/CV sync/exit/re-entry; Burst CCW slow/CW fast',slowMs,fastMs,frames,changes,observedStackBytes:2303-spMin,observedGap:spMin-addr('__bss_end')},null,2));
 process.exit(0);
}

run(70);for(let i=0;i<9;i++){D.setPin(4,true);run(55);D.setPin(4,false);run(55);}assert.equal(cpu.data[addr('mode')],9);assert(cpu.data[0x2a]&4);assert(!(cpu.data[0x6f]&2),'Audio IRQ should not be enabled');
adc.channelValues[0]=0.93;adc.channelValues[1]=1.51;adc.channelValues[6]=2.39;run(30);
const masks=[0x0841,0x2491,0x5555];
const state=()=>[last[0]===87,last[1]===87,boolGate()];
for(let n=0;n<32;n++){
 adc.channelValues[2]=5;run(2);assert.deepEqual(state(),masks.map(m=>!!(m&(1<<(n&15)))));
 run(23);assert.deepEqual(state(),[false,false,false],'Pulses remained high');
 adc.channelValues[2]=0;run(25);
}
adc.channelValues[2]=5;run(3);assert.deepEqual(state(),[true,true,true]);
C.setPin(3,true);run(30);assert.deepEqual(state(),[false,false,false]);
adc.channelValues[2]=0;run(25);adc.channelValues[2]=5;run(3);assert.deepEqual(state(),[true,true,true]);
adc.channelValues[2]=0;run(30);C.setPin(3,false);run(30);
for(let i=0;i<2;i++){D.setPin(4,true);run(55);D.setPin(4,false);run(55);}assert.equal(cpu.data[addr('mode')],11);
adc.channelValues[0]=adc.channelValues[1]=5;adc.channelValues[6]=0;run(40);
let latched=[...last],gateEdges=0;
B.addListener((v,o)=>{if((v&16)&&!(o&16))latched=[...last];});
D.addListener((v,o)=>{if(cpu.data[addr('mode')]===11&&(v&4)&&!(o&4)){gateEdges++;assert.deepEqual(latched,last,'Gate rose before DAC latch');}});
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
for(let i=0;i<4;i++){D.setPin(4,true);run(55);D.setPin(4,false);run(55);}assert.equal(cpu.data[addr('mode')],15);
assert.deepEqual(last,[1068,1068],'S&H starts at the centre of the positive range');
adc.channelValues[0]=5;adc.channelValues[1]=0;adc.channelValues[6]=0;run(40);
adc.channelValues[2]=5;run(4);assert(boolGate());
run(25);assert(!boolGate()&&last[0]===last[1]);
for(let i=0;i<1024;i++)assert.equal(backend.memory[i],255,'Unexpected EEPROM write at '+i);
console.log(JSON.stringify({result:'PASS',scenario:'Euclidean Trigs on DAC A/B and D2, S&H Glide, Turing routing/latch, EEPROM isolation',gateEdges,frames,changes,observedStackBytes:2303-spMin,observedGap:spMin-addr('__bss_end')},null,2));
