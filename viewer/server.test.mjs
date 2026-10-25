import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile,mkdtemp,mkdir,writeFile,rm} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {createViewerServer} from './server.mjs';
import {routeMetrics,validPoints} from './route-math.js';
import {drawSVG} from './drawing.js';

const root=path.dirname(fileURLToPath(import.meta.url)),repoRoot=path.dirname(root),localRoot=path.join(repoRoot,'.local');
const model=JSON.parse(await readFile(path.join(root,'model.json'),'utf8'));
test('shared metric baseline preserves measured endpoints, diameter and historical comparisons',()=>{
 assert.equal(model.coordinate_frame.kind,'owner-measured-with-estimates');
 assert.deepEqual(model.coordinate_frame.estimated_meters_per_scene_unit,[1,1,1]);
 const near=(a,b)=>assert.ok(Math.abs(a-b)<1e-9,`${a} != ${b}`),p=model.water_route.points;
 near(routeMetrics(p.slice(0,7)).total,8.9);near(routeMetrics(p).total,15.7);
 near(p[1][0]-p[0][0],.53);near(p[2][1]-p[1][1],.26);
 near(model.existing_pipes.find(v=>v.id==='existing-ledge-pipe').radius*2,.1);
 near(p[5][1]-p[6][1],.4);near(p[6][1]-p[7][1],2.6);
 near(model.layout.zoneB.maxZ-model.layout.zoneB.minZ,1.5);
 near(model.layout.zoneB.minZ-(model.layout.parapet.z+model.layout.parapet.thickness/2),0);
 near(model.layout.zoneB.maxX-model.layout.zoneB.minX,3.13);near(model.layout.zoneB.minX-model.layout.leftWall.innerX,1);
 near(Math.abs(p[8][2]-p[7][2]),.1);near(p[8][1]-p[9][1],.1);near(Math.abs(p[10][0]-p[9][0]),4);
 const ap=model.objects.find(v=>v.id==='AP').pose,window=model.objects.find(v=>v.id==='window-left').pose;
 near(ap[1],1.7);near(window[2]-ap[2],3);assert.equal(model.source_option,'multi');assert.ok(model.source_assembly.manual_isolation_before_future_solenoid);
 assert.equal(model.measurements.length,14);assert.equal(model.default_route_preset,'main');
 for(const[key,preset]of Object.entries(model.route_presets))assert.equal(preset.read_only,key!=='main');
 const ids=model.navigation_groups.flatMap(g=>g.ids);assert.equal(new Set(ids).size,ids.length);for(const o of model.objects)assert.ok(ids.includes(o.id));
 assert.equal(model.layout.zoneB.maxX,model.layout.terrace.maxX);
 assert.equal(model.source_assembly.tank_yaw_degrees,90);
 for(const object of model.objects)for(const id of object.sources)assert.ok(model.sources[id],`${object.id} -> ${id}`);
 for(const[key,preset]of Object.entries(model.route_presets)){
  assert.ok(validPoints(preset.points));assert.equal(preset.points.length,preset.point_labels.length);assert.ok(routeMetrics(preset.points,model.coordinate_frame.estimated_meters_per_scene_unit).total>0);
  for(const projection of ['plan','section']){
   const copy=structuredClone(model);copy.water_route.points=preset.points;
   const svg=drawSVG(copy,{projection,selected:'F',pots:copy.pot_samples,layers:{water:true,pots:true,room:true,dimensions:true}});
   assert.ok(svg.includes(copy.model_version));assert.ok(!/NaN|undefined|Infinity/.test(svg),`${key}/${projection}`);
   const clear=drawSVG(copy,{projection,selected:'F',pots:copy.pot_samples,layers:{water:true,pots:true,room:true,dimensions:false}});
   assert.ok(!clear.includes('8,90 m tổng'));assert.ok(!clear.includes('Vòi nguồn'));assert.ok(!clear.includes('data-dimension='));
  }
 }
});
test('standalone clone serves without raw images and persists guarded local feedback',async()=>{
 await mkdir(localRoot,{recursive:true});const fixture=await mkdtemp(path.join(localRoot,'viewer-test-'));
 const server=await createViewerServer({feedbackDir:fixture,sourceManifest:null});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const origin=`http://127.0.0.1:${server.address().port}`;
 try{
  assert.equal((await fetch(origin+'/')).status,200);
  const info=await(await fetch(origin+'/api/info')).json();assert.equal(info.profile,'isolated');assert.match(info.build_id,/^[a-f0-9]{64}$/);
  assert.equal((await fetch(origin+'/node_modules/three/build/three.module.js')).status,200);
  assert.deepEqual(await(await fetch(origin+'/api/sources')).json(),{available:[]});
  assert.equal((await fetch(origin+'/source/R04')).status,404);
  assert.equal((await fetch(origin+'/.local/viewer-sources.json')).status,404);
  assert.equal((await fetch(origin+'/server.mjs')).status,404);
  assert.deepEqual(await(await fetch(origin+'/api/feedback')).json(),{revision:0,draft:null});
  const preset=model.route_presets.right;
  const draft={model_version:model.model_version,expectedRevision:0,route:{points:preset.points,labels:preset.point_labels,preset:'right'},source_option:'multi',comment:'test-only feedback',notes:{},synthetic:true};
  const post=(value,from=origin)=>fetch(origin+'/api/feedback',{method:'POST',headers:{Origin:from,'Content-Type':'application/json'},body:JSON.stringify(value)});
  assert.equal((await post(draft,'https://example.invalid')).status,403);
  assert.equal((await post(null)).status,400);
  assert.equal((await post({...draft,source_option:'unknown'})).status,400);
  const saved=await post(draft);assert.equal(saved.status,200);assert.equal((await saved.json()).revision,1);
  const disk=JSON.parse(await readFile(path.join(fixture,'current-draft.json'),'utf8'));assert.equal(disk.draft.source_option,'multi');assert.deepEqual(disk.draft.route.points,preset.points);
  assert.equal((await post(draft)).status,409);
  const competing={...draft,expectedRevision:1};const replies=await Promise.all([post(competing),post(competing)]);assert.deepEqual(replies.map(r=>r.status).sort(),[200,409]);
 }finally{
  await new Promise(resolve=>server.close(resolve));
  // Only remove this freshly-created, named fixture inside this repository's ignored state.
  assert.equal(path.dirname(fixture),localRoot);assert.ok(path.basename(fixture).startsWith('viewer-test-'));await rm(fixture,{recursive:true,force:true});
 }
});
test('private source alias accepts only an ignored local file and never exposes filesystem paths',async()=>{
 await mkdir(localRoot,{recursive:true});const fixture=await mkdtemp(path.join(localRoot,'viewer-test-'));
 const photo=path.join(fixture,'source.png');await writeFile(photo,'fixture');const manifest=path.join(fixture,'sources.json');
 await writeFile(manifest,JSON.stringify([{id:'R04',path:path.relative(repoRoot,photo)},{id:'outside',path:'README.md'}]));
 const server=await createViewerServer({feedbackDir:fixture,sourceManifest:manifest});await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const origin=`http://127.0.0.1:${server.address().port}`;
 try{
  assert.deepEqual(await(await fetch(origin+'/api/sources')).json(),{available:['R04']});assert.equal(await(await fetch(origin+'/source/R04')).text(),'fixture');assert.equal((await fetch(origin+'/source/outside')).status,404);
 }finally{
  await new Promise(resolve=>server.close(resolve));assert.equal(path.dirname(fixture),localRoot);assert.ok(path.basename(fixture).startsWith('viewer-test-'));await rm(fixture,{recursive:true,force:true});
 }
});

