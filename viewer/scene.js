import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { TransformControls } from 'three/addons/controls/TransformControls.js';
import {siteWallBoxes} from './route-math.js';
import {installationFeatures,installationPose,installationStage,installationPickId} from './installation.js';

export function createScene(host, labelHost, model, onSelect, hooks = {}, {pixelRatio=Math.min(devicePixelRatio,2),pipeRadius,background} = {}) {
  const scene = new THREE.Scene();
  const renderer = new THREE.WebGLRenderer({antialias:true,alpha:true});
  renderer.setPixelRatio(pixelRatio);
  if(background!==undefined)scene.background=new THREE.Color(background);
  renderer.outputColorSpace=THREE.SRGBColorSpace;renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1.25;
  host.append(renderer.domElement);
  const camera=new THREE.PerspectiveCamera(46,1,.015,180);
  const controls=new OrbitControls(camera,renderer.domElement);
  controls.enableDamping=true;controls.minDistance=.15;controls.maxDistance=65;controls.maxPolarAngle=Math.PI*.98;
  const groups={};for(const name of ['building','room','upper','closed','water','pots','dimensions','edit','comparison','future']){groups[name]=new THREE.Group();scene.add(groups[name]);}
  const pickables=[],labels=new Map(),object=id=>model.objects.find(o=>o.id===id);
  let selected='F',highlight=true,ready=false,mode='orbit',currentCamera='all',editMode=false,points=structuredClone(model.water_route.points),pointLabels=[...model.water_route.point_labels],handles=[],selectedPoint=0,dragDirty=false,comparison=null;
  const keys=new Set();let looking=false,lastPointer=null,speed=2.2,lastFrame=performance.now(),pots=model.pot_samples;
  const {balcony:b,terrace:t,room:r,zoneB:zb,rightWall:rw,rightLedge:ledge}=model.layout;
  const P={wall:0xe6e0cf,floor:0xd9d0b9,rail:0x617268,existing:0x8c9996};
  function mesh(group,geometry,color,pos,id,extra={}){
    const item=new THREE.Mesh(geometry,new THREE.MeshStandardMaterial({color,roughness:.8,...extra}));item.position.set(...pos);group.add(item);
    if(id){item.userData.pickId=id;pickables.push(item);}return item;
  }
  const box=(g,size,color,pos,id,extra)=>mesh(g,new THREE.BoxGeometry(...size),color,pos,id,extra);
  function tube(g,ps,radius,color,id,extra={}){
    const curve=new THREE.CurvePath();for(let i=1;i<ps.length;i++)if(new THREE.Vector3(...ps[i]).distanceTo(new THREE.Vector3(...ps[i-1]))>1e-7)curve.add(new THREE.LineCurve3(new THREE.Vector3(...ps[i-1]),new THREE.Vector3(...ps[i])));
    if(!curve.curves.length)return null;
    return mesh(g,new THREE.TubeGeometry(curve,Math.max(20,ps.length*12),radius,8,false),color,[0,0,0],id,extra);
  }
  function line(g,ps,color,dashed=false){
    const mat=dashed?new THREE.LineDashedMaterial({color,dashSize:.1,gapSize:.08}):new THREE.LineBasicMaterial({color,transparent:true,opacity:.65});
    const item=new THREE.Line(new THREE.BufferGeometry().setFromPoints(ps.map(p=>new THREE.Vector3(...p))),mat);item.computeLineDistances();g.add(item);return item;
  }
  function clear(group){
    const obsolete=new Set();group.traverse(o=>obsolete.add(o));for(let i=pickables.length-1;i>=0;i--)if(obsolete.has(pickables[i]))pickables.splice(i,1);
    for(const child of [...group.children]){group.remove(child);child.traverse(o=>{o.geometry?.dispose();if(o.material)for(const mat of [].concat(o.material))mat.dispose();});}
  }
  function slab(g,bounds,color,id){box(g,[bounds.maxX-bounds.minX,.08,bounds.maxZ-bounds.minZ],color,[(bounds.minX+bounds.maxX)/2,bounds.floor-.04,(bounds.minZ+bounds.maxZ)/2],id);}
  slab(groups.building,b,P.floor,'balcony');slab(groups.room,r,0xe2decf,'room-area');slab(groups.upper,t,P.floor,'terrace');slab(groups.upper,zb,0xdce2c8,'zoneB');
  // One set of solid bounds drives rendering and tube-clearance checks.
  for(const wall of siteWallBoxes(model))box(groups[wall.group],wall.max.map((v,i)=>v-wall.min[i]),wall.id==='right-wall'?0xcac7b8:wall.id==='right-ledge'?0xc8c3ae:P.wall,wall.max.map((v,i)=>(v+wall.min[i])/2),wall.id);
  box(groups.upper,[.1,.48,t.maxZ-t.minZ],P.wall,[0,t.floor+.24,(t.minZ+t.maxZ)/2],'terrace');
  box(groups.upper,[4.13,.48,.1],P.wall,[2.065,t.floor+.24,t.minZ],'terrace');
  for(let x=zb.minX+.2;x<zb.maxX-.1;x+=.4)for(let z=zb.minZ+.2;z<zb.maxZ-.1;z+=.4)box(groups.upper,[.14,.005,.14],0xb3bea4,[x,zb.floor+.005,z],'zoneB');
  for(const pipe of model.existing_pipes)tube(groups.upper,pipe.points,pipe.radius,P.existing,pipe.id);
  const railTop=model.layout.railTop,railZ=model.layout.railZ;
  box(groups.building,[4,.1,.1],P.wall,[2.03,.05,railZ],'fence-base');
  for(let x=.03;x<=4.04;x+=.4)box(groups.building,[.025,railTop-.135,.025],P.rail,[x,(railTop+.065)/2,railZ],'fence');
  box(groups.building,[4,.035,.045],P.rail,[2.03,railTop-.0175,railZ],'fence');
  for(let x=.08;x<4;x+=.4)line(groups.building,[[x,.015,b.minZ],[x,.015,b.maxZ]],0xbab8a5);
  box(groups.room,[4.13,.8,.1],P.wall,[2.065,.4,r.maxZ],'room-area');
  box(groups.room,[4.13,.8,.1],P.wall,[2.065,2.9,r.maxZ],'room-area');
  for(const [x,width]of [[.3,.6],[2.1,.25],[4.02,.22]])box(groups.room,[width,1.7,.1],P.wall,[x,1.65,r.maxZ],'room-area');
  const door=object('W').pose,win=object('window-left').pose;
  box(groups.room,[1.45,2.3,.05],0x657468,door,'W');
  for(const dx of [-.74,0,.74])box(groups.room,[.035,2.35,.08],0x40574a,[door[0]+dx,door[1],door[2]+.04],'W');
  box(groups.room,[1.3,1.5,.055],0x8eafa3,win,'window-left',{roughness:.45});
  for(const dx of [-.66,0,.66])box(groups.room,[.035,1.54,.08],0x40574a,[win[0]+dx,win[1],win[2]+.04],'window-left');
  box(groups.room,[.09,.16,.28],0xdfe5d3,object('AP').pose,'AP');
  box(groups.closed,[4.4,.08,zb.maxZ-t.minZ+.4],0x9fa998,[2.06,6,(zb.maxZ+t.minZ)/2],'terrace');groups.closed.visible=false;
  const tankPose=object('tank').pose,tankGroup=new THREE.Group();tankGroup.position.set(...tankPose);tankGroup.rotation.y=Math.PI/2;groups.upper.add(tankGroup);
  const tank=mesh(tankGroup,new THREE.CylinderGeometry(.53,.53,1.3,40),0xaeb9b2,[0,0,0],'tank',{metalness:.5,roughness:.4});tank.rotation.x=Math.PI/2;
  for(const dz of [-.5,-.28,.28,.5])mesh(tankGroup,new THREE.TorusGeometry(.535,.012,6,40),0x87978e,[0,0,dz],'tank',{metalness:.4});
  for(const dz of [-.46,.46])for(const dx of [-.34,.34])box(tankGroup,[.045,.33,.045],P.rail,[dx,-.67,dz],'tank');
  const pp=object('pump').pose;box(groups.upper,[.38,.25,.34],0x386553,pp,'pump');const motor=mesh(groups.upper,new THREE.CylinderGeometry(.11,.11,.36,16),0x4c7660,[pp[0]+.1,pp[1],pp[2]],'pump');motor.rotation.z=Math.PI/2;
  box(groups.upper,[.48,.08,.42],0x9b7c5c,[pp[0],t.floor+.05,pp[2]],'pump');
  const sourceGroup=new THREE.Group();groups.upper.add(sourceGroup);
  {
    const p=object('F').pose;
    tube(sourceGroup,[[tankPose[0]+.65,tankPose[1]-.35,0],[3.1,3.85,0],[3.1,3.65,.25],[3.7,3.65,.25],[3.85,3.65,.25]],.032,P.existing,'existing-blue-valves');
    for(const x of [3.22,3.58]){box(sourceGroup,[.13,.035,.1],0x377e9b,[x,3.73,.25],'existing-blue-valves');tube(sourceGroup,[[x,3.65,.25],[x,3.72,.25]],.02,P.existing,'existing-blue-valves');}
    tube(sourceGroup,[[3.1,3.65,.25],[p[0]-.15,3.65,.25],[p[0]-.15,p[1],p[2]],[p[0],p[1],p[2]]],.025,P.existing,'F');
    box(sourceGroup,[.15,.03,.055],0xbd4d39,[p[0]-.09,p[1]+.065,p[2]],'F');
  }
  const routeGroup=new THREE.Group();groups.water.add(routeGroup,groups.future);
  const palette=model.installation.stages;
  const pendingColor=palette.deferred.color;
  function rebuildRoute(){
    clear(routeGroup);clear(groups.future);
    for(const {id,style,stage,points:ps} of installationFeatures(model)){
      const pickId=installationPickId(id),end=ps.at(-1),color=palette[stage].color;
      if(style==='direction'||style==='outline'){
        const feature=line(groups.future,ps,pendingColor,true);feature.material.dashSize=.14;feature.material.gapSize=.09;
        if(style==='direction')mesh(groups.future,new THREE.SphereGeometry(.026,8,6),pendingColor,end,pickId);
      }else if(style==='junction'){
        // The deferred Y shares the current cap location and is only labelled
        // when inspected; there is no installed fitting to mesh over the cap.
        continue;
      }else if(style==='manifold'){
        tube(routeGroup,ps,.035,0x927054,pickId);
        box(routeGroup,[.085,.035,.12],0xc85135,[ps[0][0],ps[0][1]+.052,(ps[0][2]+end[2])/2],pickId);
      }else if(style==='port-cap'){
        tube(routeGroup,ps,.014,0x9b7754,pickId);
        box(routeGroup,[.034,.048,.044],0xb5a180,end,pickId);
      }else if(style==='wash'){
        tube(routeGroup,ps,.022,0x9c7952,pickId);
        box(routeGroup,[.075,.027,.036],0x6b8178,[end[0],ps[0][1]+.046,end[2]-.065],pickId);
      }else if(style==='solenoid'){
        tube(routeGroup,ps,.032,0x9c754e,pickId);
        const pos=installationPose(model,id);
        box(routeGroup,[.12,.09,.085],0x3d5960,[pos[0],pos[1]+.034,pos[2]],pickId);
        box(routeGroup,[.042,.026,.053],0x29424b,[pos[0],pos[1]+.091,pos[2]],pickId);
      }else{
        const radius=style==='rigid'?(pipeRadius??model.routing_geometry.tube_radius_m):style==='hose'?.019:style==='end-cap'?Math.max(.029,(pipeRadius??0)*1.35):.023;
        tube(routeGroup,ps,radius,color,pickId);
        if(style==='end-cap')mesh(routeGroup,new THREE.SphereGeometry(radius,12,8),0xd66e2c,end,pickId);
      }
    }
    select(selected);
  }
  function rebuildPots(pots){
    clear(groups.pots);
    for(const [i,p]of pots.entries()){
      const id='pot:'+p.id;mesh(groups.pots,new THREE.CylinderGeometry(.15,.12,.23,12),0xb68c63,[p.x,.16,p.z],id);
      mesh(groups.pots,new THREE.CylinderGeometry(.12,.12,.01,12),0x655237,[p.x,.28,p.z],id);
      for(let j=0;j<4;j++){const a=j*2.4+i,h=.44+.1*Math.sin(j+i);tube(groups.pots,[[p.x,.28,p.z],[p.x+.05*Math.cos(a),h,p.z+.05*Math.sin(a)]],.005,0x587954,id);const leaf=mesh(groups.pots,new THREE.SphereGeometry(1,7,5),0x6f905d,[p.x+.06*Math.cos(a),h,p.z+.06*Math.sin(a)],id);leaf.scale.set(.06,.03,.13);leaf.rotation.y=a;}
    }select(selected);
  }
  for(const d of model.dimension_mappings||[])line(groups.dimensions,d.points,d.kind==='owner'?0x5c8a6c:0xb9964d,true);
  scene.add(new THREE.HemisphereLight(0xfff8e8,0xc1ccb7,2.6));const sun=new THREE.DirectionalLight(0xfff7e4,3);sun.position.set(-4,12,9);scene.add(sun);
  const selectionBox=new THREE.Box3Helper(new THREE.Box3(),0xd59d37);selectionBox.material.depthTest=false;selectionBox.renderOrder=5;selectionBox.visible=false;scene.add(selectionBox);
  function matches(item,id){
    const pid=item.userData.pickId;
    if(id==='source-assembly')return ['F','tank','pump','dnv5','household-outlet','existing-blue-valves','source-hose','future-valve','valve-adapter'].includes(pid);
    if(id==='mini-low-walls')return ['mini-low-walls','mini-edge'].includes(pid);
    return pid===id;
  }
  function visible(item){for(let p=item;p;p=p.parent)if(!p.visible)return false;return true;}
  function boundsFor(id){
    scene.updateMatrixWorld(true);const bounds=new THREE.Box3();
    for(const item of pickables)if(matches(item,id)&&visible(item))bounds.expandByObject(item);
    if(bounds.isEmpty()&&object(id)?.status!=='historical'){const p=installationPose(model,id);if(p)bounds.setFromCenterAndSize(new THREE.Vector3(...p),new THREE.Vector3(.2,.2,.2));}
    return bounds;
  }
  function select(id){
    selected=id;for(const item of pickables)item.material.emissive?.setHex(highlight&&matches(item,id)?0x37562a:0);
    const bounds=boundsFor(id);selectionBox.visible=highlight&&!bounds.isEmpty();if(!bounds.isEmpty())selectionBox.box.copy(bounds);
  }
  function resize(){const w=host.clientWidth,h=host.clientHeight;if(!w||!h)return;camera.aspect=w/h;camera.updateProjectionMatrix();renderer.setSize(w,h,false);}
  function stopMotion(){keys.clear();looking=false;lastPointer=null;}
  function setNavigation(next){stopMotion();mode=next==='fly'?'fly':'orbit';controls.enabled=mode==='orbit'&&!editMode;host.dataset.navigation=mode;hooks.onNavigationChange?.(mode);}
  function focus(id){
    const bounds=boundsFor(id);if(bounds.isEmpty())return;stopMotion();const center=bounds.getCenter(new THREE.Vector3()),size=bounds.getSize(new THREE.Vector3());
    const radius=Math.max(.18,size.length()/2),dist=radius/Math.sin(THREE.MathUtils.degToRad(camera.fov/2))/Math.min(1,camera.aspect)*1.25;
    const direction=camera.position.clone().sub(controls.target).normalize();if(!Number.isFinite(direction.x)||direction.length()<.1)direction.set(-.8,.6,1).normalize();
    camera.position.copy(center).addScaledVector(direction,dist);controls.target.copy(center);camera.lookAt(center);if(mode==='orbit')controls.update();select(id);
  }
  const presets={all:{pos:[-7,9,zb.maxZ+7.4],target:[2.0,2.6,zb.maxZ/2]},source:{pos:[3.27,5.22,.78],target:[3.65,3.96,-.12]},right:{pos:[-3,7,zb.maxZ+3.4],target:[3.8,3.2,zb.maxZ/2]},balcony:{pos:[-2.7,3.3,b.maxZ+3.4],target:[2,1.1,(b.minZ+b.maxZ)/2]},mini:{pos:[-1,6,zb.maxZ+2.4],target:[2.5,3.8,(zb.minZ+zb.maxZ)/2]}};
  function setCamera(name){stopMotion();currentCamera=presets[name]?name:'all';const p=presets[currentCamera];camera.position.set(...p.pos);controls.target.set(...p.target);camera.lookAt(controls.target);if(mode==='orbit')controls.update();}
  const transform=new TransformControls(camera,renderer.domElement);transform.setSize(.7);scene.add(transform.getHelper());
  function selectRoutePoint(index){selectedPoint=Math.max(0,Math.min(index,handles.length-1));handles.forEach((h,i)=>h.material.color.setHex(i===selectedPoint?0xe7a042:0x66825b));if(editMode&&handles[selectedPoint])transform.attach(handles[selectedPoint]);}
  function setRoute(next,nextLabels=pointLabels){
    points=structuredClone(next);pointLabels=[...nextLabels];model.water_route.points=structuredClone(next);rebuildRoute();
    if(handles.length!==points.length){transform.detach();clear(groups.edit);handles=points.map(p=>mesh(groups.edit,new THREE.SphereGeometry(.045,12,8),0x66825b,p,null,{depthTest:false}));}
    else handles.forEach((h,i)=>h.position.set(...points[i]));selectRoutePoint(selectedPoint);
  }
  function setEditMode(value){editMode=Boolean(value);if(editMode)setNavigation('orbit');groups.edit.visible=editMode;if(editMode)selectRoutePoint(selectedPoint);else transform.detach();}
  transform.addEventListener('dragging-changed',e=>{controls.enabled=!e.value&&mode==='orbit';if(e.value){stopMotion();hooks.onRouteDragStart?.();}});
  transform.addEventListener('objectChange',()=>{points=handles.map(h=>h.position.toArray());model.water_route.points=structuredClone(points);rebuildRoute();dragDirty=true;});
  function setComparison(key){comparison=key;clear(groups.comparison);if(key&&model.route_presets[key]?.read_only)tube(groups.comparison,model.route_presets[key].points,.012,0x8399a4,null,{transparent:true,opacity:.55});}
  let pointerStart=null;
  function pointerDown(e){host.focus({preventScroll:true});pointerStart=[e.clientX,e.clientY];if(mode==='fly'&&e.button===2){looking=true;lastPointer=[e.clientX,e.clientY];e.preventDefault();}}
  function pointerMove(e){if(mode!=='fly'||!looking)return;const dx=e.clientX-lastPointer[0],dy=e.clientY-lastPointer[1];lastPointer=[e.clientX,e.clientY];const rot=new THREE.Euler().setFromQuaternion(camera.quaternion,'YXZ');rot.y-=dx*.004;rot.x=THREE.MathUtils.clamp(rot.x-dy*.004,-1.54,1.54);rot.z=0;camera.quaternion.setFromEuler(rot);}
  function pointerUp(e){const wasLooking=looking;looking=false;lastPointer=null;if(wasLooking||!pointerStart||e.button!==0||Math.hypot(e.clientX-pointerStart[0],e.clientY-pointerStart[1])>5)return;const rect=renderer.domElement.getBoundingClientRect();if(!rect.width||!rect.height)return;
    const ray=new THREE.Raycaster();ray.setFromCamera(new THREE.Vector2((e.clientX-rect.left)/rect.width*2-1,-(e.clientY-rect.top)/rect.height*2+1),camera);
    if(editMode){if(transform.axis)return;const hit=ray.intersectObjects(handles,false)[0];if(hit){selectRoutePoint(handles.indexOf(hit.object));hooks.onRoutePointSelect?.(selectedPoint);return;}}
    const hit=ray.intersectObjects(pickables.filter(visible),false).find(h=>object(h.object.userData.pickId)||h.object.userData.pickId?.startsWith('pot:'));if(hit)onSelect(hit.object.userData.pickId);
  }
  const keyDown=e=>{if(e.target!==host&&e.target!==renderer.domElement)return;if(e.code==='Escape'){stopMotion();setNavigation('orbit');return;}if(mode==='fly'&&['KeyW','KeyA','KeyS','KeyD','KeyQ','KeyE','ArrowLeft','ArrowRight','ArrowUp','ArrowDown','ShiftLeft','ShiftRight'].includes(e.code)){keys.add(e.code);e.preventDefault();}};
  const keyUp=e=>keys.delete(e.code),context=e=>{if(mode==='fly')e.preventDefault();},wheel=e=>{if(mode==='fly'){speed=THREE.MathUtils.clamp(speed*Math.exp(-e.deltaY*.001),.2,12);e.preventDefault();}};
  renderer.domElement.addEventListener('pointerdown',pointerDown);renderer.domElement.addEventListener('pointermove',pointerMove);renderer.domElement.addEventListener('pointerup',pointerUp);renderer.domElement.addEventListener('contextmenu',context);renderer.domElement.addEventListener('wheel',wheel,{passive:false});
  const release=()=>{looking=false;lastPointer=null;};window.addEventListener('pointerup',release);host.addEventListener('keydown',keyDown);window.addEventListener('keyup',keyUp);host.addEventListener('blur',stopMotion);window.addEventListener('blur',stopMotion);
  function updateLabels(){
    const dimensions=(model.dimension_mappings||[]).filter(d=>d.views.includes(currentCamera)&&(currentCamera!=='all'||['M03','M07','M09','M12'].includes(d.id)));
    const landmarks=currentCamera==='source'?['F','dnv5','future-valve']:currentCamera==='all'?['F','capped-stub']:currentCamera==='balcony'?['balcony','capped-stub']:['zoneB'];
    const ids=[...new Set([...landmarks.filter(id=>id!=='capped-stub'||selected!=='y-deferred'),selected,...dimensions.map(d=>'dim:'+d.id)])];
    for(const [id,el]of labels)if(!ids.includes(id)){el.remove();labels.delete(id);}
    for(const id of ids){const d=dimensions.find(v=>'dim:'+v.id===id),o=object(id),pot=pots.find(p=>'pot:'+p.id===id);const pos=d?d.points[0].map((v,i)=>(v+d.points[1][i])/2):installationPose(model,id)|| (pot?[pot.x,.6,pot.z]:null);if(!pos)continue;
      let el=labels.get(id);if(!el){el=document.createElement('span');el.className='scene-label'+(d?' dimension-label owner':'');labelHost.append(el);labels.set(id,el);}el.textContent=d?.label||o?.short||'Chậu mẫu';el.classList.toggle('selected',highlight&&selected===id);
      const stage=installationStage(model,id);const p=new THREE.Vector3(...pos);if(!d)p.y+=.15;const projected=p.project(camera);el.hidden=projected.z< -1||projected.z>1||Math.abs(projected.x)>.96||Math.abs(projected.y)>.95||(d&&!groups.dimensions.visible)||(stage&&!groups.water.visible)||(stage==='deferred'&&!groups.future.visible);el.style.left=(projected.x*.5+.5)*host.clientWidth+'px';el.style.top=(-projected.y*.5+.5)*host.clientHeight+'px';
    }
  }
  function animate(){const now=performance.now(),dt=Math.min((now-lastFrame)/1000,.05);lastFrame=now;
    if(mode==='fly'&&!editMode){const yaw=(keys.has('ArrowLeft')?1:0)-(keys.has('ArrowRight')?1:0),pitch=(keys.has('ArrowUp')?1:0)-(keys.has('ArrowDown')?1:0);if(yaw||pitch){const rot=new THREE.Euler().setFromQuaternion(camera.quaternion,'YXZ');rot.y+=yaw*1.65*dt;rot.x=THREE.MathUtils.clamp(rot.x+pitch*1.3*dt,-1.54,1.54);rot.z=0;camera.quaternion.setFromEuler(rot);}const v=new THREE.Vector3((keys.has('KeyD')?1:0)-(keys.has('KeyA')?1:0),0,(keys.has('KeyS')?1:0)-(keys.has('KeyW')?1:0));if(v.lengthSq())v.normalize().applyQuaternion(camera.quaternion);v.y+=(keys.has('KeyE')?1:0)-(keys.has('KeyQ')?1:0);const delta=v.multiplyScalar(speed*(keys.has('ShiftLeft')||keys.has('ShiftRight')?3:1)*dt);camera.position.add(delta);controls.target.copy(camera.position).add(new THREE.Vector3(0,0,-1).applyQuaternion(camera.quaternion));}
    else controls.update();if(dragDirty){dragDirty=false;hooks.onRouteChange?.(structuredClone(points),selectedPoint);}renderer.render(scene,camera);updateLabels();ready=true;
  }
  const visibility=()=>{stopMotion();lastFrame=performance.now();renderer.setAnimationLoop(document.hidden?null:animate);};document.addEventListener('visibilitychange',visibility);
  const observer=new ResizeObserver(resize);observer.observe(host);setRoute(points);rebuildPots(model.pot_samples);setEditMode(false);setCamera('all');resize();renderer.setAnimationLoop(animate);
  return {select,focus,setCamera,setNavigation,setEditMode,selectRoutePoint,setRoute,setComparison,setPots:next=>{pots=next;rebuildPots(next);},setHighlight:value=>{highlight=value;select(selected);},
    setLayer:(key,value)=>{if(groups[key])groups[key].visible=value;if(key==='dimensions')labelHost.hidden=!value;if(key==='water')groups.comparison.visible=value;select(selected);},cutaway:value=>{groups.closed.visible=!value;select(selected);},
    // Still export uses the same built scene. Controls and viewer labels stay
    // on the interactive camera; the editorial preset supplies a second camera.
    renderStill({position,target,span,width,height,anchors=[]}){
      const half=span/2,aspect=width/height;
      const view=new THREE.OrthographicCamera(-half*aspect,half*aspect,half,-half,.015,180);
      view.position.set(...position);view.lookAt(...target);view.updateMatrixWorld();
      const size=renderer.getSize(new THREE.Vector2());
      renderer.setSize(width,height,false);renderer.render(scene,view);
      const image=renderer.domElement.toDataURL('image/png');
      const locations=Object.fromEntries(anchors.map(id=>{
        const p=new THREE.Vector3(...installationPose(model,id)).project(view);
        return [id,[(p.x*.5+.5)*width,(.5-p.y*.5)*height]];
      }));
      renderer.setSize(size.x,size.y,false);
      return {image,locations};
    },
    diagnostics:()=>({renderer:'Three.js '+THREE.REVISION,ready,selected,mode,highlight,comparison,editMode,camera:camera.position.toArray(),target:controls.target.toArray(),width:host.clientWidth,height:host.clientHeight,triangles:renderer.info.render.triangles,lines:renderer.info.render.lines}),
    dispose(){renderer.setAnimationLoop(null);observer.disconnect();controls.dispose();transform.dispose();document.removeEventListener('visibilitychange',visibility);window.removeEventListener('pointerup',release);window.removeEventListener('keyup',keyUp);window.removeEventListener('blur',stopMotion);host.removeEventListener('blur',stopMotion);host.removeEventListener('keydown',keyDown);for(const [name,fn]of [['pointerdown',pointerDown],['pointermove',pointerMove],['pointerup',pointerUp],['contextmenu',context],['wheel',wheel]])renderer.domElement.removeEventListener(name,fn);const geometries=new Set(),materials=new Set();scene.traverse(o=>{if(o.geometry)geometries.add(o.geometry);if(o.material)for(const mat of [].concat(o.material))materials.add(mat);});for(const g of geometries)g.dispose();for(const mat of materials)mat.dispose();renderer.dispose();renderer.domElement.remove();labelHost.replaceChildren();}
  };
}
