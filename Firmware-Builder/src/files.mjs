import fs from 'node:fs/promises';import path from 'node:path';import {randomUUID}from'node:crypto';
// Replacing a generated file atomically also avoids hydrating an evicted iCloud copy.
export async function writeAtomic(file,data){await fs.mkdir(path.dirname(file),{recursive:true});const temporary=path.join(path.dirname(file),'.'+path.basename(file)+'.'+randomUUID()+'.tmp');await fs.writeFile(temporary,data);await fs.rename(temporary,file);}
