import {routeMetrics,validPoints} from './route-math.js';
const $=s=>document.querySelector(s), $$=s=>[...document.querySelectorAll(s)];
const clone=v=>structuredClone(v);
export function initRouteEditor({model,getScene,redraw,getNotes,restoreNotes}){
  const scale=model.coordinate_frame.estimated_meters_per_scene_unit;
  let points=clone(model.water_route.points),labels=[...model.water_route.point_labels],preset=model.default_route_preset,sourceOption=model.source_option,selected=0,revision=0,editing=false,dirty=false,previous=null,migratedFrom=null;
  const history=[],cacheKey='migarden-route-draft-05';
  function pack(){return {model_version:model.model_version,expectedRevision:revision,route:{points:clone(points),labels:[...labels],preset},source_option:sourceOption,comment:$('#route-comment').value,notes:clone(getNotes()),synthetic:true,migrated_from:migratedFrom,client_saved_at:new Date().toISOString()};}
  function cache(){try{localStorage.setItem(cacheKey,JSON.stringify(pack()));}catch{}}
  function markDirty(){dirty=true;$('#route-save-status').textContent='Có thay đổi chưa lưu cho Agent. Nháp đang được giữ trên trình duyệt.';cache();}
  function remember(){history.push({points:clone(points),labels:[...labels],preset,sourceOption,comment:$('#route-comment').value,migratedFrom});if(history.length>100)history.shift();}
  function refresh(selectOptions=true){
    selected=Math.max(0,Math.min(selected,points.length-1));
    if(selectOptions)$('#route-point').replaceChildren(...points.map((p,i)=>{const o=document.createElement('option');o.value=String(i);o.textContent=`${i+1}. ${labels[i]||'Điểm tuyến'}`;return o;}));
    $('#route-point').value=String(selected);
    for(const [i,axis] of ['x','y','z'].entries())if(document.activeElement!==$('#route-'+axis))$('#route-'+axis).value=(points[selected][i]*scale[i]).toFixed(2);
    const metrics=routeMetrics(points,scale);
    const peak=Math.max(...points.map(p=>p[1])),height=(peak-model.layout.terrace.floor)*scale[1];
    $('#route-metric-summary').textContent=`Tuyến đang vẽ ≈${metrics.total.toFixed(2)} m · đỉnh ≈${height.toFixed(2)} m trên sàn thượng · ${points.length} điểm · chưa cộng dư thi công`;
    const t=model.triangle_assumption;
    $('#triangle-explanation').textContent=preset.startsWith('right')?'Bồn–vòi vuông góc tường phải. Sang phải theo một đường ngang hoặc hạ thấp nhưng vẫn trên ống sẵn, nâng trước lan can nhà mình rồi men dọc như cũ. Số và khoảng hở là ước lượng; không gán nhãn 7,79 m cũ sang tuyến phải.':`Giả định cũ phía trái: chéo sàn ${t.diagonal.toFixed(2)} m; hai cạnh ≈${t.transverse.toFixed(2)} / ${t.longitudinal.toFixed(2)} m, góc ≈${t.angleDegrees.toFixed(1)}°. Đây là synthetic; bạn có thể sửa điểm.`;
    $('#route-site-warning').textContent=(model.route_presets[preset]?.note||'Tuyến bạn đang chỉnh; kiểm tra vị trí đỉnh, vật cản và các đoạn cần đỡ trên hình.')+' ◇ đánh dấu nơi cần gá/đỡ, chưa chốt cách lắp. Hai phương án chưa là lựa chọn thi công.';
    $$('[data-route-preset]').forEach(b=>b.setAttribute('aria-pressed',b.dataset.routePreset===preset));
    $$('#route-comparison [data-compare]').forEach(card=>{
      const key=card.dataset.compare==='right'&&preset==='right-low'?'right-low':card.dataset.compare,p=model.route_presets[key];
      card.classList.toggle('active',key===preset);card.querySelector('strong').textContent=p.label;
      card.querySelector('span').textContent=`Ước lượng gốc ≈${routeMetrics(p.points,scale).total.toFixed(2)} m · đỉnh ≈${((Math.max(...p.points.map(v=>v[1]))-model.layout.terrace.floor)*scale[1]).toFixed(2)} m`;
    });
    $$('[data-source-option]').forEach(button=>button.setAttribute('aria-pressed',button.dataset.sourceOption===sourceOption));
    $('#source-option-note').textContent=model.source_options[sourceOption].note;
    $('#route-remove').disabled=points.length<=2;$('#route-undo').disabled=!history.length;
  }
  function apply(next,{fromScene=false,keepSelection=false}={}){
    if(!validPoints(next))return false;points=clone(next);model.water_route.points=clone(points);model.water_route.point_labels=[...labels];model.water_route.preset=preset;
    model.water_route.support_indices=labels.flatMap((s,i)=>/gá|neo|đỡ/.test(s)?[i]:[]);
    if(!fromScene)getScene()?.setRoute(points,labels);
    refresh(!keepSelection);redraw();return true;
  }
  function restore(payload){
    if(payload?.model_version!==model.model_version||!validPoints(payload?.route?.points))return false;
    labels=Array.isArray(payload.route.labels)&&payload.route.labels.length===payload.route.points.length?payload.route.labels.map(String):payload.route.points.map((_,i)=>'Điểm '+(i+1));
    preset=payload.route.preset||'custom';sourceOption=model.source_options[payload.source_option]?payload.source_option:'reuse';model.source_option=sourceOption;getScene()?.setSourceOption(sourceOption);
    migratedFrom=payload.migrated_from||null;$('#route-comment').value=payload.comment||'';return apply(payload.route.points);
  }
  $$('[data-route-preset]').forEach(button=>button.addEventListener('click',()=>{
    remember();const p=model.route_presets[button.dataset.routePreset];labels=[...p.point_labels];preset=button.dataset.routePreset;selected=0;apply(p.points);markDirty();
  }));
  $$('[data-source-option]').forEach(button=>button.addEventListener('click',()=>{
    remember();sourceOption=button.dataset.sourceOption;model.source_option=sourceOption;getScene()?.setSourceOption(sourceOption);refresh(false);redraw();markDirty();
  }));
  $('#route-edit-toggle').addEventListener('click',()=>{
    editing=!editing;$('#route-edit-toggle').setAttribute('aria-pressed',editing);$('#route-editor').open=editing;
    $('#route-edit-toggle').textContent=editing?'Đang sửa · kéo chấm/trục':'Sửa điểm trên 3D';getScene()?.setEditMode(editing);
  });
  $('#focus-3d').addEventListener('click',()=>{const focus=$('#workspace').classList.toggle('focus-3d');$('#focus-3d').setAttribute('aria-pressed',focus);$('#focus-3d').textContent=focus?'Hiện bản vẽ / thông tin':'Xem 3D rộng';});
  $('#route-point').addEventListener('change',()=>{selected=Number($('#route-point').value);getScene()?.selectRoutePoint(selected);refresh(false);});
  $('#route-apply').addEventListener('click',()=>{
    const next=['x','y','z'].map((axis,i)=>Number($('#route-'+axis).value)/scale[i]);
    if(!next.every(Number.isFinite)){$('#route-save-status').textContent='Cần ba tọa độ dạng số.';return;}
    remember();const changed=clone(points);changed[selected]=next;preset='custom';apply(changed);markDirty();
  });
  $('#route-add').addEventListener('click',()=>{remember();const at=Math.min(selected,points.length-2),next=clone(points);next.splice(at+1,0,next[at].map((v,i)=>(v+next[at+1][i])/2));labels.splice(at+1,0,'Điểm bạn thêm');selected=at+1;preset='custom';apply(next);markDirty();});
  $('#route-remove').addEventListener('click',()=>{if(points.length<=2)return;remember();const next=clone(points);next.splice(selected,1);labels.splice(selected,1);preset='custom';apply(next);markDirty();});
  $('#route-undo').addEventListener('click',()=>{const prev=history.pop();if(!prev)return;labels=prev.labels;preset=prev.preset;sourceOption=prev.sourceOption;model.source_option=sourceOption;getScene()?.setSourceOption(sourceOption);migratedFrom=prev.migratedFrom;$('#route-comment').value=prev.comment;apply(prev.points);markDirty();});
  $('#route-restore-previous').addEventListener('click',()=>{
    if(!previous)return;remember();
    restore({...clone(previous),model_version:model.model_version,migrated_from:{model_version:previous.model_version,source:'explicit-owner-restore',at:new Date().toISOString()}});
    restoreNotes?.(previous.notes);markDirty();
    $('#route-save-status').textContent='Đã nạp tuyến/feedback cũ để đối chiếu trên bản 05. Nháp gốc vẫn giữ riêng; chưa ghi vào project.';
  });
  $('#route-comment').addEventListener('input',markDirty);
  $('#route-export').addEventListener('click',()=>{
    const url=URL.createObjectURL(new Blob([JSON.stringify(pack(),null,2)],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download='migarden-route-feedback.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1500);
  });
  $('#route-save').addEventListener('click',async()=>{
    $('#route-save').disabled=true;$('#route-save-status').textContent='Đang lưu vào project local…';
    try{
      const response=await fetch('/api/feedback',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(pack())});const result=await response.json();
      if(!response.ok)throw new Error(result.error||'Chưa lưu được.');revision=result.revision;dirty=false;cache();
      $('#route-save-status').textContent=`Đã lưu cho Agent · bản ${revision}. Bạn có thể tiếp tục chỉnh rồi lưu tiếp.`;
    }catch(error){$('#route-save-status').textContent=error.message+' Nháp trên màn hình vẫn giữ; có thể xuất dự phòng.';}
    finally{$('#route-save').disabled=false;}
  });
  $('#route-comparison').replaceChildren(...['folded','right'].map(key=>{
    const p=model.route_presets[key],card=document.createElement('div');card.dataset.compare=key;
    const title=document.createElement('strong');title.textContent=p.label;
    const metric=document.createElement('span');metric.textContent=`Ước lượng gốc ≈${routeMetrics(p.points,scale).total.toFixed(2)} m · đỉnh ≈${((Math.max(...p.points.map(v=>v[1]))-model.layout.terrace.floor)*scale[1]).toFixed(2)} m`;
    card.append(title,metric);return card;
  }));
  for(const version of ['04','03'])try{const old=JSON.parse(localStorage.getItem('migarden-route-draft-'+version)||'null');if(!previous&&old?.model_version==='model-v0-draft-'+version&&validPoints(old?.route?.points))previous=old;}catch{}
  $('#previous-draft').hidden=!previous;
  refresh();
  let cached;try{cached=JSON.parse(localStorage.getItem(cacheKey)||'null');}catch{}
  const restoredCache=cached&&restore(cached);
  if(restoredCache){dirty=true;$('#route-save-status').textContent='Đã khôi phục nháp bản 05 từ trình duyệt. Bấm Lưu cho Agent để ghi vào project.';}
  fetch('/api/feedback').then(r=>r.json()).then(saved=>{
    revision=saved.revision||0;
    if(!restoredCache&&!dirty&&saved.draft&&restore(saved.draft))$('#route-save-status').textContent=`Đang xem bản đã lưu cho Agent · ${revision}.`;
    if(['model-v0-draft-03','model-v0-draft-04'].includes(saved.draft?.model_version)&&validPoints(saved.draft?.route?.points)){
      previous=previous||saved.draft;$('#previous-draft').hidden=false;
      if(!dirty&&!restoredCache)$('#route-save-status').textContent='Đang xem phương án bản 05. Bản đã lưu cũ vẫn nguyên; dùng nút nạp nháp cũ nếu cần.';
    }
    cache();
  }).catch(()=>{$('#route-save-status').textContent='Chưa đọc được bản lưu project; vẫn có thể chỉnh và xuất dự phòng.';});
  return {
    beginDrag(){remember();},
    selectPoint(index){selected=index;refresh(false);},
    changedFromScene(next,index){if(!validPoints(next))return;preset='custom';selected=index;apply(next,{fromScene:true,keepSelection:true});markDirty();}
  };
}
