import http from 'node:http';import fs from 'node:fs/promises';import path from 'node:path';import os from 'node:os';import {fileURLToPath}from'node:url';import {build}from'./tools/build.mjs';import{zipFiles}from'./src/artifacts.mjs';import{compilerStatus}from'./src/compiler.mjs';
const root=path.dirname(fileURLToPath(import.meta.url)),port=Number(process.env.PORT||4173);let busy=false;
http.createServer(async(req,res)=>{try{
 const expected=`127.0.0.1:${port}`;if(![expected,`localhost:${port}`].includes(req.headers.host)){res.writeHead(403);return res.end('Host non consentito');}
 if(req.headers.origin&&!['http://'+expected,`http://localhost:${port}`].includes(req.headers.origin)){res.writeHead(403);return res.end('Origin non consentito');}
 const url=new URL(req.url,'http://'+expected),json=(code,data)=>{res.writeHead(code,{'Content-Type':'application/json','Cache-Control':'no-store'});res.end(JSON.stringify(data));};
 if(url.pathname==='/api/status')return json(200,await compilerStatus());
 if(url.pathname==='/api/build'&&req.method==='POST'){
  if(busy)return json(429,{error:'Una compilazione è già in corso.'});
  let body='',size=0;for await(const chunk of req){size+=chunk.length;if(size>65536)return json(413,{error:'Configurazione troppo grande.'});body+=chunk;}
  let input;try{input=JSON.parse(body);}catch{return json(400,{error:'JSON non valido.'});}
  if(busy)return json(429,{error:'Una compilazione è già in corso.'});busy=true;
  let dir;try{dir=await fs.mkdtemp(path.join(os.tmpdir(),'alteria-build-'));}catch(e){busy=false;throw e;}
  try{const result=await build(input,dir);return json(200,{report:result.report,zip:Buffer.from(zipFiles(result.files)).toString('base64')});}finally{busy=false;await fs.rm(dir,{recursive:true,force:true});}
 }
 if(req.method!=='GET'){res.writeHead(405);return res.end();}
 const relative=decodeURIComponent(url.pathname==='/'?'/index.html':url.pathname),file=path.resolve(root,'.'+relative);
 if(!file.startsWith(root+path.sep)||!/^\/(index.html|app.js|styles.css|src\/[\w.-]+\.mjs|data\/[\w.-]+\.json)$/.test(relative)){res.writeHead(404);return res.end('Non trovato');}
 const mime={'.html':'text/html; charset=utf-8','.js':'text/javascript','.mjs':'text/javascript','.css':'text/css','.json':'application/json'}[path.extname(file)];res.writeHead(200,{'Content-Type':mime,'Cache-Control':'no-cache','X-Content-Type-Options':'nosniff'});res.end(await fs.readFile(file));
 }catch(e){res.writeHead(400,{'Content-Type':'application/json'});res.end(JSON.stringify({error:e.message}));}}).listen(port,'127.0.0.1',()=>console.log(`ALTERIA Firmware Builder · http://127.0.0.1:${port}`));
