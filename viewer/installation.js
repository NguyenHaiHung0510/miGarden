// The model owns stage, positions and connectivity. These helpers adapt it to
// drawables; neither renderer supplies its own installation coordinates.
export function installationFeatures(model) {
  const route=model.water_route;
  const start=route.points[0],end=route.points.at(-1),previous=route.points.at(-2);
  const delta=end.map((v,i)=>v-previous[i]),length=Math.hypot(...delta);
  const capStart=length>0?end.map((v,i)=>v-delta[i]*Math.min(.055/length,1)):end;
  const {paths,termination,deferred}=model.installation;
  return [
    ...paths.map(path=>{
      const {points,connect,style}=path;
      if(connect==='route-start')return {...path,points:[...points,start]};
      if(connect==='route-end')return {...path,points:[end,...points]};
      if(style==='hose')return {...path,points:sampleHose(points)};
      return path;
    }),
    {...route,style:'rigid'},
    {id:termination.id,stage:route.stage,style:'end-cap',points:[capStart,end]},
    {id:deferred.junction,stage:'deferred',style:'junction',points:[end]}
  ];
}

function sampleHose([a,b,c,d]) {
  return Array.from({length:25},(_,i)=>{
    const t=i/24,u=1-t;
    return a.map((v,k)=>u**3*v+3*u*u*t*b[k]+3*u*t*t*c[k]+t**3*d[k]);
  });
}

export function installationPose(model,id) {
  if(id===model.installation.termination.id||id===model.installation.deferred.junction)return model.water_route.points.at(-1);
  return model.objects.find(o=>o.id===id)?.pose;
}

export function installationStage(model,id) {
  if(id===model.water_route.id||id===model.installation.termination.id)return model.water_route.stage;
  if(id===model.installation.deferred.junction)return 'deferred';
  return model.installation.paths.find(path=>path.id===id)?.stage;
}

export const installationPickId=id=>({
  'tap-adapter':'F','wash-adapter':'household-outlet','dnv5-cap-1':'dnv5','dnv5-cap-2':'dnv5'
}[id]||id);
