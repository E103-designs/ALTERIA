export const VERSION='3.1.0', TARGET='nano328p-rev5-2';
export function validateConfig(input,catalog){
  if(!input||input.schemaVersion!==1)throw Error('Versione configurazione non supportata: serve schemaVersion 1.');
  if(input.catalogVersion!==catalog.version&&input.catalogVersion!=='3.0.0')throw Error('Versione catalogo diversa. Riesporta con questo catalogo.');
  if(input.target!==TARGET)throw Error('Target incompatibile: seleziona Nano ATmega328P / REV5-2.');
  if(typeof input.name!=='string'||!input.name.trim()||input.name.length>64)throw Error('Il nome deve contenere da 1 a 64 caratteri.');
  if(typeof input.version!=='string'||!/^\d+\.\d+\.\d+(?:-[\w.-]+)?$/.test(input.version))throw Error('Versione firmware non valida (es. 1.0.0).');
  if(!Array.isArray(input.modes)||input.modes.length<1||input.modes.length>16)throw Error('Seleziona da 1 a 16 modalità complessive.');
  const ids=input.modes.map(id=>input.catalogVersion==='3.0.0'&&id==='rh_euclid'?'rh_euclidean_trigs':id);
  if(new Set(ids).size!==ids.length)throw Error('La stessa modalità compare più volte.');
  const modes=ids.map(id=>{const m=catalog.modes.find(m=>m.id===id);if(!m)throw Error('Modalità sconosciuta: '+String(id));return m;});
  const claims=new Map();for(const m of modes)for(const claim of m.exclusiveResources||[]){if(claims.has(claim))throw Error(`Dipendenze incompatibili: ${m.name} e ${claims.get(claim)} condividono ${claim}.`);claims.set(claim,m.name);}
  return {schemaVersion:1,catalogVersion:catalog.version,target:TARGET,name:input.name.trim(),version:input.version,modes:ids};
}
export function defaultConfig(catalog){return {schemaVersion:1,catalogVersion:catalog.version,target:TARGET,name:'ALTERIA · Il mio strumento',version:'1.0.0',modes:['ar_ad','ar_ar','ar_dual_decay','ar_ducking','ar_burst','ar_acid','ar_tremolo','ar_grids','ar_clock3','rh_euclidean_trigs','ar_xor','ar_turing','ar_humanizer','ar_bernoulli','ar_trance','cv_sh_glide']};}
export const stemFor=c=>('ALTERIA_'+c.name.normalize('NFD').replace(/[\u0300-\u036f]/g,'').replace(/[^A-Za-z0-9_]/g,'_').replace(/_+/g,'_')).slice(0,60);
export const ledBits=slot=>(slot&15).toString(2).padStart(4,'0').split('').reverse().join('');
export function selected(c,catalog){return c.modes.map((id,i)=>({...catalog.modes.find(m=>m.id===id),slot:i+1,led:ledBits(i+1)}));}
export function generateFirmware(input,catalog,sources){
 const c=validateConfig(input,catalog), modes=selected(c,catalog), stem=stemFor(c);
 const isPreset=modes.some(m=>m.id==='ar_presets8');
 const audio=modes.filter(m=>m.audio),composites=modes.filter(m=>m.composite);
 let code=`// ALTERIA Builder ${VERSION}. Firmware ${c.version}. Slots are defined by configuration.json.\n`+sources.runtime+'\n'+sources.scheduler+'\n'+sources.allrounder_math+'\n'+sources.legacy_helpers+'\n';
 code+='volatile uint8_t audioOwner=0;bool chordTracked=false;uint32_t chordSince=0;\n';
 for(const m of modes)code+=`\n// ${m.slot}: ${m.id}; algorithm SHA256 ${m.algorithmSha256}\nnamespace ${m.id} {\n${sources[m.codeKey]}\n}\n`;
 if(audio.length)code+=`\n#if defined(__AVR__)\nISR(TIMER1_COMPA_vect){switch(audioOwner){${audio.map(m=>`case ${m.audio}:${m.id}::${m.audioTick}();break;`).join('')}default:break;}}\n#endif\n`;
 code+='\n'+sources.calibration;
 const preset=isPreset?'ar_presets8::':'';
 code+=`
static bool selectedFast(){switch(slot){${modes.filter(m=>m.fast).map(m=>`case ${m.slot-1}:return true;`).join('')}default:return false;}}
static bool selectedInput(){switch(slot){${modes.filter(m=>m.ioDirection==='input').map(m=>`case ${m.slot-1}:return true;`).join('')}default:return false;}}
static bool selectedComposite(){switch(slot){${composites.map(m=>`case ${m.slot-1}:return true;`).join('')}default:return false;}}
static void displayModes(uint32_t nowMs){
 uint8_t bits=(slot+1)&15;
 ${isPreset?`if(slot==${modes.findIndex(m=>m.id==='ar_presets8')}){bits=8|${preset}selectedPreset;if(${preset}presetEditing&&((nowMs/140)&1))bits&=7;if(${preset}presetSaveError)bits=((nowMs/100)&1)?15:0;}`:''}
 ${modes.some(m=>m.id==='ar_orbit')?'if(slot=='+modes.findIndex(m=>m.id==='ar_orbit')+'&&ar_orbit::orbitNotice&&!elapsedMs(nowMs,ar_orbit::orbitNoticeAt,1000))bits=1U<<ar_orbit::orbitNextScale;':''}
 setLedPattern(bits);
}
static void enterSlot(uint8_t next){
 ${audio.map(m=>`if(audioOwner==${m.audio})${m.id}::${m.stop}();`).join('')}audioOwner=0;
 ${isPreset?`if(${preset}presetEditing){${preset}presetEditing=false;${preset}startPresetSave();}`:''}
 schClearAll();pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);
 slot=next%${modes.length};slotEnteredAt=millis();modeJustChanged=true;
 pendingTrigger=pendingTriggerA=pendingTriggerB=pendingCv=pendingManual=resetTriggered=false;
 bipolarOutputs=false;
 switch(slot){${modes.filter(m=>m.range.startsWith('-5')).map(m=>`case ${m.slot-1}:bipolarOutputs=true;break;`).join('')}default:break;}
 dacLastA=dacLastB=0xFFFF;out1(bipolarOutputs?2048:0);out2(bipolarOutputs?2048:0);latchDac();
 pinMode(PIN_OUT3,selectedInput()?INPUT:OUTPUT);
 ioGate=selectedInput()&&digitalRead(PIN_OUT3)==HIGH;ioRising=false;
 cvLevelQ12=normalizeCv(analogRead(PIN_CV_INPUT));cvGate=cvLevelQ12>=CV_GATE_ON_Q12;cvRising=cvFalling=false;
 switch(slot){${modes.map(m=>`case ${m.slot-1}:{using namespace ${m.id};${m.entry}break;}`).join('\n')}default:break;}
 lastUpdateUs=micros();displayModes(millis());
}
static bool serviceModeButton(uint32_t nowMs){
 if(modeBtn.rising){modePressArmed=true;modeLongHandled=false;modePressedMs=nowMs;}
 if(modePressArmed&&modeBtn.stable&&!modeLongHandled&&elapsedMs(nowMs,modePressedMs,650)){
  modeLongHandled=true;
  ${isPreset?`if(slot==${modes.findIndex(m=>m.id==='ar_presets8')}){${preset}presetEditing=!${preset}presetEditing;if(${preset}presetEditing){${preset}presetSaveError=false;${preset}resetPickup();}else ${preset}startPresetSave();}`:''}
 }
 if(modeBtn.falling&&modePressArmed){modePressArmed=false;if(!modeLongHandled&&${modes.length}>1){enterSlot((slot+1)%${modes.length});return true;}}
 return false;
}
void setup(){
 pinMode(PIN_MODE_BUTTON,INPUT);pinMode(PIN_MANUAL_BUTTON,INPUT);pinMode(PIN_CV_INPUT,INPUT);pinMode(PIN_OUT3,INPUT);digitalWrite(PIN_OUT3,LOW);pinMode(A5,INPUT);
 pinMode(PIN_INPUT_LED,OUTPUT);pinMode(PIN_IO_LED,OUTPUT);for(uint8_t i=0;i<4;i++)pinMode(PIN_LED_MODE_1+i,OUTPUT);
 pinMode(PIN_DAC_CS,OUTPUT);digitalWrite(PIN_DAC_CS,HIGH);SPI.begin();digitalWrite(PIN_DAC_LDAC,HIGH);pinMode(PIN_DAC_LDAC,OUTPUT);
 manualBtn.raw=manualBtn.stable=digitalRead(PIN_MANUAL_BUTTON)==HIGH;modeBtn.raw=modeBtn.stable=digitalRead(PIN_MODE_BUTTON)==HIGH;
 manualBtn.rawSinceMs=modeBtn.rawSinceMs=millis();
 pot1=analogRead(PIN_POT_1);pot2=analogRead(PIN_POT_2);pot3=analogRead(PIN_POT_3);shapeSector=(uint32_t)pot3*12/1024;
 randomSeed(((uint32_t)pot1<<20)^micros()^pot2);loadCalibration();${isPreset?`${preset}loadPresets();`:''}
 if(modeBtn.stable&&manualBtn.stable){calBootPending=true;calBootStart=millis();out1(0);out2(0);latchDac();}else enterSlot(0);
}
void loop(){
 uint32_t nowMs=millis();scanButton(manualBtn,nowMs);scanButton(modeBtn,nowMs);readPots(nowMs);
 if(calBootPending){serviceCalBoot(nowMs);return;}if(calibrationActive){serviceCalibration(nowMs);return;}
 ${isPreset?`${preset}servicePresetSave();`:''}
 ${audio.filter(m=>m.composite).map(m=>m.audio===2?`${m.id}::servicePatchSave(nowMs);`:`${m.id}::serviceKitSave(nowMs);`).join('')}
 if(selectedComposite()){
  if(${modes.length}>1&&modeBtn.stable&&manualBtn.stable){if(!chordTracked){chordTracked=true;chordSince=nowMs;}else if(elapsedMs(nowMs,chordSince,2000)){chordTracked=false;enterSlot((slot+1)%${modes.length});return;}}else chordTracked=false;
  switch(slot){${composites.map(m=>`case ${m.slot-1}:${m.id}::moduleTick();break;`).join('')}default:break;}
  if(!elapsedMs(nowMs,slotEnteredAt,800))displayModes(nowMs);return;
 }
 if(serviceModeButton(nowMs))return;
 uint32_t nowUs=micros();readCvInput(nowUs);
 ioRising=false;if(selectedInput()){bool level=digitalRead(PIN_OUT3)==HIGH;ioRising=level&&!ioGate;ioGate=level;}
 digitalWrite(PIN_INPUT_LED,(manualBtn.stable||cvGate)?HIGH:LOW);digitalWrite(PIN_IO_LED,selectedInput()?ioGate:digitalRead(PIN_OUT3)==HIGH);displayModes(nowMs);
 pendingCv|=cvRising;pendingManual|=manualBtn.rising;pendingTrigger|=cvRising||manualBtn.rising;
 pendingTriggerA|=cvRising||manualBtn.rising;pendingTriggerB|=ioRising||manualBtn.rising;
 if(!audioOwner){schService(nowMs);latchDac();}uint32_t dtUs=nowUs-lastUpdateUs;
 if(!selectedFast()&&dtUs<PROCESS_PERIOD_US)return;
 lastUpdateUs=nowUs;uint16_t dtMs=dtUs/1000UL;if(dtMs>20)dtMs=20;if(dtMs<1)dtMs=1;
 bool trig=pendingTrigger,a=pendingTriggerA,b=pendingTriggerB;resetTriggered=trig;
 switch(slot){${modes.map(m=>`case ${m.slot-1}:{using namespace ${m.id};${m.tick}break;}`).join('\n')}default:break;}
 modeJustChanged=false;resetTriggered=pendingTrigger=pendingTriggerA=pendingTriggerB=pendingCv=pendingManual=false;if(!audioOwner)latchDac();
}
`;
 // Global declarations must precede the untouched calibration functions.
 code=code.replace(sources.calibration,'uint32_t slotEnteredAt=0;\n'+sources.calibration);
 code=code.replace(/^ +$/gm,'');
 return {[`${stem}/${stem}.ino`]:`#include "firmware.hpp"\n`,[`${stem}/firmware.hpp`]:code};
}
export const OPERATING_NOTES=[
 'MODE breve al rilascio avanza di uno slot e ritorna al primo. Una modalità alla volta. Al cambio lo stato riparte, code ed eventi precedenti sono cancellati, senza trigger sintetico; i preset persistono.',
 'LED1..4, letti da sinistra a destra: pesi 1, 2, 4, 8. Slot 16 = tutti spenti (0000), non indica un guasto.',
 'I/O è digitale e cambia direzione secondo la modalità. Scollegare le sorgenti esterne prima di passare a un modo con I/O uscita o riavviare.',
 'Calibrazione: MODE+MANUAL al boot per 100 ms; rilasciare. P1/P2 centrati acquisiscono, poi correggono OUT1/2; P3 inutilizzato. MANUAL breve conferma 0/+5/-5 V; alla conferma finale MANUAL 1 s salva. MODE 1,2 s annulla. Serve un multimetro. ACAL 256..301 preservato.',
 'Range nominali software, non misure elettriche. Nessun nuovo collaudo sul modulo. Non caricare un binario con report di memoria non valido.'
];
export function buildReadme(c){return `# ${c.name}\nFirmware ${c.version}; Builder ${VERSION}\nTarget: Arduino Nano ATmega328P, 16 MHz, REV5-2.\n\nAprire la cartella sketch in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano, processore ATmega328P (Old Bootloader se montato).\nIl sorgente include SPI/EEPROM del core; nessuna libreria esterna. La calibrazione ACAL e i preset esistenti sono conservati.\n\nBuild riproducibile: nel Builder eseguire node tools/build.mjs configuration.json --out /percorso/output. Toolchain avr-gcc 7.3.0-atmel3.6.1-arduino7, core 1.8.6. Vedere README del Builder per installazione e variabili.\n\nIl file .hex viene incluso soltanto dopo compilazione riuscita; verificare build-report.json (flash <=30720, SRAM statica + riserva stack 384 <=2048). Lo stack è una riserva, non un limite dimostrato. Upload manuale tramite Arduino IDE; nessun caricamento automatico.\n\n${OPERATING_NOTES.map(n=>'- '+n).join('\n')}\n`;}
