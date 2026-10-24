import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { TransformControls } from 'three/addons/controls/TransformControls.js';

export function createScene(host, labelHost, model, onSelect, editHooks = {}) {
  const scene = new THREE.Scene();
  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  renderer.setClearColor(0xeeeeE2, 0);
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.35;
  host.append(renderer.domElement);
  const camera = new THREE.OrthographicCamera(-8, 8, 6, -6, .1, 100);
  const controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = .12;
  controls.minZoom = .45;
  controls.maxZoom = 5;
  controls.maxPolarAngle = Math.PI * .49;
  controls.minPolarAngle = .1;
  controls.enablePan = true;
  const groups = {};
  for (const key of ['building', 'room', 'water', 'pots', 'closed', 'upper', 'dimensions', 'edit', 'routeHints']) {
    groups[key] = new THREE.Group(); scene.add(groups[key]);
  }
  let selected = 'F', viewHeight = 12, pots = model.pot_samples, ready = false, currentCamera = 'all';
  const pickables = [], labels = new Map();
  const P = { cream: 0xdedecb, warm: 0xf2ead9, floor: 0xd9d3bc, trim: 0x9b7155, glass: 0x57867f, metal: 0xa2a99d, rail: 0x566858, soil: 0x554b33, green: 0x4c7350, orange: 0xca7f40 };
  function mesh(group, geometry, color, pos, id, extra = {}) {
    const material = new THREE.MeshStandardMaterial({ color, roughness: .83, metalness: 0, ...extra });
    const result = new THREE.Mesh(geometry, material);
    result.position.set(...pos); result.castShadow = true; result.receiveShadow = true;
    group.add(result);
    if (id) { result.userData.pickId = id; pickables.push(result); }
    return result;
  }
  function box(group, size, color, pos, id, extra) { return mesh(group, new THREE.BoxGeometry(...size), color, pos, id, extra); }
  function tube(group, points, radius, color, id) {
    const curve = new THREE.CurvePath();
    for (let i = 1; i < points.length; i++) curve.add(new THREE.LineCurve3(new THREE.Vector3(...points[i - 1]), new THREE.Vector3(...points[i])));
    return mesh(group, new THREE.TubeGeometry(curve, Math.max(12, points.length * 10), radius, 7, false), color, [0, 0, 0], id);
  }
  function line(group, points, color, dashed = false) {
    const material = dashed ? new THREE.LineDashedMaterial({ color, dashSize: .15, gapSize: .12 }) : new THREE.LineBasicMaterial({ color, transparent: true, opacity: .65 });
    const result = new THREE.Line(new THREE.BufferGeometry().setFromPoints(points.map(p => new THREE.Vector3(...p))), material);
    result.computeLineDistances(); group.add(result); return result;
  }
  const { balcony: b, terrace: t, room: r } = model.layout;
  const center = bounds => [(bounds.minX + bounds.maxX) / 2, bounds.floor, (bounds.minZ + bounds.maxZ) / 2];
  const size = bounds => [bounds.maxX - bounds.minX, .12, bounds.maxZ - bounds.minZ];
  box(groups.building, size(b), P.floor, center(b), 'balcony');
  box(groups.room, size(r), 0xe2decc, center(r), 'W');
  // Sparse cutaway walls: recognition aids, not surveyed dimensions.
  box(groups.room, [6.15, 3.5, .1], P.warm, [2.725, 1.75, .2], 'W');
  // Solid left wall with a real window opening, to the left of the main door.
  box(groups.room, [3.55, 1.05, .12], 0xeee9d8, [1.425, .525, 3.02], 'window-left');
  box(groups.room, [3.55, 1.15, .12], 0xeee9d8, [1.425, 3.125, 3.02], 'window-left');
  box(groups.room, [1.15, 1.5, .12], 0xeee9d8, [.225, 1.8, 3.02], 'window-left');
  box(groups.room, [.95, 1.5, .12], 0xeee9d8, [2.725, 1.8, 3.02], 'window-left');
  box(groups.room, [.45, 3.7, .12], 0xeee9d8, [5.575, 1.85, 3.02], 'W');
  box(groups.room, [2.15, 1.2, .12], 0xe7e2d0, [4.275, 3.1, 3.02], 'W');
  const door = model.objects.find(o => o.id === 'W').pose;
  box(groups.room, [2.15, 2.45, .055], 0x53615b, door, 'W', { roughness: .7 });
  for (const dx of [-1.08, 0, 1.08]) box(groups.room, [.045, 2.5, .1], 0x34483d, [door[0] + dx, door[1], door[2] + .05], 'W');
  for (const dy of [-1.22, 1.22]) box(groups.room, [2.2, .045, .1], 0x34483d, [door[0], door[1] + dy, door[2] + .05], 'W');
  for (const dx of [-.5,.5]) box(groups.room,[.75,1.9,.025],0x657069,[door[0]+dx,door[1],door[2]+.05],'W');
  box(groups.room,[.025,.17,.04],0xc1c4b3,[door[0]+.13,1.05,door[2]+.1],'W');
  const win = model.objects.find(o=>o.id==='window-left').pose;
  box(groups.room,[1.4,1.5,.06],P.glass,win,'window-left',{roughness:.5});
  for(const dx of [-.72,0,.72])box(groups.room,[.04,1.53,.1],0x42554d,[win[0]+dx,win[1],win[2]+.04],'window-left');
  for(const dy of [-.77,.77])box(groups.room,[1.48,.04,.1],0x42554d,[win[0],win[1]+dy,win[2]+.04],'window-left');
  const wall=model.layout.leftWall;
  box(groups.building,[.12,wall.height,wall.maxZ-wall.minZ],0xe8e3d2,[wall.x,wall.height/2,(wall.minZ+wall.maxZ)/2],'left-wall');
  const rightWall=model.layout.rightWall;
  box(groups.building,[rightWall.thickness,rightWall.maxY-rightWall.minY,rightWall.maxZ-rightWall.minZ],0xd5d0bb,[rightWall.x,(rightWall.maxY+rightWall.minY)/2,(rightWall.minZ+rightWall.maxZ)/2],'right-wall');
  // Balcony parapet/rail and paving, inspired by A05/A07/A08.
  const baseHeight=model.layout.railBaseHeight;
  box(groups.building, [6.15, baseHeight, .13], 0xe6e1d1, [2.725, baseHeight/2, 4.85], 'rail');
  box(groups.building, [6.22, .025, .17], P.trim, [2.725, baseHeight+.0125, 4.85], 'rail');
  for (let x = 0; x <= 5.7; x += .48) box(groups.building, [.025, 1.1, .025], P.rail, [x, .65, 4.85], 'rail');
  box(groups.building, [6.1, .045, .045], P.rail, [2.72, 1.2, 4.85], 'rail');
  for (let x = -.2; x < 5.8; x += .45) line(groups.building, [[x, .065, 3.05], [x, .065, 4.75]], 0xbebda9);
  for (let z = 3.1; z < 4.8; z += .43) line(groups.building, [[-.3, .065, z], [5.75, .065, z]], 0xbebda9);
  const drain = mesh(groups.building, new THREE.CylinderGeometry(.1, .1, .012, 16), 0x818a7c, [4.1, .08, 3.5], 'balcony');
  for (let i = -2; i <= 2; i++) line(groups.building, [[4.03, .091, 3.5 + i * .025], [4.17, .091, 3.5 + i * .025]], 0xd9ddce);
  // Upper terrace outline remains visible in cutaway; full slab/roof are optional.
  const terraceCorners = [[t.minX, t.floor, t.minZ], [t.maxX, t.floor, t.minZ], [t.maxX, t.floor, t.maxZ], [t.minX, t.floor, t.maxZ], [t.minX, t.floor, t.minZ]];
  line(groups.upper, terraceCorners, 0xaab59a, true);
  box(groups.upper, [6.15, .58, .12], P.cream, [2.725, 4.69, -4.3], 'terrace');
  box(groups.upper, [.12, .58, 9.15], P.cream, [-.35, 4.69, .275], 'terrace');
  // Show the real functional route surface; cutaway removes roofs, not this floor.
  box(groups.upper,[6.15,.12,7.15],0xd7d1ba,[2.725,4.34,-.725],'terrace');
  for(let z=-4.1;z<2.8;z+=.75)line(groups.upper,[[-.3,4.405,z],[5.75,4.405,z]],0xc2bda7);
  for(let x=-.2;x<5.8;x+=.75)line(groups.upper,[[x,4.405,-4.2],[x,4.405,2.78]],0xc2bda7);
  const parapet=model.layout.parapet;
  box(groups.upper,[6.15,parapet.height_scene,.14],0xe6dfc9,[2.725,4.4+parapet.height_scene/2,parapet.z],'left-wall');
  const zb=model.layout.zoneB;
  box(groups.upper,size(zb),0xe4dcc6,center(zb),'zoneB');
  box(groups.upper,[zb.maxX-zb.minX,.48,.12],0xeee8d9,[(zb.minX+zb.maxX)/2,zb.floor+.24,zb.maxZ],'zoneB');
  box(groups.upper,[.12,.48,zb.maxZ-zb.minZ],0xeee8d9,[zb.minX,zb.floor+.24,(zb.minZ+zb.maxZ)/2],'zoneB');
  for(let x=zb.minX+.12;x<zb.maxX;x+=.36)for(let z=zb.minZ+.1;z<zb.maxZ-.08;z+=.36){
    const tile=mesh(groups.upper,new THREE.CylinderGeometry(.1,.1,.008,8),0xabb798,[x,zb.floor+.066,z],'zoneB');
  }
  mesh(groups.upper,new THREE.SphereGeometry(.17,12,8),0x6b8a55,[4.35,4.63,3.65],'zoneB');
  const ledge=model.layout.rightLedge;
  box(groups.upper,[ledge.maxX-ledge.minX,ledge.height,ledge.maxZ-ledge.minZ],0xc7c2ae,[(ledge.minX+ledge.maxX)/2,ledge.floor+ledge.height/2,(ledge.minZ+ledge.maxZ)/2],'right-ledge');
  for(const pipe of model.existing_pipes)tube(groups.upper,pipe.points,pipe.radius,0x89958e,pipe.id);
  box(groups.closed, size(t), 0xd1d2bf, center(t), 'terrace');
  box(groups.closed, [6.55, .08, 9.5], 0x9fa998, [2.7, 7.1, .2], 'terrace');
  for (const [x, z] of [[-.25, -4.1], [5.65, -4.1], [-.25, 4.6], [5.65, 4.6]]) box(groups.closed, [.06, 2.7, .06], 0x7c8979, [x, 5.75, z], 'terrace');
  for (let z = -4.2; z <= 4.7; z += .3) line(groups.closed, [[-.55, 7.15, z], [5.95, 7.15, z]], 0x899581);
  groups.closed.visible = false;
  const tankPose = model.objects.find(o => o.id === 'tank').pose;
  const tankGroup=new THREE.Group();tankGroup.position.set(...tankPose);tankGroup.rotation.y=THREE.MathUtils.degToRad(model.source_assembly.tank_yaw_degrees);groups.upper.add(tankGroup);
  const tank = mesh(tankGroup, new THREE.CylinderGeometry(.62, .62, 1.6, 44), 0xafb2a0, [0,0,0], 'tank', { roughness: .4, metalness: .45 });
  tank.rotation.x = Math.PI / 2;
  for (const dz of [-.6, -.35, .35, .6]) {
    const hoop = mesh(tankGroup, new THREE.TorusGeometry(.63, .018, 6, 44), 0x8c9685, [0,0,dz], 'tank', { metalness: .45 });
    hoop.rotation.x = 0;
  }
  for (const dx of [-.45, .45]) box(tankGroup, [.05, .2, 1.15], P.rail, [dx,4.5-tankPose[1],0], 'tank');
  const pumpPose = model.objects.find(o => o.id === 'pump').pose;
  const pumpGroup=new THREE.Group();pumpGroup.position.set(...pumpPose);pumpGroup.rotation.y=tankGroup.rotation.y;groups.upper.add(pumpGroup);
  box(pumpGroup, [.5, .34, .4], 0x3a6653, [0,0,0], 'pump');
  const motor = mesh(pumpGroup, new THREE.CylinderGeometry(.15, .15, .45, 16), 0x547665, [.1,.04,-.17], 'pump');
  motor.rotation.x = Math.PI / 2;
  const sourceGroup=new THREE.Group();groups.upper.add(sourceGroup);
  function rebuildSource(option){
    if(!model.source_options[option])return;
    model.source_option=option;
    const obsolete=new Set(sourceGroup.children);for(let i=pickables.length-1;i>=0;i--)if(obsolete.has(pickables[i]))pickables.splice(i,1);
    for(const item of [...sourceGroup.children]){sourceGroup.remove(item);item.geometry?.dispose();item.material?.dispose();}
    const p=model.objects.find(o=>o.id==='F').pose;
    tube(sourceGroup,[[tankPose[0]+.8,tankPose[1]-.28,tankPose[2]],[p[0]-.3,tankPose[1]-.28,p[2]],[p[0]-.3,p[1],p[2]],[p[0],p[1],p[2]]],.045,0x8a9a8c,'F');
    // Existing red manual handle; source options do not actuate a device.
    box(sourceGroup,[.16,.035,.065],0xb94e37,[p[0]-.12,p[1]+.085,p[2]],'F');
    if(option==='multi'){
      const normal=[p[0]-.1,p[1]-.12,p[2]+.38];model.objects.find(o=>o.id==='household-outlet').pose=normal;
      tube(sourceGroup,[[p[0]-.12,p[1],p[2]],[p[0]-.12,p[1],normal[2]],[normal[0],normal[1],normal[2]]],.045,0x7c9690,'household-outlet');
      box(sourceGroup,[.16,.035,.065],0xb94e37,[p[0]-.12,p[1]+.085,normal[2]],'household-outlet');
      mesh(sourceGroup,new THREE.TorusGeometry(.055,.012,6,12),0x6b8b86,normal,'household-outlet');
    }
  }
  rebuildSource(model.source_option);
  box(groups.room, [.36, .1, .25], 0xe1e5d7, model.objects.find(o => o.id === 'AP').pose, 'AP');
  let routeMesh=tube(groups.water, model.water_route.points, .035, P.orange, 'route');
  for(let x=0;x<=5.7;x+=.48){
    const clip=mesh(groups.water,new THREE.TorusGeometry(.052,.011,6,12),0x667562,[x,model.layout.railHeight,model.layout.pipeZ],'rail');clip.rotation.y=Math.PI/2;
    box(groups.water,[.04,.05,.055],0x667562,[x,model.layout.railHeight,4.85],'rail');
  }
  const dimensionLines=new Map();
  for(const d of model.dimension_mappings||[]){const item=line(groups.dimensions,d.points,d.kind==='estimate'?0xb58c49:d.kind==='owner'?0x548151:0x73919a,true);dimensionLines.set(d.id,item);}
  const c = model.objects.find(o => o.id === 'C').pose;
  mesh(groups.building, new THREE.SphereGeometry(.09, 12, 10), 0x8a9e6a, c, 'C');
  const plantGeometry = new THREE.SphereGeometry(1, 8, 6);
  function rebuildPots(next) {
    const obsolete = new Set();
    groups.pots.traverse(o => { if (o.isMesh) obsolete.add(o); });
    for (let i = pickables.length - 1; i >= 0; i--) if (obsolete.has(pickables[i])) pickables.splice(i, 1);
    while (groups.pots.children.length) {
      const child = groups.pots.children.pop();
      child.traverse(o => { if (o.geometry && o.geometry !== plantGeometry) o.geometry.dispose(); if (o.material) o.material.dispose(); });
    }
    pots = next;
    for (const [index, p] of pots.entries()) {
      const id = 'pot:' + p.id;
      if (p.type === 'round') mesh(groups.pots, new THREE.CylinderGeometry(.22, .17, .3, 14), 0xb47b56, [p.x, .23, p.z], id);
      else box(groups.pots, [.45, .28, .4], 0xe9e6d8, [p.x, .22, p.z], id);
      mesh(groups.pots, new THREE.CylinderGeometry(.18, .18, .015, 12), P.soil, [p.x, .375, p.z], id);
      for (let j = 0; j < 6; j++) {
        const angle = j * 2.4 + index * .7, h = .55 + .11 * Math.sin(index + j);
        tube(groups.pots, [[p.x, .36, p.z], [p.x + .08 * Math.cos(angle), h, p.z + .08 * Math.sin(angle)]], .009, P.green, id);
        const leaf = mesh(groups.pots, plantGeometry, [0x4a754e, 0x678357, 0x82945a][j % 3], [p.x + .1 * Math.cos(angle), h, p.z + .1 * Math.sin(angle)], id);
        leaf.scale.set(.09, .045, .22); leaf.rotation.set(.3, angle, .4);
      }
      // Soft drop line and small manual connector: a sketch, no live state.
      tube(groups.pots, [[p.x, 1.15, model.layout.pipeZ], [p.x, .98, model.layout.pipeZ], [p.x + .035, .5, p.z], [p.x, .42, p.z]], .012, 0x6e7860, id);
      box(groups.pots, [.07, .045, .045], P.orange, [p.x, 1.09, model.layout.pipeZ], id);
    }
    select(selected);
  }
  const ambient = new THREE.HemisphereLight(0xfff9e8, 0xc6cdb4, 2.3); scene.add(ambient);
  const sun = new THREE.DirectionalLight(0xfff6dd, 3);
  sun.position.set(-4, 14, 8); sun.castShadow = true; sun.shadow.mapSize.set(1024, 1024);
  Object.assign(sun.shadow.camera, { left: -12, right: 12, top: 12, bottom: -12, near: .5, far: 40 });
  sun.shadow.bias = -.0003; sun.shadow.normalBias = .03; scene.add(sun);
  const base = new THREE.Mesh(new THREE.PlaneGeometry(100, 100), new THREE.ShadowMaterial({ color: 0x53644a, opacity: .1 }));
  base.rotation.x = -Math.PI / 2; base.position.y = -.12; base.receiveShadow = true; scene.add(base);
  function select(id) {
    selected = id;
    for (const item of pickables) item.material.emissive?.setHex(item.userData.pickId === id ? 0x23371a : 0x000000);
    refreshLabelNodes();
  }
  function refreshLabelNodes() {
    const ids = [...new Set(['terrace', 'balcony', 'F', 'zoneB', 'route-peak', ...(model.source_option==='multi'?['household-outlet']:[]),selected, ...(model.dimension_mappings||[]).filter(d=>d.views.includes(currentCamera)).map(d=>'dim:'+d.id)])];
    for (const [id, element] of labels) if (!ids.includes(id)) { element.remove(); labels.delete(id); }
    for (const id of ids) if (!labels.has(id)) {
      const object = model.objects.find(o => o.id === id), dimension=(model.dimension_mappings||[]).find(d=>'dim:'+d.id===id);
      const element = document.createElement('span'); element.className = 'scene-label';
      element.textContent = object?.short || dimension?.label || 'Chậu mẫu';
      if(dimension){element.classList.add('dimension-label',dimension.kind);element.title=dimension.mapping;}
      if(id==='route-peak')element.classList.add('route-peak-label');
      labelHost.append(element); labels.set(id, element);
    }
    for (const [id, element] of labels) element.classList.toggle('selected', id === selected);
  }
  const presets = {
    all: { pos: [-11, 12, 17], target: [2.4, 3.0, .8], h: 13.7 },
    balcony: { pos: [9, 5.4, 12], target: [2.7, 1.25, 3.85], h: 5.7 },
    source: { pos: [-5, 10, 6], target: [4.0, 4.95, -3.35], h: 5.6 },
    right: { pos: [-2, 9, 11], target: [5.25, 3.6, .5], h: 10.8 }
  };
  function resize() {
    const width = host.clientWidth, height = host.clientHeight;
    if (!width || !height) return;
    const aspect = width / height;
    camera.left = -viewHeight * aspect / 2; camera.right = viewHeight * aspect / 2;
    camera.top = viewHeight / 2; camera.bottom = -viewHeight / 2;
    camera.updateProjectionMatrix(); renderer.setSize(width, height, false);
  }
  function setCamera(name) {
    const p = presets[name] || presets.all;
    currentCamera=name;
    for(const d of model.dimension_mappings||[])dimensionLines.get(d.id).visible=d.views.includes(name);
    refreshLabelNodes();
    viewHeight = p.h; camera.position.set(...p.pos); controls.target.set(...p.target); camera.zoom = 1;
    controls.update(); resize();
  }
  const raycaster = new THREE.Raycaster(), pointer = new THREE.Vector2();
  const transform=new TransformControls(camera,renderer.domElement);
  transform.setMode('translate');transform.setSpace('world');transform.setSize(.72);
  const transformHelper=transform.getHelper();scene.add(transformHelper);
  let editMode=false,selectedPoint=0,handles=[],routePoints=model.water_route.points.map(p=>[...p]),pendingRouteEdit=false;
  let routeLabels=[...model.water_route.point_labels],peakPoint=null;
  let sourcePoint=[...model.objects.find(o=>o.id==='F').pose];
  function updateRouteHints(){
    for(const child of [...groups.routeHints.children]){groups.routeHints.remove(child);child.geometry?.dispose();child.material?.dispose();}
    peakPoint=routePoints.reduce((a,p)=>p[1]>a[1]?p:a,routePoints[0]);
    const peak=mesh(groups.routeHints,new THREE.SphereGeometry(.075,12,8),0xd4a335,peakPoint,null,{depthTest:false});peak.renderOrder=4;
    line(groups.routeHints,[[peakPoint[0],t.floor,peakPoint[2]],peakPoint],0xbc9343,true);
    routeLabels.forEach((label,i)=>{
      if(!/gá|neo|đỡ/.test(label)||!routePoints[i])return;
      const diamond=mesh(groups.routeHints,new THREE.OctahedronGeometry(.085),0xc79935,routePoints[i],null,{wireframe:true,depthTest:false});diamond.renderOrder=4;
    });
  }
  function updateRouteTube(points){
    const old=routeMesh;groups.water.remove(old);const at=pickables.indexOf(old);if(at>=0)pickables.splice(at,1);old.geometry.dispose();old.material.dispose();
    routeMesh=tube(groups.water,points,.035,P.orange,'route');
  }
  function selectRoutePoint(index){
    selectedPoint=Math.max(0,Math.min(index,handles.length-1));
    handles.forEach((h,i)=>{h.material.color.setHex(i===selectedPoint?0xe39a2e:0x657e52);});
    if(editMode&&handles[selectedPoint])transform.attach(handles[selectedPoint]);
    editHooks.onRoutePointSelect?.(selectedPoint);
  }
  function setRoute(points,labels=model.water_route.point_labels){
    routePoints=points.map(p=>[...p]);
    routeLabels=[...labels];
    const delta=new THREE.Vector3(...routePoints[0]).sub(new THREE.Vector3(...sourcePoint));
    for(const item of pickables)if(['F','household-outlet'].includes(item.userData.pickId))item.position.add(delta);
    const normal=model.objects.find(o=>o.id==='household-outlet');normal.pose=normal.pose.map((v,i)=>v+delta.getComponent(i));
    sourcePoint=[...routePoints[0]];model.objects.find(o=>o.id==='F').pose=[...sourcePoint];
    updateRouteTube(routePoints);
    updateRouteHints();
    if(handles.length!==routePoints.length){
      transform.detach();for(const h of handles){groups.edit.remove(h);h.geometry.dispose();h.material.dispose();}
      handles=routePoints.map((p,i)=>{const h=mesh(groups.edit,new THREE.SphereGeometry(.095,12,10),0x657e52,p,null,{depthTest:false,transparent:true,opacity:.92});h.userData.routeIndex=i;h.renderOrder=5;return h;});
    }else handles.forEach((h,i)=>h.position.set(...routePoints[i]));
    selectRoutePoint(selectedPoint);
  }
  transform.addEventListener('dragging-changed',event=>{controls.enabled=!event.value;if(event.value)editHooks.onRouteDragStart?.();});
  transform.addEventListener('objectChange',()=>{
    routePoints=handles.map(h=>h.position.toArray());
    const delta=new THREE.Vector3(...routePoints[0]).sub(new THREE.Vector3(...sourcePoint));
    for(const item of pickables)if(['F','household-outlet'].includes(item.userData.pickId))item.position.add(delta);
    const normal=model.objects.find(o=>o.id==='household-outlet');normal.pose=normal.pose.map((v,i)=>v+delta.getComponent(i));
    sourcePoint=[...routePoints[0]];model.objects.find(o=>o.id==='F').pose=[...sourcePoint];
    updateRouteTube(routePoints);pendingRouteEdit=true;
    updateRouteHints();
  });
  function setEditMode(value){editMode=value;groups.edit.visible=value;if(value)selectRoutePoint(selectedPoint);else transform.detach();}
  groups.edit.visible=false;setRoute(routePoints);
  let pointerStart = null;
  const down = event => { pointerStart = [event.clientX, event.clientY]; };
  function visible(object) { for (let p = object; p; p = p.parent) if (!p.visible) return false; return true; }
  const up = event => {
    if (!pointerStart || Math.hypot(event.clientX - pointerStart[0], event.clientY - pointerStart[1]) > 5) return;
    const rect = renderer.domElement.getBoundingClientRect();
    if (!rect.width || !rect.height) return;
    pointer.set((event.clientX - rect.left) / rect.width * 2 - 1, -(event.clientY - rect.top) / rect.height * 2 + 1);
    raycaster.setFromCamera(pointer, camera);
    if(editMode){
      if(transform.axis)return;
      const pointHit=raycaster.intersectObjects(handles,false)[0];
      if(pointHit){selectRoutePoint(pointHit.object.userData.routeIndex);return;}
    }
    const hit = raycaster.intersectObjects(pickables.filter(visible), false)[0];
    if (hit) onSelect(hit.object.userData.pickId);
  };
  renderer.domElement.addEventListener('pointerdown', down);
  renderer.domElement.addEventListener('pointerup', up);
  function updateLabels() {
    for (const [id, element] of labels) {
      const item = model.objects.find(o => o.id === id), dimension=(model.dimension_mappings||[]).find(d=>'dim:'+d.id===id);
      const pot = id.startsWith('pot:') && pots.find(p => 'pot:' + p.id === id);
      const mid=dimension?dimension.points[0].map((v,i)=>(v+dimension.points[1][i])/2):null;
      const pos = item?.pose || mid || (id==='route-peak'?peakPoint:null) || (pot ? [pot.x, .75, pot.z] : null);
      if (!pos) { element.hidden = true; continue; }
      if(id==='household-outlet'&&model.source_option!=='multi'){element.hidden=true;continue;}
      if(id==='route-peak')element.textContent=`Đỉnh ≈${((pos[1]-t.floor)*model.coordinate_frame.estimated_meters_per_scene_unit[1]).toFixed(2)} m trên sàn`;
      const projection = new THREE.Vector3(pos[0], pos[1] + (id === 'balcony' ? -.35 : .45), pos[2]).project(camera);
      element.hidden = projection.z < -1 || projection.z > 1 || Math.abs(projection.x) > .95 || Math.abs(projection.y) > .95 || (id === 'route' && !groups.water.visible) || (id==='route-peak'&&!groups.routeHints.visible) || (pot && !groups.pots.visible) || (['W','window-left'].includes(id) && !groups.room.visible) || (dimension&&!groups.dimensions.visible) || (dimension&&host.clientWidth<600&&!['length-A','terrace-route'].includes(dimension.id));
      element.style.left = `${(projection.x * .5 + .5) * host.clientWidth}px`;
      element.style.top = `${(-projection.y * .5 + .5) * host.clientHeight}px`;
    }
  }
  function animate() {
    if(pendingRouteEdit){pendingRouteEdit=false;editHooks.onRouteChange?.(routePoints.map(p=>[...p]),selectedPoint);}
    controls.update(); renderer.render(scene, camera); updateLabels(); ready = true;
  }
  const visibility = () => renderer.setAnimationLoop(document.hidden ? null : animate);
  document.addEventListener('visibilitychange', visibility);
  const observer = new ResizeObserver(resize); observer.observe(host);
  rebuildPots(pots); setCamera('all'); renderer.setAnimationLoop(animate);
  return {
    select, setCamera, setPots: rebuildPots, setRoute, setEditMode, selectRoutePoint,
    setSourceOption: option=>{rebuildSource(option);refreshLabelNodes();},
    setLayer: (key, value) => { if (groups[key]) groups[key].visible = value; groups.routeHints.visible=groups.water.visible&&groups.dimensions.visible; },
    cutaway: value => { groups.closed.visible = !value; },
    diagnostics: () => ({ renderer: 'Three.js ' + THREE.REVISION, ready, triangles: renderer.info.render.triangles, selected, potCount: pots.length, camera: camera.position.toArray(), target: controls.target.toArray(), zoom: camera.zoom, width: host.clientWidth, height: host.clientHeight }),
    dispose() {
      renderer.setAnimationLoop(null); controls.dispose(); transform.dispose(); observer.disconnect(); document.removeEventListener('visibilitychange', visibility);
      renderer.domElement.removeEventListener('pointerdown', down); renderer.domElement.removeEventListener('pointerup', up);
      const geometries = new Set(), materials = new Set(); scene.traverse(o => { if (o.geometry) geometries.add(o.geometry); if (o.material) materials.add(o.material); });
      geometries.add(plantGeometry); for (const g of geometries) g.dispose(); for (const m of materials) m.dispose(); renderer.dispose(); renderer.domElement.remove(); labelHost.replaceChildren();
    }
  };
}
