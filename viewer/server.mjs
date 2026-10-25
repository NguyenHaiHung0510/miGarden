import http from 'node:http';
import {readFile,stat,mkdir,writeFile,rename} from 'node:fs/promises';
import {randomUUID,createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
import path from 'node:path';

const root=path.dirname(fileURLToPath(import.meta.url));
const repoRoot=path.dirname(root),privateRoot=path.join(repoRoot,'.local');
const within=(base,candidate)=>candidate.startsWith(base+path.sep);
const files=new Set(['index.html','style.css','app.js','scene.js','drawing.js','route-math.js','route-editor.js','model.json']);
const types={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.json':'application/json; charset=utf-8','.png':'image/png','.jpg':'image/jpeg','.svg':'image/svg+xml'};
const headers={'Cache-Control':'no-store','X-Content-Type-Options':'nosniff','Referrer-Policy':'no-referrer'};
function json(res,status,value){res.writeHead(status,{...headers,'Content-Type':'application/json; charset=utf-8'});res.end(JSON.stringify(value));}
export async function viewerBuildId(){const hash=createHash('sha256');for(const name of [...files,'server.mjs'].sort()){hash.update(name);hash.update(await readFile(path.join(root,name)));}return hash.digest('hex');}

export async function createViewerServer({feedbackDir=path.join(privateRoot,'viewer-feedback'),sourceManifest=path.join(privateRoot,'viewer-sources.json')}={}){
 const model=JSON.parse(await readFile(path.join(root,'model.json'),'utf8'));
 const buildId=await viewerBuildId();
 const versions=new Set([model.model_version,'model-v0-draft-03','model-v0-draft-04','model-v0-draft-05']);
 const sourceFiles=new Map();
 if(sourceManifest)try{
  const sources=JSON.parse((await readFile(sourceManifest,'utf8')).replace(/^\uFEFF/,''));
  for(const source of sources){
   if(!/^[A-Za-z0-9_-]{1,40}$/.test(source.id)||typeof source.path!=='string')continue;
   const target=path.resolve(repoRoot,source.path);
   if(within(privateRoot,target)&&(await stat(target)).isFile())sourceFiles.set(source.id,target);
  }
 }catch(error){if(error.code!=='ENOENT')throw error;}
 const feedbackFile=path.join(feedbackDir,'current-draft.json');
 let queue=Promise.resolve();
 async function getDraft(){try{return JSON.parse(await readFile(feedbackFile,'utf8'));}catch(error){if(error.code==='ENOENT')return {revision:0,draft:null};throw error;}}
 const server=http.createServer(async(req,res)=>{
  try{
   const url=new URL(req.url,'http://127.0.0.1');
   if(url.pathname==='/api/info'){if(req.method!=='GET'){json(res,405,{error:'Method not allowed'});return;}json(res,200,{model_version:model.model_version,build_id:buildId,profile:path.resolve(feedbackDir)===path.join(privateRoot,'viewer-feedback')?'owner':'isolated'});return;}
   if(url.pathname==='/api/sources'){
    if(req.method!=='GET'){json(res,405,{error:'Method not allowed'});return;}
    json(res,200,{available:[...sourceFiles.keys()]});return;
   }
   if(url.pathname==='/api/feedback'){
    if(req.method==='GET'){json(res,200,await getDraft());return;}
    if(req.method!=='POST'){json(res,405,{error:'Method not allowed'});return;}
    if(req.headers.origin!==`http://127.0.0.1:${server.address().port}`){json(res,403,{error:'Chỉ lưu từ viewer local.'});return;}
    let raw='';for await(const chunk of req){raw+=chunk;if(Buffer.byteLength(raw)>262144){json(res,413,{error:'Bản nháp quá lớn; hãy xuất dự phòng.'});return;}}
    let draft;try{draft=JSON.parse(raw);}catch{json(res,400,{error:'JSON không hợp lệ.'});return;}
    const points=draft?.route?.points;
    if(!draft||!versions.has(draft.model_version)||!Number.isInteger(draft.expectedRevision)||!Array.isArray(points)||points.length<2||points.length>256||!points.every(p=>Array.isArray(p)&&p.length===3&&p.every(Number.isFinite))||typeof draft.comment!=='string'||(draft.source_option!==undefined&&!['reuse','multi'].includes(draft.source_option))){json(res,400,{error:'Bản nháp thiếu version hoặc tọa độ hợp lệ.'});return;}
    queue=queue.then(async()=>{
     const current=await getDraft();
     if(draft.expectedRevision!==current.revision){json(res,409,{error:'Đã có bản lưu mới. Nháp trên màn hình vẫn giữ; hãy xuất dự phòng trước khi tải lại.'});return;}
     const id=randomUUID(),record={revision:current.revision+1,saved_at:new Date().toISOString(),draft};
     await mkdir(feedbackDir,{recursive:true});const body=JSON.stringify(record,null,2)+'\n';
     await writeFile(path.join(feedbackDir,`snapshot-${id}.json`),body,{flag:'wx'});
     const temp=path.join(feedbackDir,`current-${id}.tmp`);await writeFile(temp,body);await rename(temp,feedbackFile);
     json(res,200,{revision:record.revision,saved_at:record.saved_at});
    }).catch(()=>{if(!res.headersSent)json(res,500,{error:'Chưa xác nhận lưu được. Nháp vẫn ở trình duyệt; hãy xuất dự phòng.'});});
    await queue;return;
   }
   if(!['GET','HEAD'].includes(req.method)){res.writeHead(405);res.end();return;}
   const pathname=decodeURIComponent(url.pathname);let filename;
   if(pathname.startsWith('/source/'))filename=sourceFiles.get(pathname.slice(8));
   else{
    const relative=pathname==='/'?'index.html':pathname.slice(1);
    if(files.has(relative)||relative.startsWith('node_modules/three/build/')||relative.startsWith('node_modules/three/examples/jsm/controls/')){
     const target=path.resolve(root,relative);if(within(root,target)&&!relative.includes('..'))filename=target;
    }
   }
   if(!filename||!(await stat(filename)).isFile()){res.writeHead(404,headers);res.end('Not found');return;}
   const bytes=await readFile(filename);res.writeHead(200,{...headers,'Content-Type':types[path.extname(filename)]||'application/octet-stream'});res.end(req.method==='HEAD'?undefined:bytes);
  }catch{if(!res.headersSent)json(res,500,{error:'Không đọc được tài nguyên local.'});else res.end();}
 });
 return server;
}

if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const options={};
 if(process.env.MIGARDEN_FEEDBACK_DIR){const target=path.resolve(repoRoot,process.env.MIGARDEN_FEEDBACK_DIR);if(!within(privateRoot,target))throw new Error('Feedback must stay inside the repository .local directory.');options.feedbackDir=target;}
 const server=await createViewerServer(options);
 server.listen(Number(process.env.MIGARDEN_PORT||4179),'127.0.0.1',()=>console.log(`miGarden model-v0: http://127.0.0.1:${server.address().port}`));
}
