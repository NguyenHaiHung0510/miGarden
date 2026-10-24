import {access} from 'node:fs/promises';
import {execFile} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
import {createViewerServer} from './server.mjs';

const args=process.argv.slice(2),at=args.indexOf('--port');
const firstPort=Number(at>=0?args[at+1]:(process.env.MIGARDEN_PORT||4181));
if(!Number.isInteger(firstPort)||firstPort<1024||firstPort>65526)throw new Error('Choose --port between1024 and65526.');
const root=path.dirname(fileURLToPath(import.meta.url));
try{await access(path.join(root,'node_modules/three/package.json'));}catch{throw new Error('Run npm ci --ignore-scripts --prefix viewer from the repository root first.');}

function show(port,reused){
 const url=`http://127.0.0.1:${port}/?layout=space`;
 console.log(`${reused?'miGarden is already running':'miGarden is ready'}: ${url}`);
 if(!reused)console.log('Keep this window open. Press Ctrl+C here to stop the viewer.');
 if(args.includes('--no-browser'))return;
 const command=process.platform==='win32'?'cmd.exe':process.platform==='darwin'?'open':'xdg-open';
 const commandArgs=process.platform==='win32'?['/d','/c','start','',url]:[url];
 execFile(command,commandArgs,{windowsHide:true},error=>{if(error)console.log(`Open the viewer manually: ${url}`);});
}
let opened=false;
for(let offset=0;offset<10;offset++){
 const port=firstPort+offset,origin=`http://127.0.0.1:${port}`;
 try{
  const response=await fetch(origin+'/model.json',{signal:AbortSignal.timeout(700)}),m=await response.json();
  if(m.model_version?.startsWith('model-v0-')&&m.title==='Từ sân thượng đến ban công'){show(port,true);opened=true;break;}
 }catch{}
 const server=await createViewerServer();
 try{
  await new Promise((resolve,reject)=>{server.once('error',reject);server.listen(port,'127.0.0.1',resolve);});
  show(port,false);opened=true;break;
 }catch(error){if(error.code!=='EADDRINUSE')throw error;}
}
if(!opened)throw new Error('No free port in the next10 ports. Try --port with another value.');
