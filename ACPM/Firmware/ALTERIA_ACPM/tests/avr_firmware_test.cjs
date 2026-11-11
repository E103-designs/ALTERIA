// Runs the actual Nano HEX/ELF with ADC, SPI, GPIO, timers and EEPROM.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');
const avr=require(process.env.AVR8JS_PATH||'avr8js');
const build=process.argv[2];
if(!build)throw Error('Usage: node avr_firmware_test.cjs BUILD_DIR');
const stem=path.join(build,'ALTERIA_ACPM.ino');
const symbols=new Map(cp.execFileSync(process.env.AVR_NM||'avr-nm',['-C',stem+'.elf'],{encoding:'utf8'})
  .trim().split('\n').map(l=>{const m=l.match(/^([0-9a-f]+)\s+\w\s+(.+)$/);return m?[m[2],parseInt(m[1],16)]:['',0];}));
const addr=n=>{assert(symbols.has(n),'Missing symbol '+n);return symbols.get(n)&65535;};
const flash=new Uint8Array(32768);
for(const line of fs.readFileSync(stem+'.hex','utf8').trim().split(/\r?\n/)){
 if(line.slice(7,9)!=='00')continue;
 const n=parseInt(line.slice(1,3),16),at=parseInt(line.slice(3,7),16);
 for(let i=0;i<n;i++)flash[at+i]=parseInt(line.slice(9+2*i,11+2*i),16);
}
const cpu=new avr.CPU(new Uint16Array(flash.buffer),2048);
const B=new avr.AVRIOPort(cpu,avr.portBConfig),C=new avr.AVRIOPort(cpu,avr.portCConfig),D=new avr.AVRIOPort(cpu,avr.portDConfig);
new avr.AVRTimer(cpu,avr.timer0Config);new avr.AVRTimer(cpu,avr.timer1Config);new avr.AVRTimer(cpu,avr.timer2Config);
const adc=new avr.AVRADC(cpu,avr.adcConfig);adc.channelValues.fill(0);
const backend=new avr.EEPROMMemoryBackend(1024);backend.memory.fill(255);new avr.AVREEPROM(cpu,backend);
const spi=new avr.AVRSPI(cpu,avr.spiConfig,16000000);
let bytes=[],commands=0,last=[0,0],latched=[0,0],rises=0,minSp=2303;
spi.onTransfer=v=>{bytes.push(v);return 0;};
B.addListener((v,o)=>{
 if((v&4)&&!(o&4)&&bytes.length){
   assert.equal(bytes.length,2);
   const w=(bytes[0]<<8)|bytes[1];assert.equal(w&0x7000,0x1000);
   last[w>>15]=w&4095;bytes=[];commands++;
 }
 if((v&16)&&!(o&16))latched=[...last];
});
D.addListener((v,o)=>{
 if((v&4)&&!(o&4)){rises++;assert.deepEqual(latched,last,'Gate before DAC latch');}
});
function run(ms){
 const end=cpu.cycles+16000*ms;
 while(cpu.cycles<end){
   avr.avrInstruction(cpu);cpu.tick();
   const sp=cpu.dataView.getUint16(0x5d,true);if(sp>=256&&sp<minSp)minSp=sp;
 }
 assert(cpu.data[0x2a]&4,'I/O must always be output');
}
const byte=n=>cpu.data[addr(n)];
const led=()=>{
 const d=cpu.data[0x2b],b=cpu.data[0x25];
 return ((d>>6)&1)*8+((d>>7)&1)*4+(b&1)*2+((b>>1)&1);
};
const press=(port,pin)=>{port.setPin(pin,true);run(45);port.setPin(pin,false);run(45);};

if(process.argv[3]==='--recall'){
 backend.memory.set(Buffer.from(process.argv[4],'base64'),190);
 run(100);
 assert.equal(byte('selected'),7);assert.equal(byte('page'),0);assert.equal(led(),7);
 assert.equal(byte('length'),8);assert.equal(byte('rangeIndex'),1);assert(byte('addressMode'));
 assert(cpu.data[0x2b]&4);assert.equal(rises,1);
 adc.channelValues[2]=2.7;run(10);assert.equal(byte('selected'),4);
 assert(!(cpu.data[0x2b]&4));process.exit(0);
}

adc.channelValues[0]=5;adc.channelValues[2]=5;run(100);
assert.equal(led(),0);assert.equal(byte('selected'),0);assert.deepEqual(latched,[2048,2048]);
for(let n=1;n<=16;n++){
 adc.channelValues[2]=0;run(6);adc.channelValues[2]=5;run(6);
 assert.equal(byte('selected'),n&15);assert.equal(led(),n&15);
}
adc.channelValues[2]=0;run(10);
D.setPin(4,true);run(40);C.setPin(3,true);run(455);
assert.equal(byte('page'),2);assert.equal(led(),8);
C.setPin(3,false);D.setPin(4,false);run(45);
adc.channelValues[0]=2.3;run(35);assert.equal(byte('length'),8);assert.equal(led(),7);
run(1100);assert.equal(led(),8);
adc.channelValues[1]=2.3;run(35);assert.equal(led(),4);
adc.channelValues[6]=4;run(35);assert.equal(led(),5);
press(D,4);assert.equal(byte('page'),0);
adc.channelValues[2]=2;run(10);assert.equal(byte('selected'),3);
press(C,3);assert.equal(byte('selected'),4);run(100);assert.equal(byte('selected'),4);
adc.channelValues[2]=4.9;run(15);assert.equal(byte('selected'),7);
press(D,4);assert.equal(byte('page'),1);
adc.channelValues[0]=3;run(30);assert(latched[0]<2048);
adc.channelValues[6]=0;run(30);adc.channelValues[6]=5;run(30);
assert(cpu.data[0x2b]&4);assert.equal(rises,1);
D.setPin(4,true);run(1350);D.setPin(4,false);run(45);
assert.equal(byte('feedback'),1);
assert.equal(backend.memory[248],0xa5);
for(let i=0;i<1024;i++)if(i<190||i>248)assert.equal(backend.memory[i],255);
cp.execFileSync(process.execPath,[__filename,build,'--recall',Buffer.from(backend.memory.slice(190,249)).toString('base64')]);
console.log(JSON.stringify({result:'PASS',scenario:'all LED indices, Clock/Address, SETUP preview, pickup, latched CV before gate, save and fresh boot',commands,rises,observedStackBytes:2303-minSp,observedGap:minSp-addr('__bss_end')},null,2));
