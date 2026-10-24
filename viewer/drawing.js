const escape = value => String(value).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&apos;'}[c]));
const colors = { ink: '#405640', muted: '#859077', orange: '#c7793a', plant: '#688554', wall: '#e7e5d6', floor: '#dcdcca', line: '#c8d0bc' };
export function drawSVG(model, state) {
  const { projection, selected, pots, layers } = state;
  const isPlan = projection === 'plan';
  const width = 1000, height = 440;
  const sectionPoint = p => [104 + (p[0] + .35) * 116, 366 - p[1] * 48];
  const planPoint = (p, forceUpper = false) => forceUpper || p[1] >= 4.2 ? [80 + (p[0] + .35) * 57, 112 + (p[2] + 4.3) * 27] : [570 + (p[0] + .35) * 57, 112 + (p[2] - .2) * 48];
  const project = isPlan ? planPoint : sectionPoint;
  const pointString = points => points.map(p => project(p).join(',')).join(' ');
  const text = (x, y, str, size = 12, color = colors.muted, extra = '') => `<text x="${x}" y="${y}" font-size="${size}" fill="${color}" ${extra}>${escape(str)}</text>`;
  const rect = (x,y,w,h,fill,extra='') => `<rect x="${x}" y="${y}" width="${w}" height="${h}" fill="${fill}" ${extra}/>`;
  const shapeBounds = (bounds, fill, upper) => {
    const a = planPoint([bounds.minX, bounds.floor, bounds.minZ], upper), b = planPoint([bounds.maxX, bounds.floor, bounds.maxZ], upper);
    return rect(a[0], a[1], b[0]-a[0], b[1]-a[1], fill, `stroke="${colors.line}" rx="3"`);
  };
  let body = `<defs><pattern id="grid" width="25" height="25" patternUnits="userSpaceOnUse"><circle cx="1" cy="1" r=".65" fill="#d5dcc8"/></pattern><marker id="arrow" viewBox="0 0 8 8" refX="6" refY="4" markerWidth="5" markerHeight="5" orient="auto-start-reverse"><path d="M1 1L6 4L1 7" fill="none" stroke="${colors.orange}" stroke-width="1.3"/></marker></defs>${rect(0,0,width,height,'#fafaf4')}${rect(0,0,width,height,'url(#grid)')}`;
  if (isPlan) {
    body += text(80,58,'SÂN THƯỢNG',14,colors.ink,'font-weight="600"') + text(570,58,'PHÒNG + BAN CÔNG',14,colors.ink,'font-weight="600"');
    body += text(80,80,'Bố cục minh họa · nhìn từ trên xuống',10) + text(570,80,'Tầng ngay dưới · tách để dễ đối chiếu',10);
    body += shapeBounds(model.layout.terrace, '#eef0e2', true);
    body += shapeBounds(model.layout.zoneB, '#e2e7d0', true);
    const zb=planPoint([model.layout.zoneB.minX,4.4,model.layout.zoneB.minZ],true);
    body += text(zb[0]+6,zb[1]+19,'ZONE B · mini',10,colors.ink);
    if (layers.room) body += shapeBounds(model.layout.room, '#e9eadf', false) + text(590,145,'PHÒNG',11);
    body += shapeBounds(model.layout.balcony, '#f1eadb', false);
    const railA = project([-.35,0,4.85]), railB = project([5.8,0,4.85]);
    body += `<path d="M${railA.join(' ')}L${railB.join(' ')}" stroke="#8a795e" stroke-width="5"/>`;
    body += text(590,290,'ZONE A',11,colors.ink);
  } else {
    body += text(35,40,'QUAN HỆ TẦNG / TUYẾN',11,colors.ink,'letter-spacing="1.3"');
    const upperY = sectionPoint([0,4.4,0])[1], lowerY = sectionPoint([0,0,0])[1];
    body += rect(103,upperY,714,9,colors.floor) + rect(103,lowerY,714,9,colors.floor);
    body += text(116,upperY-17,'SÂN THƯỢNG · NGUỒN NƯỚC',12,colors.ink,'font-weight="600"');
    body += text(116,lowerY+31,'BAN CÔNG · ZONE A',12,colors.ink,'font-weight="600"');
    if (layers.room) {
      const dx = sectionPoint(model.objects.find(o=>o.id==='W').pose)[0];
      body += rect(104,lowerY-177,714,177,'#edece1',`stroke="#d7dccb"`);
      body += rect(dx-53,lowerY-120,106,120,'#69766c',`stroke="#485e4e" stroke-width="3" rx="2"`) + `<path d="M${dx} ${lowerY-120}v120" stroke="#425c49" stroke-width="2"/>`;
      const wx=sectionPoint(model.objects.find(o=>o.id==='window-left').pose)[0];
      body += rect(wx-39,lowerY-123,78,73,'#b3cbc2',`stroke="#657b6e" stroke-width="3" rx="2"`) + `<path d="M${wx} ${lowerY-123}v73" stroke="#657b6e" stroke-width="2"/>`;
    }
    const zba=sectionPoint([model.layout.zoneB.minX,4.4,0])[0],zbb=sectionPoint([model.layout.zoneB.maxX,4.4,0])[0];
    body += rect(zba,upperY-22,zbb-zba,22,'#dce4c6',`stroke="#9fb08a"`) + text(zba+7,upperY-7,'ZONE B · mini',10,colors.ink);
    body += rect(103,lowerY-5,714,5,'#d8d5c0');
    for(let rx=110;rx<=810;rx+=34)body+=`<path d="M${rx} ${lowerY-5}v-52" stroke="#718168" stroke-width="2"/>`;
    body += `<path d="M103 ${lowerY-57}H817" stroke="#718168" stroke-width="3"/>`;
  }
  // Tank orientation and source options use the same canonical data as Three.js.
  const tank=model.objects.find(o=>o.id==='tank'),tankAt=project(tank.pose),yaw=model.source_assembly.tank_yaw_degrees*Math.PI/180;
  const tankX=Math.abs(Math.sin(yaw))*1.6+Math.abs(Math.cos(yaw))*1.24,tankZ=Math.abs(Math.cos(yaw))*1.6+Math.abs(Math.sin(yaw))*1.24;
  const tankWidth=tankX*(isPlan?57:116),tankHeight=isPlan?tankZ*27:1.24*48;
  body+=rect(tankAt[0]-tankWidth/2,tankAt[1]-tankHeight/2,tankWidth,tankHeight,'#c7cebb',`rx="12" stroke="#8e9c84" data-id="tank"`)+text(tankAt[0],tankAt[1]-tankHeight/2-7,'Bồn · trục vuông góc tường phải',10,colors.muted,'text-anchor="middle"');
  for(const u of [-.3,.3])body+=`<path d="M${tankAt[0]+tankWidth*u} ${tankAt[1]-tankHeight/2}v${tankHeight}" stroke="#96a28b" fill="none"/>`;
  if(model.source_option==='multi'){
    const a=project(model.objects.find(o=>o.id==='F').pose),b=project(model.objects.find(o=>o.id==='household-outlet').pose);
    body+=`<g data-id="household-outlet"><path d="M${a[0]} ${a[1]}L${b[0]} ${b[1]}" stroke="#6b8b86" stroke-width="3"/><circle cx="${b[0]}" cy="${b[1]}" r="4" fill="#6b8b86"/>${text(b[0]-8,b[1]+22,'Đầu sinh hoạt riêng · tay',9,'#5b7f78','text-anchor="end"')}</g>`;
  }
  // The neighbor wall and existing pipes are site constraints, separate from the proposed route.
  const rw=model.layout.rightWall,ledge=model.layout.rightLedge;
  if(isPlan){
    body+=shapeBounds({...ledge,maxZ:ledge.maxZ},'#d4d3c2',true);
    for(const upper of [true,false]){
      const bounds={minX:rw.x-rw.thickness/2,maxX:rw.x+rw.thickness/2,minZ:upper?rw.minZ:model.layout.room.minZ,maxZ:rw.maxZ,floor:upper?4.4:0};
      body+=shapeBounds(bounds,'#bdbda9',upper);
    }
    const at=planPoint([rw.x,4.4,rw.minZ],true);
    body+=text(at[0]+12,at[1]+12,'Tường hàng xóm',9,'#7d806b');
  }else{
    const a=sectionPoint([rw.x-rw.thickness/2,rw.maxY,0]),b=sectionPoint([rw.x+rw.thickness/2,rw.minY,0]);
    body+=rect(a[0],a[1],b[0]-a[0],b[1]-a[1],'#c2c1ad',`data-id="right-wall"`)+text(a[0]-6,a[1]+16,'Tường phải · kín',9,'#7a7d69','text-anchor="end"');
    const la=sectionPoint([ledge.minX,ledge.floor+ledge.height,0]),lb=sectionPoint([ledge.maxX,ledge.floor,0]);
    body+=rect(la[0],la[1],lb[0]-la[0],lb[1]-la[1],'#cecbb7',`data-id="right-ledge"`);
  }
  for(const pipe of model.existing_pipes){
    const coords=pipe.points.map(p=>project(p)),radius=isPlan?pipe.radius*57:pipe.radius*48;
    body+=`<g data-id="${escape(pipe.id)}"><title>Ống hiện hữu · vị trí/kích thước ước lượng</title>`;
    if(Math.hypot(coords[1][0]-coords[0][0],coords[1][1]-coords[0][1])<.01)body+=`<circle cx="${coords[0][0]}" cy="${coords[0][1]}" r="${radius}" fill="#89958e"/>`;
    else body+=`<polyline points="${coords.map(p=>p.join(',')).join(' ')}" stroke="#89958e" stroke-width="${radius*2}" stroke-linecap="round" fill="none"/>`;
    body+='</g>';
  }
  if (layers.water) {
    if (isPlan) {
      const upperRoute = model.water_route.points.filter(p=>p[1]>=4.2);
      body += `<polyline points="${pointString(upperRoute)}" fill="none" stroke="${colors.orange}" stroke-width="4" stroke-linejoin="round" data-id="route"/>`;
      const lowerRoute = model.water_route.points.filter(p=>p[1]<4.2);
      body += `<polyline points="${pointString(lowerRoute)}" fill="none" stroke="${colors.orange}" stroke-width="4" data-id="rail"/>`;
      body += `<path d="M458 254H535" stroke="${colors.orange}" stroke-dasharray="5 4" marker-end="url(#arrow)"/>` + text(456,242,'xuống',10,colors.orange);
    } else {
      body += `<polyline points="${pointString(model.water_route.points)}" fill="none" stroke="${colors.orange}" stroke-width="4" stroke-linejoin="round" data-id="route"/>`;
      body += text(855,231,'Tuyến cấp',12,colors.orange) + text(855,249,'đề xuất',11,colors.orange);
    }
  }
  if(layers.water&&layers.dimensions){
    const peak=model.water_route.points.reduce((a,p)=>p[1]>a[1]?p:a,model.water_route.points[0]),at=project(peak);
    const peakHeight=(peak[1]-model.layout.terrace.floor)*model.coordinate_frame.estimated_meters_per_scene_unit[1];
    body+=`<circle cx="${at[0]}" cy="${at[1]}" r="4" fill="#d4a335"/>`+text(at[0]+8,at[1]-10,`Đỉnh ≈${peakHeight.toFixed(2)} m trên sàn`,10,'#9a7736');
    for(const index of model.water_route.support_indices||[]){
      const p=model.water_route.points[index];if(!p)continue;const [x,y]=project(p);
      body+=`<path d="M${x} ${y-5}l5 5-5 5-5-5Z" stroke="#b38c38" stroke-width="1.5" fill="#fff8e4"><title>Vị trí cần gá/đỡ · chưa chốt cách lắp</title></path>`;
    }
  }
  if (layers.pots) for (const p of pots) {
    const [x,y] = project([p.x,.32,p.z]), id = 'pot:'+p.id;
    const rail = project([p.x,model.layout.railHeight,model.layout.pipeZ]);
    body += `<g data-id="${escape(id)}" role="button" tabindex="0" aria-label="Chọn chậu mẫu ${escape(p.id)}">`;
    if (layers.water) body += `<path d="M${rail.join(' ')}Q${x+4} ${y-16},${x} ${y}" fill="none" stroke="#8c9673" stroke-width="1.8"/><rect x="${rail[0]-3}" y="${rail[1]-3}" width="6" height="6" rx="1" fill="${colors.orange}"/>`;
    body += rect(x-12,y-3,24,18,p.type==='round'?'#c49b73':'#e0dfcb',`rx="${p.type==='round'?7:2}" stroke="${selected===id?'#365c42':'#aab399'}" stroke-width="${selected===id?3:1}"`);
    body += `<path d="M${x} ${y-1}v-19m0 10q-15 -17 -10 -18q15 -2 10 15m0-10q14 -18 14 -7q-3 12 -14 13" fill="#82986a" stroke="#617d50" stroke-width="1.1"/></g>`;
  }
  if(layers.dimensions)for(const d of model.dimension_mappings||[]){
    let coords=isPlan?d.svg_plan:d.svg_section;
    if(isPlan&&d.id==='length-A'){const a=project(d.points[0]),b=project(d.points[1]);coords=[a[0],a[1]+22,b[0],b[1]+22];}
    if(!coords)continue;
    const [x1,y1,x2,y2]=coords,color=d.kind==='estimate'?'#aa8345':d.kind==='owner'?'#548151':'#698b95';
    const vertical=Math.abs(y2-y1)>Math.abs(x2-x1),mx=(x1+x2)/2,my=(y1+y2)/2;
    body+=`<g class="measurement" data-dimension="${escape(d.id)}"><title>${escape(d.mapping)}</title><path d="M${x1} ${y1}L${x2} ${y2}" stroke="${color}" stroke-width="1.2" stroke-dasharray="3 3"/><path d="M${x1-3} ${y1-3}l6 6M${x2-3} ${y2-3}l6 6" stroke="${color}"/>${text(mx+(vertical?8:0),my+(vertical?0:-7),d.label,10,color,vertical?'':'text-anchor="middle"')}</g>`;
  }
  for (const id of ['F','C','W','window-left','AP','rail','zoneB']) {
    if ((id==='AP'||id==='W'||id==='window-left')&&!layers.room || id==='rail'&&!layers.water || (id==='AP'&&!isPlan&&selected!=='AP')) continue;
    const object = model.objects.find(o=>o.id===id), [x,y]=project(object.pose), active=selected===id;
    const offset = id==='rail' ? 18 : -12;
    body += `<g data-id="${id}" role="button" tabindex="0" aria-label="Chọn ${escape(object.label)}"><circle cx="${x}" cy="${y}" r="${active?7:5}" fill="${active?'#365c42':'#fafaf4'}" stroke="#718460" stroke-width="1.5"/>${text(x+12,y+offset,object.short,10,active?'#365c42':colors.ink)}</g>`;
  }
  body += text(25,426,'Xanh: nhãn ảnh / đầu mút tạm gán · Nâu ≈: Agent ước lượng',9,colors.muted);
  body += text(975,426,'SƠ ĐỒ · CHƯA CALIBRATION',9,colors.muted,'text-anchor="end" letter-spacing="1"');
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${width} ${height}" role="img" aria-label="${isPlan?'Mặt bằng sơ đồ hai tầng':'Mặt cắt sơ đồ quan hệ tầng'}" data-model-version="${escape(model.model_version)}" font-family="Segoe UI,system-ui,sans-serif"><title>${escape(model.title)} — sơ đồ không theo tỷ lệ</title><desc>Cùng nguồn với 3D. Vị trí dựng hình minh họa; không suy kích thước từ SVG.</desc><style>[data-id]{pointer-events:bounding-box}</style>${body}</svg>`;
}
