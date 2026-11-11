import {selected,OPERATING_NOTES,VERSION} from './model.mjs';
import metrics from './font-metrics.mjs';
export const escapeHtml=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
export function modeRows(m){return [
 ...m.controls.map(c=>[c.pot,`${c.function}${c.function.includes(c.range)?'':'. '+c.range}; curva: ${c.curve}; ${c.acquisition}.`]),
 ['CV IN',m.cv],['MANUAL',m.manual],['OUT1',m.out1],['OUT2',m.out2],['I/O',`${m.ioDirection==='input'?'INGRESSO':'USCITA'}: ${m.io}`],['Range',m.range],...(!m.leds.startsWith('Slot binario')?[['LED',m.leds]]:[]),...m.notes.map(n=>['Nota',n])];}
export function cheatsheetHtml(c,catalog){const modes=selected(c,catalog),e=escapeHtml;return `<!doctype html><html lang="it"><meta charset="utf-8"><title>${e(c.name)} · Cheatsheet</title><style>
@page{size:A4;margin:15mm}*{box-sizing:border-box}body{font:11pt/1.4 system-ui,sans-serif;color:#202724;max-width:180mm;margin:20px auto}h1{font-size:24pt;line-height:1.1}h2{font-size:15pt}small{font-size:10pt}table{border-collapse:collapse;width:100%;margin-bottom:1em}th,td{padding:5px 8px;text-align:left;border-bottom:1px solid #d8ddd8;vertical-align:top}th{width:20%}article{break-inside:avoid;border-top:3px solid #64783b;margin-top:24px;padding-top:8px}.led{font-family:monospace;letter-spacing:2px}li{margin-bottom:5px}footer{font-size:9pt;color:#57605b}@media screen{body{padding:20px}button{padding:12px}}@media print{button{display:none}body{margin:0;padding:0}}
</style><body><button onclick="window.print()">Stampa / salva PDF</button><header><small>ALTERIA / PERSONAL FIRMWARE · BUILDER ${VERSION}</small><h1>${e(c.name)}</h1><p>Firmware ${e(c.version)} · ${modes.length} slot · Nano ATmega328P / REV5-2</p></header><table><tr><th>Slot / LED1..4</th><th>Modalità</th><th>Famiglia</th></tr>${modes.map(m=>`<tr><td>${m.slot} / <span class="led">${m.led}</span></td><td>${e(m.name)}</td><td>${e(m.family)}</td></tr>`).join('')}</table><ul>${OPERATING_NOTES.map(n=>`<li>${e(n)}</li>`).join('')}</ul>${modes.map(m=>`<article data-mode="${e(m.id)}"><small>SLOT ${m.slot} · LED1..4 <span class="led">${m.led}</span> · ${e(m.family)}</small><h2>${e(m.name)}</h2><p>${e(m.description)}</p><table>${modeRows(m).map(([k,v])=>`<tr><th>${e(k)}</th><td>${e(v)}</td></tr>`).join('')}</table></article>`).join('')}<footer>Configurazione schema 1 · catalogo ${e(c.catalogVersion)}. Range software nominali. Test hardware non eseguiti su questa build.</footer></body></html>`;}
// Dependency-free PDF 1.4, A4, embedded page tree and Helvetica/WinAnsi.
// Fixed 10.5pt body with measured line budget; never shrink text to fit a page.
const latin=s=>String(s).replace(/[’‘]/g,"'").replace(/[“”]/g,'"').replace(/[–—·−]/g,'-').replace(/→/g,'->').replace(/…/g,'...').replace(/[^\x20-\xff\n]/g,'?');
const pdfString=s=>latin(s).split('').map(c=>c==='('||c===')'||c==='\\'?'\\'+c:c.charCodeAt(0)>126?'\\'+c.charCodeAt(0).toString(8).padStart(3,'0'):c).join('');
function wrap(text,width=507,size=10.5,bold=false){const widths=metrics[bold?'Helvetica-Bold':'Helvetica'],measure=s=>[...s].reduce((n,c)=>n+(widths[c.charCodeAt(0)-32]||600),0)*size/1000,lines=[];let line='';for(let word of latin(text).split(/\s+/)){if(measure(line+(line?' ':'')+word)>width&&line){lines.push(line);line='';}while(measure(word)>width){let i=1;while(i<word.length&&measure(word.slice(0,i+1))<=width)i++;lines.push(word.slice(0,i));word=word.slice(i);}line+=(line?' ':'')+word;}if(line)lines.push(line);return lines;}
export function cheatsheetPdf(c,catalog){
 const pages=[];let content='',y=790;const newPage=()=>{if(content)pages.push(content);content='';y=790;};
 function line(text,size=10.5,bold=false){if(y<55)newPage();content+=`BT /${bold?'F2':'F1'} ${size} Tf 1 0 0 1 44 ${y} Tm (${pdfString(text)}) Tj ET\n`;y-=size+4;}
 const para=(text,bold=false)=>wrap(text).forEach(t=>line(t,10.5,bold));
 line('ALTERIA / PERSONAL FIRMWARE',10.5,true);y-=10;wrap(c.name,507,22,true).forEach(t=>line(t,22,true));para(`Firmware ${c.version} - Builder ${VERSION} - Nano ATmega328P / REV5-2`);y-=12;
 for(const m of selected(c,catalog))para(`${String(m.slot).padStart(2,'0')}   LED1..4 ${m.led}   ${m.name}`);
 y-=12;for(const n of OPERATING_NOTES){para(n);y-=6;}
 newPage();
 for(const m of selected(c,catalog)){
  const rows=modeRows(m),lines=rows.flatMap(([k,v])=>wrap(k+': '+v));const estimate=65+lines.length*14.5;
  if(y-estimate<55&&y<780)newPage();
  line(`SLOT ${m.slot} / LED1..4 ${m.led} / ${m.family}`,10.5,true);y-=5;line(m.name,16,true);para(m.description);y-=6;
  rows.forEach(([k,v])=>para(k+': '+v));y-=22;
 }
 if(content)pages.push(content);
 const objects=['<< /Type /Catalog /Pages 2 0 R >>','', '<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>','<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>'];
 const kids=[];pages.forEach((p,i)=>{const pageId=objects.length+1,streamId=pageId+1;kids.push(`${pageId} 0 R`);const footer=`BT /F1 9 Tf 1 0 0 1 44 30 Tm (ALTERIA - ${pdfString(c.version)} - ${i+1} / ${pages.length}) Tj ET\n`;const stream=p+footer;objects.push(`<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595.28 841.89] /Resources << /Font << /F1 3 0 R /F2 4 0 R >> >> /Contents ${streamId} 0 R >>`);objects.push(`<< /Length ${stream.length} >>\nstream\n${stream}endstream`);});
 objects[1]=`<< /Type /Pages /Count ${pages.length} /Kids [${kids.join(' ')}] >>`;let out='%PDF-1.4\n',offset=[0];objects.forEach((obj,i)=>{offset.push(out.length);out+=`${i+1} 0 obj\n${obj}\nendobj\n`;});const xref=out.length;out+=`xref\n0 ${objects.length+1}\n0000000000 65535 f \n`+offset.slice(1).map(n=>`${String(n).padStart(10,'0')} 00000 n \n`).join('')+`trailer\n<< /Size ${objects.length+1} /Root 1 0 R >>\nstartxref\n${xref}\n%%EOF\n`;return new TextEncoder().encode(out);
}
function crc32(data){let c=0xffffffff;for(const v of data){c^=v;for(let k=0;k<8;k++)c=c&1?(c>>>1)^0xedb88320:c>>>1;}return (c^0xffffffff)>>>0;}
export function zipFiles(files){const enc=new TextEncoder(),local=[],central=[];let offset=0;function header(size){const a=new Uint8Array(size);return [a,new DataView(a.buffer)];}
 for(const [name,body]of Object.entries(files)){const n=enc.encode(name),b=typeof body==='string'?enc.encode(body):body,crc=crc32(b);const [h,v]=header(30);v.setUint32(0,0x04034b50,true);v.setUint16(4,20,true);v.setUint16(6,0x800,true);v.setUint16(12,0x21,true);v.setUint32(14,crc,true);v.setUint32(18,b.length,true);v.setUint32(22,b.length,true);v.setUint16(26,n.length,true);local.push(h,n,b);
 const [ch,cv]=header(46);cv.setUint32(0,0x02014b50,true);cv.setUint16(4,20,true);cv.setUint16(6,20,true);cv.setUint16(8,0x800,true);cv.setUint16(14,0x21,true);cv.setUint32(16,crc,true);cv.setUint32(20,b.length,true);cv.setUint32(24,b.length,true);cv.setUint16(28,n.length,true);cv.setUint32(42,offset,true);central.push(ch,n);offset+=h.length+n.length+b.length;}
 const centralSize=central.reduce((s,b)=>s+b.length,0),[end,ev]=header(22);ev.setUint32(0,0x06054b50,true);ev.setUint16(8,Object.keys(files).length,true);ev.setUint16(10,Object.keys(files).length,true);ev.setUint32(12,centralSize,true);ev.setUint32(16,offset,true);const result=new Uint8Array(offset+centralSize+22);let p=0;for(const b of [...local,...central,end]){result.set(b,p);p+=b.length;}return result;
}
