import {routeMetrics,validPoints,routeObstacleClashes} from './route-math.js';
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)],clone=v=>structuredClone(v);
export function initRouteEditor({model,getScene,redraw,getNotes,restoreNotes}){
 let points=clone(model.water_route.points),labels=[...model.water_route.point_labels],preset='main',selected=0,revision=0,editing=false,dirty=false,previous=null,buffer=model.pipe_allowance.buffer_m,comparison=null,editGeneration=0;
 const history=[],baseline=clone(model.water_route),cacheKey='migarden-route-'+model.model_version+'-'+model.installation_revision;
 function pack(){return {model_version:model.model_version,baseline_revision:model.baseline_revision,installation_revision:model.installation_revision,expectedRevision:revision,route:{points:clone(points),labels:[...labels],preset},source_option:model.source_option,comment:$('#route-comment').value,notes:clone(getNotes()),buffer_m:buffer,coordinate_kind:model.coordinate_frame.kind,baseline_date:model.baseline_date,client_saved_at:new Date().toISOString()};}
 function cache(){try{localStorage.setItem(cacheKey,JSON.stringify(pack()));}catch{}}
 function markDirty(){editGeneration++;dirty=true;$('#route-save-status').textContent='Có thay đổi chưa lưu cho Agent. Nháp giữ riêng; số đo baseline không bị ghi đè.';cache();}
 function remember(){history.push({points:clone(points),labels:[...labels],preset,comment:$('#route-comment').value,buffer});if(history.length>100)history.shift();}
 function refresh(rebuild=true){
  selected=Math.max(0,Math.min(selected,points.length-1));if(rebuild)$('#route-point').replaceChildren(...points.map((_,i)=>{const o=document.createElement('option');o.value=String(i);o.textContent=(i+1)+'. '+(labels[i]||'Điểm tuyến');return o;}));
  $('#route-point').value=String(selected);for(const [i,axis]of ['x','y','z'].entries())if(document.activeElement!==$('#route-'+axis))$('#route-'+axis).value=points[selected][i].toFixed(3);
  const metric=routeMetrics(points),nominal=baseline.reference_span_m,clashes=routeObstacleClashes(points,model);
  $('#route-metric-summary').textContent='PA2 · mốc S → đỉnh rào '+nominal.toFixed(2)+' m · tim uPVC '+(preset==='custom'?'nháp':'dự tính')+' ≈ '+metric.total.toFixed(2)+' m · dây mềm 1,2 m tính riêng';
  $('#pipe-purchase-summary').textContent='Tham khảo tim uPVC + dư cắt: '+metric.total.toFixed(2)+' + '+buffer.toFixed(2)+' ≈ '+(metric.total+buffer).toFixed(2)+' m. Chưa phải số cắt hoặc số mua; không gồm phần sau Y.';
  $('#pipe-buffer').value=String(buffer);
  $('#triangle-explanation').textContent='Mốc PA2 từ S: 7,40 m → hạ 40 cm → men mini 1,50 m → hạ 2,60 m đến đỉnh rào. Đầu chờ dưới đỉnh rào và vị trí van chỉ minh họa.';
  $('#route-site-warning').textContent=(clashes.length?'Nháp giao tường/ống sẵn tại '+clashes.map(h=>'đoạn '+(h.segment+1)+' / '+h.wall).join(', ')+'. ':'')+(comparison?model.route_presets[comparison].note+' Editor vẫn sửa uPVC đợt một.':'Cần đo lại vị trí gá, khoảng tháo van và cao độ đầu chờ tại chỗ.');
  $$('[data-route-preset]').forEach(b=>b.setAttribute('aria-pressed',b.dataset.routePreset===(comparison||'main')));
  $('#route-remove').disabled=points.length<=2;$('#route-undo').disabled=!history.length;
 }
 function apply(next,{fromScene=false}={}){if(!validPoints(next))return false;points=clone(next);model.water_route.points=clone(points);model.water_route.point_labels=[...labels];model.water_route.preset=preset;if(!fromScene)getScene()?.setRoute(points,labels);refresh();redraw();return true;}
 function isCurrent(payload){return payload?.model_version===model.model_version&&payload.baseline_revision===model.baseline_revision&&payload.installation_revision===model.installation_revision&&payload.source_option===model.source_option;}
 function restore(payload){if(!isCurrent(payload)||!validPoints(payload?.route?.points))return false;labels=Array.isArray(payload.route.labels)&&payload.route.labels.length===payload.route.points.length?payload.route.labels.map(String):payload.route.points.map((_,i)=>'Điểm '+(i+1));preset=payload.route.preset==='main'?'main':'custom';buffer=Number.isFinite(payload.buffer_m)&&payload.buffer_m>=0?payload.buffer_m:model.pipe_allowance.buffer_m;$('#route-comment').value=payload.comment||'';restoreNotes?.(payload.notes);return apply(payload.route.points);}
 $$('[data-route-preset]').forEach(b=>b.addEventListener('click',()=>{const key=b.dataset.routePreset;comparison=key==='main'||key===comparison?null:key;model.comparison_route=comparison;getScene()?.setComparison(comparison);refresh(false);redraw();}));
 function setEditing(value){editing=value;$('#route-edit-toggle').setAttribute('aria-pressed',editing);$('#route-edit-toggle').textContent=editing?'Đang sửa tuyến chính':'Sửa điểm trên 3D';if(editing)$('#route-editor').open=true;getScene()?.setEditMode(editing);}
 $('#route-edit-toggle').addEventListener('click',()=>setEditing(!editing));
 $('#focus-3d').addEventListener('click',()=>{const focus=$('#workspace').classList.toggle('focus-3d');$('#focus-3d').setAttribute('aria-pressed',focus);$('#focus-3d').textContent=focus?'Hiện bản vẽ / thông tin':'Xem 3D rộng';});
 $('#route-point').addEventListener('change',()=>{selected=Number($('#route-point').value);getScene()?.selectRoutePoint(selected);refresh(false);});
 $('#route-apply').addEventListener('click',()=>{const values=['x','y','z'].map(axis=>$('#route-'+axis).value.trim()),next=values.map(Number);if(values.some(v=>v==='')||!next.every(Number.isFinite)){$('#route-save-status').textContent='Cần đủ ba tọa độ dạng số.';return;}remember();const ps=clone(points);ps[selected]=next;preset='custom';apply(ps);markDirty();});
 $('#route-add').addEventListener('click',()=>{remember();const i=Math.min(selected,points.length-2),ps=clone(points);ps.splice(i+1,0,ps[i].map((v,axis)=>(v+ps[i+1][axis])/2));labels.splice(i+1,0,'Điểm bạn thêm');selected=i+1;preset='custom';apply(ps);markDirty();});
 $('#route-remove').addEventListener('click',()=>{if(points.length<=2)return;remember();const ps=clone(points);ps.splice(selected,1);labels.splice(selected,1);preset='custom';apply(ps);markDirty();});
 $('#route-undo').addEventListener('click',()=>{const prev=history.pop();if(!prev)return;({points,labels,preset,buffer}=prev);$('#route-comment').value=prev.comment;apply(points);markDirty();});
 $('#route-reset').addEventListener('click',()=>{remember();const main=baseline;labels=[...main.point_labels];preset='main';selected=0;apply(main.points);markDirty();});
 $('#pipe-buffer').addEventListener('change',()=>{const raw=$('#pipe-buffer').value.trim(),next=Number(raw);if(!raw||!Number.isFinite(next)||next<0||next>100){$('#route-save-status').textContent='Buffer cần từ0 đến100m.';$('#pipe-buffer').value=String(buffer);return;}remember();buffer=next;refresh(false);markDirty();});
 $('#route-restore-previous').addEventListener('click',()=>{if(!previous)return;remember();restoreNotes?.(previous.notes);$('#route-comment').value=previous.comment||'';markDirty();$('#route-save-status').textContent='Đã nạp nội dung ghi chú cũ. Tọa độ cũ vẫn giữ riêng, không áp lên thiết kế đợt một.';});
 $('#route-comment').addEventListener('input',markDirty);
 $('#route-export').addEventListener('click',()=>{const url=URL.createObjectURL(new Blob([JSON.stringify(pack(),null,2)],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download='migarden-route-feedback-'+model.model_version+'.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1500);});
 $('#route-save').addEventListener('click',async()=>{const sentGeneration=editGeneration,payload=pack();$('#route-save').disabled=true;$('#route-save-status').textContent='Đang lưu nháp cho Agent…';try{const r=await fetch('/api/feedback',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)}),result=await r.json();if(!r.ok)throw Error(result.error||'Chưa lưu được.');revision=result.revision;dirty=editGeneration!==sentGeneration;cache();$('#route-save-status').textContent=dirty?'Đã lưu bản '+revision+'; có chỉnh sửa mới hơn chưa lưu. Bấm Lưu cho Agent lần nữa.':'Đã lưu cho Agent · bản '+revision+'. Baseline nguồn vẫn nguyên.';}catch(e){$('#route-save-status').textContent=e.message+' Nháp vẫn giữ; có thể xuất dự phòng.';}finally{$('#route-save').disabled=false;}});
 $('#route-comparison').textContent='Cam liền: tuyến đợt một · xám đứt: sau Y và phần nới · xanh xám: tuyến bản cũ khi bật so sánh.';
 function keepPrevious(draft){if(!previous&&!isCurrent(draft)&&validPoints(draft?.route?.points)){previous=draft;$('#previous-draft').hidden=false;}}
 $('#previous-draft').hidden=true;
 for(const suffix of ['07-r2','07','06','05','04','03'])try{keepPrevious(JSON.parse(localStorage.getItem('migarden-route-draft-'+suffix)||'null'));}catch{}
 refresh();let cached;try{cached=JSON.parse(localStorage.getItem(cacheKey)||'null');}catch{}
 const restored=restore(cached);if(restored){dirty=true;$('#route-save-status').textContent='Đã khôi phục nháp đợt một từ trình duyệt.';}
 fetch('/api/feedback').then(r=>{if(!r.ok)throw Error();return r.json();}).then(saved=>{
  revision=restored&&Number.isInteger(cached.expectedRevision)?cached.expectedRevision:(saved.revision||0);keepPrevious(saved.previous_draft);
  if(!restored&&!dirty&&restore(saved.draft))$('#route-save-status').textContent='Đang xem bản đã lưu cho Agent · '+revision+'.';
  else if(previous&&!restored&&!dirty)$('#route-save-status').textContent='Đang xem thiết kế đợt một; nháp tọa độ bản cũ giữ riêng.';
 }).catch(()=>{$('#route-save-status').textContent='Chưa đọc được bản lưu project; vẫn có thể chỉnh và xuất dự phòng.';});
 return {stopEditing:()=>setEditing(false),notesChanged:markDirty,beginDrag:remember,selectPoint:i=>{selected=i;refresh(false);},changedFromScene:(next,i)=>{if(!validPoints(next))return;preset='custom';selected=i;apply(next,{fromScene:true});markDirty();},diagnostics:()=>({revision,preset,dirty,comparison,buffer_m:buffer,points:clone(points)})};
}
