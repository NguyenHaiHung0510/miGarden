import { createScene } from './scene.js';
import { drawSVG } from './drawing.js';
import { initRouteEditor } from './route-editor.js';

const $ = selector => document.querySelector(selector);
const $$ = selector => [...document.querySelectorAll(selector)];
const model = await fetch('/model.json').then(r => { if (!r.ok) throw new Error('Không đọc được model'); return r.json(); });
const availableSources=new Set(await fetch('/api/sources').then(r=>r.ok?r.json():{available:[]}).then(value=>value.available||[]).catch(()=>[]));
const state = { layout: new URLSearchParams(location.search).get('layout') === 'drawing' ? 'drawing' : 'space', projection: 'section', selected: 'F', scenario: 'original', layers: { water: true, pots: true, room: true, dimensions: true }, pots: structuredClone(model.pot_samples) };
const statusNames = { 'owner-confirmed': 'Bạn đã xác nhận', observed: 'Có trong ảnh · vị trí vẽ minh họa', inferred: 'Cần đối chiếu vị trí', estimated: 'Ước lượng · chưa đo', proposed: 'Đề xuất · chưa lắp', sample: 'Chậu mẫu · không phải inventory thật' };
let scene = null, routeEditor = null, notes = {};
try { notes = JSON.parse(localStorage.getItem('migarden-model-v0-notes') || '{}'); if (!notes || typeof notes !== 'object' || Array.isArray(notes)) notes = {}; } catch { notes = {}; }
function entity(id) {
  const base = model.objects.find(o => o.id === id);
  if (base) return base;
  const pot = state.pots.find(p => 'pot:' + p.id === id);
  if (!pot) return model.objects[1];
  return { id, label: 'Chậu mẫu ' + (state.pots.indexOf(pot) + 1), status: 'sample', summary: 'Chậu và nhánh mềm ở đây chỉ để bạn chỉ vị trí. Chưa chốt việc kiểm soát hoặc ghi nhận từng đầu vòi. Đổi bố trí không tạo thiết bị, telemetry hay trạng thái van thật.', sources: ['A07', 'A08'] };
}
function renderDrawing() {
  $('#drawing').innerHTML = drawSVG(model, state);
  $$('#drawing [data-id]').forEach(node => {
    node.addEventListener('click', () => select(node.dataset.id));
    node.addEventListener('keydown', event => { if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); select(node.dataset.id); } });
  });
}
function openSource(id) {
  const source = model.sources[id]; if (!source||!availableSources.has(id)) return;
  $('#photo-code').textContent = id;
  $('#photo-title').textContent = source.title;
  $('#photo-image').src = '/source/' + encodeURIComponent(id);
  $('#photo-image').alt = id + ' · ' + source.title;
  $('#photo-note').textContent = source.note;
  $('#photo-dialog').showModal();
}
function renderInspector() {
  const object = entity(state.selected);
  $('#selected-code').textContent = object.id.startsWith('pot:') ? 'MẪU' : (object.display_id || object.id.toUpperCase());
  const heading = document.createElement('h2'); heading.textContent = object.label;
  const status = document.createElement('span'); status.className = 'evidence-status ' + object.status; status.textContent = statusNames[object.status];
  const summary = document.createElement('p'); summary.textContent = object.summary;
  $('#selection-info').replaceChildren(heading, status, summary);
  $('#source-list').replaceChildren(...object.sources.map(id => {
    const button = document.createElement('button'); button.className = 'source-thumb'; button.setAttribute('aria-label', 'Mở ảnh ' + id + ': ' + model.sources[id].title);
    const available=availableSources.has(id);
    if(available){const img=document.createElement('img');img.src='/source/'+id;img.alt=model.sources[id].title;img.loading='lazy';button.append(img);}
    const label = document.createElement('span'); label.textContent = id + (available?' · Mở ảnh ↗':' · Ảnh gốc giữ riêng'); button.append(label);button.disabled=!available;button.title=model.sources[id].title;button.addEventListener('click', () => openSource(id)); return button;
  }));
  $('#note-text').value = notes[state.selected]?.text || '';
  $('#note-status').textContent = '';
  $$('#object-list button').forEach(b => b.setAttribute('aria-pressed', b.dataset.id === state.selected));
}
function select(id) {
  if (!model.objects.some(o=>o.id===id) && !state.pots.some(p=>'pot:'+p.id===id)) return;
  state.selected = id; scene?.select(id); $('#quick-focus').value=id; renderInspector(); renderDrawing();
}
function setLayout(layout) {
  state.layout = layout; $('#workspace').classList.toggle('layout-drawing', layout === 'drawing');
  $$('[data-layout]').forEach(b => b.setAttribute('aria-pressed', b.dataset.layout === layout));
  const url = new URL(location.href); url.searchParams.set('layout', layout); history.replaceState(null, '', url);
}
function scenario(name) {
  state.scenario = name; state.pots = structuredClone(model.pot_samples);
  if (name === 'move') {
    for (const index of [1, 4, 7]) { state.pots[index].z = 6.8; state.pots[index].x += .1; }
  }
  if (name === 'add') state.pots.push({ id: 'sample-extra-01', x: 1.3, z: 6.8, type: 'round' }, { id: 'sample-extra-02', x: 3.6, z: 6.85, type: 'box' });
  if (state.selected.startsWith('pot:') && !state.pots.some(p=>'pot:'+p.id===state.selected)) state.selected = 'balcony';
  scene?.setPots(state.pots); scene?.select(state.selected);
  $('#pot-count').textContent = state.pots.length + ' chậu · vị trí minh họa';
  $$('[data-scenario]').forEach(b => b.setAttribute('aria-pressed', b.dataset.scenario === name));
  renderInspector(); renderDrawing();
}
function saveFile(contents, filename, type) {
  const url = URL.createObjectURL(new Blob([contents], { type }));
  const link = document.createElement('a'); link.href = url; link.download = filename; document.body.append(link); link.click(); link.remove(); setTimeout(() => URL.revokeObjectURL(url), 1500);
}
$('#model-version').textContent = model.model_version;
function quickFocus(id){select(id);scene?.focus(id);}
$('#object-list').replaceChildren(...model.navigation_groups.map(group=>{
  const section=document.createElement('div');section.className='object-group';const title=document.createElement('h4');title.textContent=group.label;section.append(title);
  const options=document.createElement('optgroup');options.label=group.label;
  for(const id of group.ids){const object=model.objects.find(o=>o.id===id);const button=document.createElement('button');button.dataset.id=id;button.textContent=object.short;button.setAttribute('aria-pressed',id===state.selected);button.addEventListener('click',()=>quickFocus(id));section.append(button);const option=document.createElement('option');option.value=id;option.textContent=object.short;options.append(option);}
  $('#quick-focus').append(options);return section;
}));
$('#quick-focus').value=state.selected;$('#quick-focus').addEventListener('change',event=>quickFocus(event.target.value));
$('#observation-list').replaceChildren(...model.observations.map(observation => {
  const row = document.createElement('div'); row.className = 'observation-row';
  const value = document.createElement('b'); value.textContent = observation.reading;
  const note = document.createElement('p'); note.textContent = observation.note;
  const source = document.createElement('button'); source.textContent = observation.source; source.setAttribute('aria-label', 'Xem nguồn ' + observation.source + ' của số đọc ' + observation.reading); source.addEventListener('click', () => openSource(observation.source));
  row.append(value, note, source); return row;
}));
$('#estimate-list').replaceChildren(...(model.agent_estimates||[]).map(estimate=>{
  const row=document.createElement('div');row.className='estimate-row';
  const title=document.createElement('strong');title.textContent=estimate.item+' · '+estimate.value;
  const note=document.createElement('p');note.textContent=estimate.basis;row.append(title,note);return row;
}));
$$('[data-layout]').forEach(b => b.addEventListener('click', () => setLayout(b.dataset.layout)));
$$('[data-projection]').forEach(b => b.addEventListener('click', () => {
  state.projection = b.dataset.projection; $$('[data-projection]').forEach(other => other.setAttribute('aria-pressed', other === b)); renderDrawing();
}));
$$('[data-camera]').forEach(b => b.addEventListener('click', () => {
  scene?.setCamera(b.dataset.camera); $$('[data-camera]').forEach(other => other.classList.toggle('active', other === b));
}));
for (const key of ['water', 'pots', 'room', 'dimensions']) $('#layer-' + key).addEventListener('change', event => { state.layers[key] = event.target.checked; scene?.setLayer(key, state.layers[key]); renderDrawing(); });
$('#cutaway').addEventListener('change', event => scene?.cutaway(event.target.checked));
$$('[data-scenario]').forEach(b => b.addEventListener('click', () => scenario(b.dataset.scenario)));
$('#close-photo').addEventListener('click', () => $('#photo-dialog').close());
$('#photo-dialog').addEventListener('click', event => { if (event.target === $('#photo-dialog')) { const rect=event.target.getBoundingClientRect(); if (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom) event.target.close(); } });
$('#note-toggle').addEventListener('click', () => { $('#note-editor').hidden = !$('#note-editor').hidden; if (!$('#note-editor').hidden) $('#note-text').focus(); });
$('#save-note').addEventListener('click', () => {
  const text = $('#note-text').value.trim();
  if (text) notes[state.selected] = { text, object_id: state.selected, model_version: model.model_version, recorded_at: new Date().toISOString(), status: 'owner-draft-note' };
  else delete notes[state.selected];
  try { localStorage.setItem('migarden-model-v0-notes', JSON.stringify(notes)); $('#note-status').textContent = 'Đã lưu cục bộ. Mô hình chưa tự thay đổi.'; } catch { $('#note-status').textContent = 'Chưa lưu được vào trình duyệt. Bạn có thể xuất ghi chú ra file.'; }
  $('#note-count').textContent = Object.keys(notes).length;
  routeEditor?.notesChanged();
});
$('#export-notes').addEventListener('click', () => saveFile(JSON.stringify({ model_version: model.model_version, type: 'comparison-notes-not-applied', notes: Object.values(notes) }, null, 2), 'migarden-ghi-chu-doi-chieu.json', 'application/json'));
$('#export-svg').addEventListener('click', () => saveFile(drawSVG(model, state), 'migarden-model-v0-' + state.projection + '.svg', 'image/svg+xml'));
$('#note-count').textContent = Object.keys(notes).length;
$('#fallback-drawing').addEventListener('click', () => setLayout('drawing'));
function navigationChanged(mode){$$('[data-navigation]').forEach(b=>b.setAttribute('aria-pressed',b.dataset.navigation===mode));$('#navigation-help').textContent=mode==='fly'?'WASD di chuyển · Q/E xuống/lên · ←/→ quay360° · ↑/↓ ngẩng/cúi · chuột phải nhìn · Shift nhanh · Esc về Xoay':'Kéo xoay · chuột phải kéo để dịch · cuộn thu/phóng';}
try { scene = createScene($('#scene'), $('#scene-labels'), model, select,{onRouteChange:(points,index)=>routeEditor?.changedFromScene(points,index),onRoutePointSelect:index=>routeEditor?.selectPoint(index),onRouteDragStart:()=>routeEditor?.beginDrag(),onNavigationChange:navigationChanged}); }
catch (error) { $('#renderer-error').hidden = false; console.warn('3D unavailable; SVG stays usable:', error.message); }
setLayout(state.layout); renderInspector(); renderDrawing();
routeEditor=initRouteEditor({model,getScene:()=>scene,redraw:renderDrawing,getNotes:()=>notes,restoreNotes:previous=>{
  if(!previous||typeof previous!=='object'||Array.isArray(previous))return;
  notes={...previous,...notes};
  try{localStorage.setItem('migarden-model-v0-notes',JSON.stringify(notes));}catch{}
  $('#note-count').textContent=Object.keys(notes).length;renderInspector();
}});
// Read-only diagnostics for local smoke evidence; no physical/device state is represented.
$$('[data-navigation]').forEach(button=>button.addEventListener('click',()=>{if(button.dataset.navigation==='fly')routeEditor?.stopEditing();scene?.setNavigation(button.dataset.navigation);$('#scene').focus({preventScroll:true});}));
try{$('#highlight-selection').checked=localStorage.getItem('migarden-highlight')!=='off';}catch{}
scene?.setHighlight($('#highlight-selection').checked);
$('#highlight-selection').addEventListener('change',event=>{scene?.setHighlight(event.target.checked);try{localStorage.setItem('migarden-highlight',event.target.checked?'on':'off');}catch{}});
const panel=$('#three-panel');
function fullscreenChanged(){const active=document.fullscreenElement===panel||panel.classList.contains('is-expanded');$('#scene-fullscreen').setAttribute('aria-pressed',active);$('#scene-fullscreen').textContent=active?'Thoát toàn màn hình':'Toàn màn hình';}
$('#scene-fullscreen').addEventListener('click',async()=>{
  if(document.fullscreenElement===panel){await document.exitFullscreen();return;}
  if(panel.classList.contains('is-expanded')){panel.classList.remove('is-expanded');$('#fullscreen-status').textContent='';fullscreenChanged();return;}
  try{if(!panel.requestFullscreen)throw Error('unavailable');await panel.requestFullscreen();$('#fullscreen-status').textContent='';}
  catch{panel.classList.add('is-expanded');$('#fullscreen-status').textContent='Trình duyệt không cấp fullscreen; đang mở rộng trong cửa sổ này.';}
  fullscreenChanged();
});
document.addEventListener('fullscreenchange',fullscreenChanged);
document.addEventListener('keydown',event=>{if(event.key==='Escape'&&panel.classList.contains('is-expanded')){panel.classList.remove('is-expanded');$('#fullscreen-status').textContent='';fullscreenChanged();}});
window.miGardenPrototype = { diagnostics: () => ({ model_version: model.model_version, coordinate_kind: model.coordinate_frame.kind, layout: state.layout, projection: state.projection, selection: state.selected, source_option:model.source_option, potCount: state.pots.length, layers: {...state.layers}, scene: scene?.diagnostics() || null, editor:routeEditor?.diagnostics(),fullscreen:document.fullscreenElement===panel,expanded:panel.classList.contains('is-expanded') }) };
window.addEventListener('pagehide', () => scene?.dispose(), { once: true });
