export function routeMetrics(points, scale = [1,1,1]) {
  const segments = points.slice(1).map((p,index)=>{
    const delta=p.map((v,i)=>(v-points[index][i])*scale[i]);
    return {from:index,to:index+1,length:Math.hypot(...delta),delta};
  });
  return {segments,total:segments.reduce((sum,s)=>sum+s.length,0)};
}
export function estimateTriangle(diagonal, transverse) {
  if(!Number.isFinite(diagonal)||!Number.isFinite(transverse)||transverse<0||diagonal<=transverse)throw new Error('Cạnh chéo phải lớn hơn cạnh ngang.');
  const longitudinal=Math.sqrt(diagonal**2-transverse**2);
  return {diagonal,transverse,longitudinal,angleDegrees:Math.atan2(longitudinal,transverse)*180/Math.PI,folded:transverse+longitudinal};
}
export function validPoints(points) {
  return Array.isArray(points)&&points.length>=2&&points.length<=256&&points.every(p=>Array.isArray(p)&&p.length===3&&p.every(Number.isFinite));
}

// Conservative tube-vs-solid check: expand each solid by the tube radius.
export function segmentHitsBox(a,b,bounds,radius=0){
  let enter=0,exit=1;
  for(let axis=0;axis<3;axis++){
    const lo=bounds.min[axis]-radius,hi=bounds.max[axis]+radius,d=b[axis]-a[axis];
    if(Math.abs(d)<1e-12){if(a[axis]<lo||a[axis]>hi)return false;continue;}
    const u=(lo-a[axis])/d,v=(hi-a[axis])/d;enter=Math.max(enter,Math.min(u,v));exit=Math.min(exit,Math.max(u,v));
    if(enter>exit)return false;
  }return true;
}
export function siteWallBoxes(model){
  const l=model.layout,w=l.rightLedge,n=l.rightWall,z=l.zoneB,d=l.parapet,left=l.leftWall;
  return [
    {id:'right-ledge',group:'upper',min:[w.minX,w.floor,w.minZ],max:[w.maxX,w.floor+w.height,w.maxZ]},
    {id:'right-wall',group:'building',min:[n.x-n.thickness/2,n.minY,n.minZ],max:[n.x+n.thickness/2,n.maxY,n.maxZ]},
    {id:'left-wall',group:'room',min:[left.x-.05,0,left.minZ],max:[left.x+.05,left.height,left.maxZ]},
    {id:'terrace-divider',group:'upper',min:[l.terrace.minX,l.terrace.floor,d.z-d.thickness/2],max:[l.terrace.maxX,l.terrace.floor+d.height_m,d.z+d.thickness/2]},
    {id:'mini-low-walls',part:'left',group:'upper',min:[z.minX,z.floor,z.minZ],max:[z.minX+z.wallThickness,z.floor+z.wallHeight,z.maxZ]},
    {id:'mini-low-walls',part:'right',group:'upper',min:[z.maxX-z.wallThickness,z.floor,z.minZ],max:[z.maxX,z.floor+z.wallHeight,z.maxZ]},
    {id:'mini-edge',group:'upper',min:[z.minX,z.floor,z.maxZ-z.wallThickness],max:[z.maxX,z.floor+z.wallHeight,z.maxZ]}
  ];
}
export function routeWallClashes(points,model){
  const radius=model.routing_geometry?.tube_radius_m??.01,boxes=siteWallBoxes(model),hits=[];
  for(let i=1;i<points.length;i++)for(const box of boxes)if(segmentHitsBox(points[i-1],points[i],box,radius))hits.push({segment:i-1,wall:box.id,part:box.part});
  return hits;
}
export function routeObstacleClashes(points,model){
  const radius=model.routing_geometry?.tube_radius_m??.01,hits=routeWallClashes(points,model);
  for(const pipe of model.existing_pipes||[]){
    const bounds={min:[0,1,2].map(i=>Math.min(...pipe.points.map(p=>p[i]))-pipe.radius),max:[0,1,2].map(i=>Math.max(...pipe.points.map(p=>p[i]))+pipe.radius)};
    for(let i=1;i<points.length;i++)if(segmentHitsBox(points[i-1],points[i],bounds,radius))hits.push({segment:i-1,wall:pipe.id});
  }return hits;
}
