// Regenerate the chord color table using the sine and Wavetable bank 1.
const fs=require('node:fs');
const text=fs.readFileSync(require('node:path').join(__dirname,'../ALTERIA_MACRO_VOICE.ino'),'utf8');
const list=s=>s.split(',').map(Number);
const sine=list(text.match(/SINE\[64\] PROGMEM=\{([^{}]+)\};/)[1]);
const bank=text.split('WAVES[3][4][4][64] PROGMEM=')[1].split('};')[0];
const waves=[...bank.matchAll(/\{([-\d,]+)\}/g)].map(m=>list(m[1]));
if(sine.length!==64||waves.length!==48||waves.some(w=>w.length!==64))throw Error('Unexpected table layout');
console.log('// 64 pre-mixed timbres per band: one table read per chord oscillator.');
console.log('const int8_t CHORD_COLOR[3][64][64] PROGMEM={');
for(let band=0;band<3;band++){
  console.log('  {');
  for(let color=0;color<64;color++){
    const frame=sine.map((s,i)=>Math.round((s*(63-color)+waves[band*4+2][i]*color)/63));
    console.log('    {'+frame.join(',')+'},');
  }
  console.log('  },');
}
console.log('};');
