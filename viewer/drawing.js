import {routeMetrics} from './route-math.js';
import {installationFeatures,installationPose,installationStage,installationPickId} from './installation.js';
const escape=value=>String(value).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&apos;'}[c]));
export function drawSVG(model,state){
 const plan=state.projection==='plan',selected=state.selected,{layout:l}=model;
 const {stages}=model.installation;
 const upper=p=>p[1]>3,section=p=>[125+p[0]*160,450-p[1]*62];
 const planPoint=(p,force)=>force??upper(p)?[70+p[0]*76,92+(p[2]+.7)*37]:[580+p[0]*76,120+(p[2]-l.room.minZ)*55];
 const project=plan?planPoint:section;
 const text=(x,y,s,size=12,color='#5a6d58',extra='')=>`<text x="${x}" y="${y}" font-size="${size}" fill="${color}" ${extra}>${escape(s)}</text>`;
 const rect=(x,y,w,h,fill,extra='')=>`<rect x="${x}" y="${y}" width="${w}" height="${h}" fill="${fill}" ${extra}/>`;
 const path=(ps,color,width=3,extra='')=>`<polyline points="${ps.map(p=>project(p).join(',')).join(' ')}" fill="none" stroke="${color}" stroke-width="${width}" stroke-linecap="round" stroke-linejoin="round" ${extra}/>`;
 const bounds=(b,color,force,id)=>{const a=planPoint([b.minX,b.floor,b.minZ],force),z=planPoint([b.maxX,b.floor,b.maxZ],force);return rect(a[0],a[1],z[0]-a[0],z[1]-a[1],color,`stroke="#c3cdb7" data-id="${escape(id)}"`);};
 let body=rect(0,0,1000,520,'#fafaf4');
 body+=text(30,30,plan?'MẶT BẰNG · HAI TẦNG':'MẶT CẮT ĐỐI CHIẾU · X/Y',13,'#354f40','font-weight="600"');
 if(state.layers.dimensions)body+=text(30,51,'Mốc PA2 S → đỉnh rào: 11,90 m · tim uPVC vẽ ≈ '+routeMetrics(model.water_route.points).total.toFixed(2)+' m · không phải số cắt',11);
 if(plan){
  body+=text(70,78,'SÂN THƯỢNG + MINI',12)+text(580,78,'PHÒNG + BAN CÔNG CHÍNH',12);
  body+=bounds(l.terrace,'#eeeade',true,'terrace')+bounds(l.zoneB,'#e1e7d0',true,'zoneB');
  if(state.layers.room)body+=bounds(l.room,'#e9e9df',false,'room-area');
  body+=bounds(l.balcony,'#eee7d6',false,'balcony');
  body+=bounds({...l.rightLedge,floor:l.terrace.floor},'#c9c7b5',true,'right-ledge');
  const divider={minX:l.terrace.minX,maxX:l.terrace.maxX,minZ:l.parapet.z-l.parapet.thickness/2,maxZ:l.parapet.z+l.parapet.thickness/2,floor:l.terrace.floor};body+=bounds(divider,'#d1cdbb',true,'terrace-divider');
  for(const isUpper of [true,false])body+=bounds({minX:l.rightWall.x-l.rightWall.thickness/2,maxX:l.rightWall.x+l.rightWall.thickness/2,minZ:isUpper?l.terrace.minZ:l.room.minZ,maxZ:l.rightWall.maxZ,floor:isUpper?l.terrace.floor:0},'#c1c1b1',isUpper,'right-wall');
  const z=l.zoneB;for(const wall of [{minX:z.minX,maxX:z.minX+z.wallThickness,minZ:z.minZ,maxZ:z.maxZ},{minX:z.maxX-z.wallThickness,maxX:z.maxX,minZ:z.minZ,maxZ:z.maxZ},{minX:z.minX,maxX:z.maxX,minZ:z.maxZ-z.wallThickness,maxZ:z.maxZ}])body+=bounds({...wall,floor:z.floor},'#c6cbb5',true,'mini-low-walls');
 }else{
  const y=section([0,l.terrace.floor,0])[1],lower=section([0,0,0])[1];body+=rect(125,y,4.13*160,7,'#d5d0ba',`data-id="terrace"`)+rect(125,lower,4.13*160,7,'#d5d0ba',`data-id="balcony"`);
  const miniY=section([0,l.zoneB.floor,0])[1];body+=rect(section([1,0,0])[0],miniY,3.13*160,5,'#cad5b6',`data-id="zoneB"`);
  const rw=l.rightWall,a=section([rw.x-rw.thickness/2,rw.maxY,0]);body+=rect(a[0],a[1],rw.thickness*160,(rw.maxY-rw.minY)*62,'#c1c1b0',`data-id="right-wall"`);
  const led=l.rightLedge,at=section([led.minX,led.floor+led.height,0]);body+=rect(at[0],at[1],(led.maxX-led.minX)*160,led.height*62,'#cac6af',`data-id="right-ledge"`);
  if(state.layers.dimensions)body+=text(130,y-12,'Sân thượng · cao độ sàn ≈',11)+text(135,lower+27,'Ban công chính · Zone A',11);
 }
 if(state.layers.room){
  for(const id of ['W','window-left']){const o=model.objects.find(o=>o.id===id),a=project(o.pose);body+=rect(a[0]-(id==='W'?45:36),a[1]-(plan?3:40),id==='W'?90:72,plan?6:80,id==='W'?'#728173':'#a6c2b6',`stroke="#516c5b" data-id="${id}"`);}
 }
 const railA=project([l.railMinX,l.railTop,l.railZ]),railB=project([l.railMaxX,l.railTop,l.railZ]);body+=`<path d="M${railA.join(' ')}L${railB.join(' ')}" stroke="#617567" stroke-width="4" data-id="fence"/>`;
 if(!plan)for(let x=l.railMinX;x<=l.railMaxX;x+=.4)body+=path([[x,.1,l.railZ],[x,l.railTop,l.railZ]],'#748576',1.5,`data-id="fence"`);
 const tank=model.objects.find(o=>o.id==='tank'),at=project(tank.pose);body+=rect(at[0]-50,at[1]-28,100,56,'#bdc9bd',`rx="15" stroke="#829784" data-id="tank"`);
 for(const pipe of model.existing_pipes){const ps=pipe.points,a=project(ps[0]),b=project(ps[1]);body+=Math.hypot(b[0]-a[0],b[1]-a[1])<.1?`<circle cx="${a[0]}" cy="${a[1]}" r="${pipe.radius*62}" fill="#879c93" data-id="${pipe.id}"/>`:path(ps,'#879c93',Math.max(3,pipe.radius*120),`data-id="${pipe.id}"`);}
 function route(ps,color,width,extra=''){
  // Split plan routes at level changes; the cutaway keeps the full drop.
  let out='',part=[ps[0]];
  for(let i=1;i<ps.length;i++){
   if(plan&&upper(ps[i])!==upper(ps[i-1])){
    if(part.length>1)out+=path(part,color,width,extra);
    part=[];
   }
   part.push(ps[i]);
  }
  if(part.length>1)out+=path(part,color,width,extra);
  return out;
 }
 function marker(pos,r,fill,extra=''){
  const [x,y]=project(pos);return '<circle cx="'+x+'" cy="'+y+'" r="'+r+'" fill="'+fill+'" '+extra+'/>';
 }
 if(state.layers.water){
  const features=installationFeatures(model);
  if(model.comparison_route&&model.route_presets[model.comparison_route])body+=route(model.route_presets[model.comparison_route].points,'#8da4b0',2,'stroke-dasharray="6 4"');
  for(const {id,stage,style,points} of [...features].sort((a,b)=>Number(b.stage==='deferred')-Number(a.stage==='deferred'))){
   if(stage==='deferred'&&state.layers.future===false||style==='junction')continue;
   const color=stages[stage].color,attrs='data-id="'+installationPickId(id)+'" data-feature="'+id+'" data-stage="'+stage+'"';
   if(stage==='deferred')body+=route(points,color,2,attrs+' stroke-dasharray="5 5"');
   else if(style==='end-cap')body+=marker(points.at(-1),5.5,color,attrs+' stroke="#702c13" stroke-width="1.5"');
   else if(style==='port-cap')body+=path(points,'#97714b',3,attrs)+marker(points.at(-1),2.4,'#b59b7c',attrs+' stroke="#664d38"');
   else if(style==='solenoid')body+=path(points,'#385963',8,attrs)+marker(installationPose(model,id),4.5,'#385963',attrs);
   else if(style==='manifold')body+=path(points,'#957254',6,attrs);
   else if(style==='wash')body+=path(points,'#8b6b4f',3,attrs);
   else body+=route(points,color,style==='rigid'?4:3,attrs);
   if(plan&&style==='rigid'){
    const drop=points.findIndex((p,i)=>i>0&&upper(p)!==upper(points[i-1]));
    if(drop>0){
     body+=marker(points[drop-1],4,'#fff9f1',attrs+' stroke="'+color+'" stroke-width="2"');
     const [x,y]=project(points[drop-1]);body+=text(x+12,y-5,'↓ Xuống rào cũ',11,'#92451f');
    }
   }
  }
 }
 if(state.layers.pots)for(const p of state.pots){const id='pot:'+p.id,[x,y]=project([p.x,.2,p.z]);body+=`<g data-id="${escape(id)}" role="button" tabindex="0" aria-label="Chậu mẫu ${escape(p.id)}">${rect(x-8,y-3,16,13,'#c69b70',`rx="3" stroke="${selected===id?'#2e5d3b':'#9c986f'}"`)}<path d="M${x} ${y-2}v-13m0 6q-13 -12 -7 -14q10 1 7 9m0 1q12 -12 11 -3q-4 8 -11 7" fill="#7a9965"/></g>`;}
 if(state.layers.dimensions){
  const ids=plan?['M01','M03','M06','M07','M08','M12','M13']:['M02','M04','M05','M07','M09','M12'];
  for(const d of model.dimension_mappings.filter(d=>ids.includes(d.id))){const a=project(d.points[0]),b=project(d.points[1]),vertical=Math.abs(a[1]-b[1])>Math.abs(a[0]-b[0]);const offset=d.id==='M12'?22:d.id==='M07'?-27:d.id==='M08'?-12:9;
   body+=`<g data-dimension="${d.id}"><title>${escape(d.mapping)}</title><path d="M${a[0]} ${a[1]}L${b[0]} ${b[1]}" stroke="#62916c" stroke-dasharray="3 3"/>${text((a[0]+b[0])/2+(vertical?12:0),(a[1]+b[1])/2+offset,d.label,10,'#497452',vertical?'':'text-anchor="middle"')}</g>`;
  }
 }
 const ids=['F','future-valve',selected==='y-deferred'?'y-deferred':'capped-stub','zoneB','W','window-left',...(state.layers.room?['AP']:[])];
 if(!ids.includes(selected)&&model.objects.some(o=>o.id===selected))ids.push(selected);
 if(state.layers.dimensions)for(const id of ids){
  const stage=installationStage(model,id);
  if(['W','window-left','AP'].includes(id)&&!state.layers.room||stage&&!state.layers.water||stage==='deferred'&&state.layers.future===false)continue;
  const o=model.objects.find(o=>o.id===id),pos=installationPose(model,id),active=selected===id;
  if(!pos||o.status==='historical')continue;
  const [x,y]=project(pos),right=x<790,offsetY=id==='F'?25:id==='future-valve'?-13:-9;
  body+='<g data-id="'+id+'" role="button" tabindex="0" aria-label="'+escape(o.label)+'"><circle cx="'+x+'" cy="'+y+'" r="'+(active?5:3)+'" fill="'+(active?'#3c6746':'#fffdf1')+'" stroke="#547a55"/>'+text(x+(right?8:-8),y+offsetY,o.short,11,active?'#345d3c':'#4f624e',right?'':'text-anchor="end"')+'</g>';
 }
 const future=state.layers.future!==false?' · xám đứt: sau Y + phần nới ước lượng':'';
 body+=text(25,503,state.layers.water?'Cam liền: đợt một tới đầu chờ bịt'+future+' · chưa lắp':'Mét · mốc thước dây và phần ước lượng dùng chung với 3D.',11);
 return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1000 520" role="img" aria-label="${plan?'Mặt bằng hai tầng':'Mặt cắt đối chiếu'}" data-model-version="${model.model_version}" font-family="Segoe UI,system-ui,sans-serif"><title>${escape(model.title)}</title><desc>Baseline hiện trường có số đo Owner và phần ước lượng; không phải survey đầy đủ.</desc>${body}</svg>`;
}
