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
test('one estimated spatial source preserves corrected site and renderable route choices',()=>{
 assert.equal(model.coordinate_frame.kind,'synthetic-estimate');
 assert.equal(model.layout.zoneB.maxX,model.layout.terrace.maxX);
 assert.equal(model.source_assembly.tank_yaw_degrees,90);
 for(const object of model.objects)for(const id of object.sources)assert.ok(model.sources[id],`${object.id} -> ${id}`);
 for(const[key,preset]of Object.entries(model.route_presets)){
  assert.ok(validPoints(preset.points));assert.equal(preset.points.length,preset.point_labels.length);assert.ok(routeMetrics(preset.points,model.coordinate_frame.estimated_meters_per_scene_unit).total>0);
  for(const projection of ['plan','section']){
   const copy=structuredClone(model);copy.water_route.points=preset.points;
   const svg=drawSVG(copy,{projection,selected:'F',pots:copy.pot_samples,layers:{water:true,pots:true,room:true,dimensions:true}});
   assert.ok(svg.includes(copy.model_version));assert.ok(!/NaN|undefined|Infinity/.test(svg),`${key}/${projection}`);
  }
 }
});
test('standalone clone serves without raw images and persists guarded local feedback',async()=>{
 await mkdir(localRoot,{recursive:true});const fixture=await mkdtemp(path.join(localRoot,'viewer-test-'));
 const server=await createViewerServer({feedbackDir:fixture,sourceManifest:null});
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));const origin=`http://127.0.0.1:${server.address().port}`;
 try{
  assert.equal((await fetch(origin+'/')).status,200);
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
