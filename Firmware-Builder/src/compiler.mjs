import fs from 'node:fs/promises';import path from 'node:path';import os from 'node:os';import {execFile} from 'node:child_process';import {promisify} from 'node:util';import {fileURLToPath} from 'node:url';
import {writeAtomic}from'./files.mjs';
const run=promisify(execFile),root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
export const LIMITS={flash:30720,sram:2048,stackReserve:384};
export function toolchain(){const base=process.env.ARDUINO_PACKAGES||path.join(os.homedir(),process.platform==='darwin'?'Library/Arduino15/packages/arduino':'.arduino15/packages/arduino');return {gcc:process.env.AVR_TOOLCHAIN||path.join(base,'tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin'),core:process.env.ARDUINO_AVR_CORE||path.join(base,'hardware/avr/1.8.6')};}
export async function compilerStatus(){const t=toolchain();try{await fs.access(path.join(t.core,'cores/arduino/Arduino.h'));const [{stdout},platform]=await Promise.all([run(path.join(t.gcc,'avr-g++'),['--version']),fs.readFile(path.join(t.core,'platform.txt'),'utf8')]);if(!stdout.includes('7.3.0')||!/^version=1\.8\.6$/m.test(platform))return {available:false,message:'Toolchain diversa dalla versione verificata: servono avr-gcc 7.3.0 e core 1.8.6. Generazione sorgenti disponibile.',limits:LIMITS};return {available:true,compiler:'avr-gcc 7.3.0',core:'Arduino AVR 1.8.6',limits:LIMITS};}catch{return {available:false,message:'Compilatore assente. Sorgenti, ZIP e PDF restano disponibili. Installa Arduino AVR Boards 1.8.6; vedi README.',limits:LIMITS};}}
export function memoryReport(sections){const flash=(sections['.text']||0)+(sections['.data']||0),sram=(sections['.data']||0)+(sections['.bss']||0)+(sections['.noinit']||0);const errors=[];if(flash>LIMITS.flash)errors.push(`Flash insufficiente: ${flash} / ${LIMITS.flash} byte.`);if(sram+LIMITS.stackReserve>LIMITS.sram)errors.push(`SRAM insufficiente: ${sram} byte statici + ${LIMITS.stackReserve} di riserva stack > ${LIMITS.sram}. Riduci la selezione.`);return {measurement:'avr-size, ELF realmente collegato',flash,sram,stackReserve:LIMITS.stackReserve,staticFree:LIMITS.sram-sram,limits:LIMITS,errors,ok:errors.length===0,stackNote:'Riserva minima di progetto; profondità reale dello stack e timing vanno verificati per la patch sul dispositivo.'};}
let corePromise;
async function coreBuild(){
 const t=toolchain(),cache=path.join(root,'.cache/core-avr-1.8.6-gcc7-v1');await fs.mkdir(cache,{recursive:true});
 const includes=[path.join(t.core,'cores/arduino'),path.join(t.core,'variants/eightanaloginputs'),path.join(t.core,'libraries/SPI/src'),path.join(t.core,'libraries/EEPROM/src')].map(p=>'-I'+p);
 const common=['-Os','-g','-ffunction-sections','-fdata-sections','-flto','-mmcu=atmega328p','-DF_CPU=16000000UL','-DARDUINO=10819','-DARDUINO_AVR_NANO','-DARDUINO_ARCH_AVR',...includes];
 const archive=path.join(cache,'core.a');
 try{await fs.access(archive);}catch{
   const files=(await fs.readdir(path.join(t.core,'cores/arduino'))).filter(n=>/\.(c|cpp|S)$/.test(n)).map(n=>path.join(t.core,'cores/arduino',n));files.push(path.join(t.core,'libraries/SPI/src/SPI.cpp'));
   const objs=[];for(const file of files){const obj=path.join(cache,path.basename(file)+'.o'),cpp=file.endsWith('.cpp');await run(path.join(t.gcc,cpp?'avr-g++':'avr-gcc'),[...common,...(cpp?['-std=gnu++11','-fno-exceptions','-fno-threadsafe-statics']:file.endsWith('.S')?['-x','assembler-with-cpp']:['-std=gnu11']),'-c',file,'-o',obj]);objs.push(obj);}
   await run(path.join(t.gcc,'avr-gcc-ar'),['rcs',archive,...objs]);
 }
 return {t,common,archive};
}
export async function compileFirmware(files,dir){
 const status=await compilerStatus();if(!status.available)return {status:'unavailable',...status};
 const work=await fs.mkdtemp(path.join(os.tmpdir(),'alteria-compile-'));
 try{
  const {t,common,archive}=await (corePromise||=(coreBuild().catch(e=>{corePromise=null;throw e;})));
  let ino;for(const [name,body]of Object.entries(files)){const dest=path.join(work,name);await writeAtomic(dest,body);await writeAtomic(path.join(dir,name),body);if(name.endsWith('.ino'))ino=dest;}
  const obj=path.join(work,'firmware.o'),elf=path.join(work,'firmware.elf');
  await run(path.join(t.gcc,'avr-g++'),[...common,'-std=gnu++11','-fno-exceptions','-fno-threadsafe-statics','-x','c++','-c',ino,'-o',obj],{maxBuffer:2e6,timeout:120000});
  await run(path.join(t.gcc,'avr-g++'),['-Os','-g','-flto','-fuse-linker-plugin','-Wl,--gc-sections','-mmcu=atmega328p','-o',elf,obj,archive,'-lm'],{maxBuffer:2e6,timeout:120000});
  const {stdout}=await run(path.join(t.gcc,'avr-size'),['-A',elf]);const sections={};for(const l of stdout.split('\n')){const m=l.match(/^(\.[\w.]+)\s+(\d+)/);if(m)sections[m[1]]=Number(m[2]);}
  const report={status:'compiled',...memoryReport(sections),toolchain:{gcc:'7.3.0-atmel3.6.1-arduino7',core:'1.8.6',mcu:'atmega328p',frequency:16000000},sections};
  await writeAtomic(path.join(dir,'firmware.elf'),await fs.readFile(elf));
  if(report.ok){const hex=path.join(work,'firmware.hex');await run(path.join(t.gcc,'avr-objcopy'),['-O','ihex','-R','.eeprom',elf,hex]);report.hex=await fs.readFile(hex,'utf8');await writeAtomic(path.join(dir,'firmware.hex'),report.hex);}else await fs.rm(path.join(dir,'firmware.hex'),{force:true});
  return report;
 }catch(e){await fs.rm(path.join(dir,'firmware.hex'),{force:true});return {status:'failed',ok:false,errors:['Compilazione AVR non riuscita. Nessuna modalità è stata rimossa automaticamente.'],diagnostics:String(e.stderr||e.message).slice(0,16000)};}finally{await fs.rm(work,{recursive:true,force:true});}
}
