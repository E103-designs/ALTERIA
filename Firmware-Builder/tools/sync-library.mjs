import fs from 'node:fs/promises';
import path from 'node:path';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { defaultConfig, generateFirmware, stemFor } from '../src/model.mjs';

const builder = fileURLToPath(new URL('../', import.meta.url));
const repo = path.dirname(path.resolve(builder));
const library = path.join(repo, 'Additional-Modes');
const check = process.argv.includes('--check');
const drift = [];
const json = value => JSON.stringify(value, null, 2) + '\n';
const read = name => fs.readFile(name, 'utf8');
async function write(file, body) {
  if (check) {
    if (await read(file).catch(e => e.code === 'ENOENT' ? null : Promise.reject(e)) !== body) {
      drift.push(path.relative(repo, file));
    }
  } else {
    await fs.mkdir(path.dirname(file), {recursive: true});
    await fs.writeFile(file, body);
  }
}
const catalogPath = path.join(builder, 'data/catalog.json');
const catalog = JSON.parse(await read(catalogPath));
const ids = new Set(catalog.modes.map(mode => mode.id));
if (ids.size !== catalog.modes.length) throw new Error('Duplicate mode IDs');
for (const entry of await fs.readdir(library, {withFileTypes: true})) {
  if (entry.isDirectory() && !ids.has(entry.name)) throw new Error('Uncatalogued mode: ' + entry.name);
}
const sources = {};
for (const name of ['runtime', 'scheduler', 'calibration', 'legacy_helpers', 'allrounder_math']) {
  sources[name] = await read(path.join(builder, 'runtime', name + '.inc'));
}
const base = defaultConfig(catalog);
const index = [];
const cell = value => value.replaceAll('|', '\\|');
for (const mode of catalog.modes) {
  const {id, name, version} = mode;
  if (!/^\w+$/.test(id) || mode.codeKey !== id || !/^\d+\.\d+(\.\d+)?$/.test(version)) throw new Error('Invalid mode: ' + id);
  const dir = path.join(library, id);
  sources[id] = await read(path.join(dir, 'Snippet/mode.inc'));
  mode.algorithmSha256 = createHash('sha256').update(sources[id]).digest('hex');
  const config = {...base, name: id, version: version.split('.').length === 2 ? version + '.0' : version, modes: [id]};
  const stem = stemFor(config);
  for (const [file, code] of Object.entries(generateFirmware(config, catalog, sources))) {
    await write(path.join(dir, 'Standalone', file), code);
  }
  const deps = mode.dependencies.length ? mode.dependencies.join(', ') : 'nessun adapter aggiuntivo';
  const controls = mode.controls.map(c => `| ${c.pot} | ${cell(c.function)} | ${cell(c.range)} · ${cell(c.curve)} · ${cell(c.acquisition)} |`).join('\n');
  await write(path.join(dir, 'README.md'), `# ${name}

**Versione ${version}** · ALTERIA REV5-2 / Arduino Nano ATmega328P, 16 MHz.

${mode.description}

| Controllo | Funzione | Intervallo e lettura |
| --- | --- | --- |
${controls}

| Jack o pulsante | Funzione |
| --- | --- |
| CV IN (A2) | ${cell(mode.cv)} |
| MANUAL (A3) | ${cell(mode.manual)} |
| OUT1 (MCP4822 A) | ${cell(mode.out1)} |
| OUT2 (MCP4822 B) | ${cell(mode.out2)} |
| I/O (D2, ${mode.ioDirection === 'input' ? 'ingresso' : 'uscita'}) | ${cell(mode.io)} |

Livelli nominali: ${mode.range}. ${mode.leds}

${mode.notes.map(note => '- ' + note).join('\n')}

## Standalone

Aprire [${stem}.ino](Standalone/${stem}/${stem}.ino) in Arduino IDE con Arduino AVR Boards 1.8.6, scheda Nano / ATmega328P. Tenere \`firmware.hpp\` nella stessa cartella. Il modo si avvia da solo. ${mode.composite ? 'MODE conserva le funzioni del motore.' : id === 'ar_presets8' ? 'MODE lungo alterna PLAY ed EDIT.' : 'MODE breve non scorre modalità.'} La combinazione di taratura al boot resta disponibile. SPI ed EEPROM sono fornite dal core AVR. Il collaudo sul modulo non fa parte di questa pubblicazione.

## Firmware Builder

Selezionare **${name}** (\`${id}\`). Lo [snippet](Snippet/mode.inc) viene inserito nel namespace della modalità; usa le API del [runtime REV5-2](../../Firmware-Builder/runtime/) e gli adapter del [catalogo](../../Firmware-Builder/data/catalog.json). Dipendenze: ${deps}. ${mode.reset}

Origine: \`${mode.source}\`. ${mode.sourceNote || ''}
`.trimEnd()+'\n');
  index.push(`| [${name}](${id}/) | ${version} | ${mode.family} | [Arduino](${id}/Standalone/${stem}/${stem}.ino) | [C++](${id}/Snippet/mode.inc) |`);
}
await write(catalogPath, json(catalog));
await write(path.join(builder, 'data/sources.json'), json(sources));
await write(path.join(builder, 'data/default.json'), json(base));
for (const [header, id] of [['euclidean_trigs', 'rh_euclidean_trigs'], ['sh_glide', 'cv_sh_glide']]) {
  await write(path.join(repo, 'ALLROUNDER/Firmware/ALTERIA_ALLROUNDER', header + '.h'),
    `// Generated from Additional-Modes/${id}/Snippet/mode.inc.\n#pragma once\n\n${sources[id]}`);
}
await write(path.join(library, 'README.md'), `# Additional Modes

${index.length} modalità per ALTERIA REV5-2. Ogni cartella contiene una guida, uno sketch Arduino autonomo e il frammento C++ utilizzato dal [Firmware Builder](../Firmware-Builder/).

` + '| Modalità | Versione | Famiglia | Standalone | Snippet |\n| --- | --- | --- | --- | --- |\n' + index.join('\n') + `

La versione 1.0 identifica la base di catalogo. 1.0.x indica una correzione, 1.x un aggiornamento compatibile, 2.0 un cambiamento incompatibile. Per modificare una modalità, aggiornare lo snippet e i metadati in \`Firmware-Builder/data/catalog.json\`, quindi eseguire \`npm run catalog\` dal Builder. Le copie generate vengono controllate da \`npm test\`.
`);
if (drift.length) throw new Error('Generated files are out of date:\n' + drift.join('\n'));
console.log(`${index.length} modes: library ${check ? 'verified' : 'generated'}`);
