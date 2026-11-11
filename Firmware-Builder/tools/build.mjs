import fs from 'node:fs/promises';import path from 'node:path';import {fileURLToPath} from 'node:url';
import {generateFirmware,validateConfig,buildReadme} from '../src/model.mjs';import {compileFirmware} from '../src/compiler.mjs';import {cheatsheetHtml,cheatsheetPdf} from '../src/artifacts.mjs';
import {writeAtomic}from'../src/files.mjs';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
export async function build(input,out,{compile=true}={}){const catalog=JSON.parse(await fs.readFile(path.join(root,'data/catalog.json'))),sources=JSON.parse(await fs.readFile(path.join(root,'data/sources.json')));const c=validateConfig(input,catalog),files=generateFirmware(c,catalog,sources);await fs.mkdir(out,{recursive:true});await fs.rm(path.join(out,'firmware.hex'),{force:true});
 const report=compile?await compileFirmware(files,out):{status:'not-requested',ok:false,message:'Solo sorgenti; memoria non misurata.'};
 files['configuration.json']=JSON.stringify(c,null,2)+'\n';files['cheatsheet.html']=cheatsheetHtml(c,catalog);files['cheatsheet.pdf']=cheatsheetPdf(c,catalog);files['README.md']=buildReadme(c);
 if(report.hex){files['firmware.hex']=report.hex;delete report.hex;}
 files['build-report.json']=JSON.stringify(report,null,2)+'\n';
 for(const [name,data]of Object.entries(files))await writeAtomic(path.join(out,name),data);
 return {files,report,configuration:c};}
if(process.argv[1]===fileURLToPath(import.meta.url)){const input=process.argv[2],out=process.argv.includes('--out')?process.argv[process.argv.indexOf('--out')+1]:path.join(root,'../output/builder/custom');if(!input)throw Error('Uso: node tools/build.mjs configuration.json --out DIRECTORY [--sources-only]');const result=await build(JSON.parse(await fs.readFile(input)),out,{compile:!process.argv.includes('--sources-only')});console.log(JSON.stringify({out,report:result.report},null,2));if(result.report.status==='failed'||result.report.status==='compiled'&&!result.report.ok)process.exitCode=1;}