test('edits arriving during a delayed save remain unsaved until another save',async()=>{
 const original={document:globalThis.document,localStorage:globalThis.localStorage,fetch:globalThis.fetch};
 const elements=new Map(),make=()=>({value:'',textContent:'',disabled:false,hidden:false,open:false,dataset:{},handlers:{},classList:{toggle(){return false;}},setAttribute(){},replaceChildren(){},append(){},addEventListener(name,fn){this.handlers[name]=fn;}});
 const get=id=>{if(!elements.has(id))elements.set(id,make());return elements.get(id);};
 try{
  globalThis.document={querySelector:get,querySelectorAll:()=>[],createElement:make,activeElement:null};
  const cached=new Map();globalThis.localStorage={setItem:(k,v)=>cached.set(k,v),getItem:k=>cached.get(k)||null};
  let finishPost,posted;globalThis.fetch=async(_url,options)=>{if(!options)return {ok:true,json:async()=>({revision:0,draft:null})};posted=JSON.parse(options.body);return await new Promise(resolve=>{finishPost=()=>resolve({ok:true,json:async()=>({revision:1})});});};
  const {initRouteEditor}=await import('./route-editor.js');
  const editor=initRouteEditor({model:structuredClone(model),getScene:()=>null,redraw(){},getNotes:()=>({}),restoreNotes(){}});
  await new Promise(resolve=>setImmediate(resolve));
  get('#route-comment').value='sent snapshot';get('#route-comment').handlers.input();const saving=get('#route-save').handlers.click();
  get('#route-comment').value='newer unsaved';get('#route-comment').handlers.input();finishPost();await saving;
  assert.equal(posted.comment,'sent snapshot');assert.equal(editor.diagnostics().revision,1);assert.equal(editor.diagnostics().dirty,true);assert.match(get('#route-save-status').textContent,/mới hơn chưa lưu/);
 }finally{for(const[key,value]of Object.entries(original))if(value===undefined)delete globalThis[key];else globalThis[key]=value;}
});
