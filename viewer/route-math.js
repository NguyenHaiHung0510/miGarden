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
