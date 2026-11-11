const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');const avr=require(process.env.AVR8JS_PATH||'avr8js');
module.exports=function simulation(dir,seed){
 const gcc=process.env.AVR_TOOLCHAIN||path.join(process.env.HOME,'Library/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin');
 const symbols=new Map(cp.execFileSync(path.join(gcc,'avr-nm'),['-C',path.join(dir,'firmware.elf')],{encoding:'utf8'}).trim().split('\n').map(l=>{const m=l.match(/^([0-9a-f]+)\s+\w\s+(.+)$/);return m?[m[2],parseInt(m[1],16)]:['',0];}));
 function address(n){assert(symbols.has(n),'Missing ELF symbol '+n);return symbols.get(n)&65535;}
 const flash=new Uint8Array(65536);let upper=0;for(const l of fs.readFileSync(path.join(dir,'firmware.hex'),'utf8').trim().split(/\r?\n/)){const n=parseInt(l.slice(1,3),16),offset=parseInt(l.slice(3,7),16),type=parseInt(l.slice(7,9),16);if(type===4)upper=parseInt(l.slice(9,13),16)<<16;if(type===0)for(let i=0;i<n;i++)flash[upper+offset+i]=parseInt(l.slice(9+i*2,11+i*2),16);}
 const cpu=new avr.CPU(new Uint16Array(flash.buffer),2048),B=new avr.AVRIOPort(cpu,avr.portBConfig),C=new avr.AVRIOPort(cpu,avr.portCConfig),D=new avr.AVRIOPort(cpu,avr.portDConfig);new avr.AVRTimer(cpu,avr.timer0Config);new avr.AVRTimer(cpu,avr.timer1Config);new avr.AVRTimer(cpu,avr.timer2Config);
 const adc=new avr.AVRADC(cpu,avr.adcConfig);adc.channelValues.fill(0);adc.channelValues[0]=adc.channelValues[1]=2.5;
 const mem=new avr.EEPROMMemoryBackend(1024);mem.memory.fill(255);if(seed)seed(mem.memory);new avr.AVREEPROM(cpu,mem);new avr.AVRUSART(cpu,avr.usart0Config,16000000);const spi=new avr.AVRSPI(cpu,avr.spiConfig,16000000);
 let bytes=[],frames=0,values=[0,0],changes=[0,0],spMin=2303,irq=0,irqMax=0,irqStart=0,inIrq=false;const vector=symbols.has('__vector_11')?symbols.get('__vector_11')/2:-1;
 spi.onTransfer=v=>{bytes.push(v);return 0;};B.addListener((v,o)=>{if((v&4)&&!(o&4)&&bytes.length){assert.equal(bytes.length,2);let w=bytes[0]*256+bytes[1],ch=w>>15,val=w&4095;if(val!==values[ch])changes[ch]++;values[ch]=val;frames++;bytes=[];}});
 function run(ms){let end=cpu.cycles+ms*16000;while(cpu.cycles<end){if(cpu.pc===vector){irq++;irqStart=cpu.cycles;inIrq=true;}let reti=cpu.progMem[cpu.pc]===0x9518;avr.avrInstruction(cpu);if(reti&&inIrq){irqMax=Math.max(irqMax,cpu.cycles-irqStart);inIrq=false;}cpu.tick();let sp=cpu.dataView.getUint16(0x5d,true);if(sp>=256)spMin=Math.min(spMin,sp);}}
 const read8=n=>cpu.data[address(n)],read16=n=>cpu.dataView.getUint16(address(n),true),write32=(n,v)=>cpu.dataView.setUint32(address(n),v,true);
 function mode(){D.setPin(4,true);run(55);D.setPin(4,false);run(55);}
 function manual(){C.setPin(3,true);run(35);C.setPin(3,false);run(35);}
 return {cpu,B,C,D,adc,memory:mem.memory,address,read8,read16,write32,run,mode,manual,values,changes,gate:()=>!!(cpu.data[0x2b]&4),input:()=>!(cpu.data[0x2a]&4),report:()=>({irq,irqMax,frames,changes,observedStack:2303-spMin,observedGap:spMin-address('__bss_end')})};
};
