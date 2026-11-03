"""Public baseline focused synthetic semantics. Pure imports; no product/device operations."""
from pathlib import Path
from copy import deepcopy
from datetime import datetime, timedelta, timezone
import json
from prepare_data import derive_state, request_projection, digest, pointer, lookup, record_of, build_views
ROOT=Path(__file__).resolve().parent

def dt(value):
    t=datetime.fromisoformat(value)
    if t.utcoffset()!=timedelta(hours=7):raise ValueError("Expected fixed UTC+7")
    return t
def within_effectivity(cfg,when):
    t=dt(when)
    return cfg["enabled"] is True and t>=dt(cfg["effective_from"]) and (cfg["effective_until"] is None or t<dt(cfg["effective_until"]))
def eligible_planned_time(cfg,when):
    if not within_effectivity(cfg,when):return False
    t=dt(when);r=cfg["recurrence"];k=r["kind"]
    if k=="once":return t==dt(r["at"]) and t.second==0 and t.microsecond==0
    if k in ["daily","weekly"]:
        h,m=map(int,r["at_local_time"].split(":"))
        return t.hour==h and t.minute==m and t.second==0 and t.microsecond==0 and (k=="daily" or t.isoweekday() in r["days_of_week"])
    if k=="interval":
        delta=(t-dt(r["anchor_at"])).total_seconds()
        return delta>=0 and delta%(r["interval_minutes"]*60)==0
    return False
def active_cap(pairs):
    counts={}
    for schedule,contents,reports in pairs:
        app=lookup(reports,"record_id").get(schedule["last_known_applied_report_ref"])
        row=lookup(contents,"content_ref").get(app.get("content_ref")) if app else None
        if row and row["config"]["enabled"]:
            counts[schedule["zone_id"]]=counts.get(schedule["zone_id"],0)+1
    return all(n<=1 for n in counts.values())
def latest_confirmation(rows,schedule_id):
    candidates=[r for r in rows if r["event_kind"]=="schedule_applied" and r["subject_ref"]==schedule_id]
    # Single synthetic origin/order scope only. No production boot/cutover algorithm.
    return max(candidates,key=lambda r:r["source_order"])["record_id"] if candidates else None
def origin_payload(row):
    return {k:v for k,v in row.items() if k!="received_at_web"}
def ingest_known(records,incoming):
    prior=lookup(records,"record_id").get(incoming["record_id"])
    if not prior:return "inserted"
    return "duplicate_same_payload" if origin_payload(prior)==origin_payload(incoming) else "conflict"
def forbidden_keys(obj):
    bad={"physical_water_evidence","actor_ref","evidence_level","active_at_controller","processed",
         "requested_at_clock_quality","execution_context","schedule_config","schedule_version","hardware_family"}
    if isinstance(obj,dict):return (set(obj)&bad)|set().union(*(forbidden_keys(v) for v in obj.values()),set())
    if isinstance(obj,list):return set().union(*(forbidden_keys(v) for v in obj),set())
    return set()
def validate(data,views=None):
    checks=[]
    def ck(name,condition,detail=""):checks.append(dict(check=name,pass_=bool(condition),detail=detail))
    # Reject unresolvable lineage explicitly before resolving projections.
    # A negative control must exercise a guard, not pass because of a crash.
    for sid,scene in data["scenarios"].items():
        st=scene["state"];refs={c["content_ref"] for c in st["schedule_contents"]}
        for o in st["occurrences"]:
            if o["content_ref"] not in refs:
                return [dict(check=sid+"/"+o["occurrence_id"]+"/retained-content",pass_=False,detail=o["content_ref"])]
    cat=data["catalog"];zones=lookup(cat["zones"],"zone_id");acts=lookup(cat["actuators"],"actuator_id")
    ctrls=lookup(cat["controllers"],"controller_id");areas=lookup(cat["areas"],"area_id")
    ck("synthetic",data["meta"]["synthetic"] and data["meta"]["not_water_policy"] and data["meta"]["case_histories_are_independent"])
    ck("one-controller",len(ctrls)==1)
    ck("no-deprecated-fields",not forbidden_keys(data),str(forbidden_keys(data)))
    ck("seven-cases",set(data["scenarios"])=={f"W{i:02}" for i in range(7)})
    for z in zones.values():ck(z["zone_id"]+"/area",z["area_id"] in areas and "actuator_id" not in z)
    for a in acts.values():ck(a["actuator_id"]+"/authoritative-mapping",a["zone_id"] in zones and a["controller_id"] in ctrls)
    ck("unique-valve-per-zone",len({a["zone_id"] for a in acts.values()})==len(acts))
    for sid,scene in data["scenarios"].items():
        st=scene["state"];prefix=sid+"/";sch=st["schedule"]
        contents=lookup(st["schedule_contents"],"content_ref");bindings=lookup(st["target_bindings"],"binding_ref")
        occs=lookup(st["occurrences"],"occurrence_id");requests=lookup(st["requests"],"request_id")
        sessions=lookup(st["sessions"],"session_id");records=lookup(st["records"],"record_id")
        for table,key in [("schedule_contents","content_ref"),("target_bindings","binding_ref"),("occurrences","occurrence_id"),
                          ("requests","request_id"),("sessions","session_id"),("records","record_id")]:
            ck(prefix+table+"/unique-ids",len(st[table])==len(lookup(st[table],key)))
        ck(prefix+"schedule-zone",sch["zone_id"] in zones)
        ck(prefix+"desired-retained",sch["desired_content_ref"] in contents)
        for key,row in {**contents,**bindings}.items():
            ck(prefix+key+"/immutable-digest",data["meta"]["immutable_content_sha256"].get(key)==digest(row))
        for key,row in contents.items():
            cfg=row["config"];r=cfg["recurrence"];duration=cfg["duration_seconds"]
            ck(prefix+key+"/same-logical-schedule",row["schedule_id"]==sch["schedule_id"])
            ck(prefix+key+"/bounded-config-duration",type(duration)is int and duration>0 and duration%15==0)
            ck(prefix+key+"/enabled-bool",type(cfg["enabled"])is bool)
            ck(prefix+key+"/effectivity",cfg["effective_until"] is None or dt(cfg["effective_until"])>dt(cfg["effective_from"]))
            ck(prefix+key+"/recurrence-kind",r["kind"] in ["once","daily","weekly","interval"])
            for t in [cfg["effective_from"],cfg["effective_until"],r.get("at"),r.get("anchor_at")]:
                if t:ck(prefix+key+"/minute-config",dt(t).second==0 and dt(t).microsecond==0)
            if r["kind"] in ["daily","weekly"]:
                h,m=map(int,r["at_local_time"].split(":"));ck(prefix+key+"/HHmm",0<=h<=23 and 0<=m<=59)
                if r["kind"]=="weekly":
                    days=r["days_of_week"];ck(prefix+key+"/ISOdays",bool(days) and len(set(days))==len(days) and all(type(x)is int and 1<=x<=7 for x in days))
            if r["kind"]=="once":ck(prefix+key+"/once-window",within_effectivity(cfg,r["at"]))
            if r["kind"]=="interval":ck(prefix+key+"/interval",type(r["interval_minutes"])is int and r["interval_minutes"]>0)
        for b in bindings.values():
            a=acts.get(b["actuator_id"]);ck(prefix+b["binding_ref"]+"/target",a and a["zone_id"]==b["zone_id"] and a["controller_id"]==b["controller_id"])
        applied=[r for r in st["records"] if r["event_kind"]=="schedule_applied"]
        for r in applied:ck(prefix+r["record_id"]+"/content-ref",r["content_ref"] in contents and r["subject_ref"]==sch["schedule_id"])
        ck(prefix+"confirmed-source-order",sch["last_known_applied_report_ref"]==latest_confirmation(st["records"],sch["schedule_id"]))
        ck(prefix+"active-cap",active_cap([(sch,st["schedule_contents"],st["records"])]))
        if sch.get("desired_submission"):
            ck(prefix+"submission-evidence",sch["desired_submission"]["state"] in ["pending","rejected","unknown"] and bool(sch["desired_submission"]["observed_at_web"]))
        opportunity_keys=[(contents[o["content_ref"]]["schedule_id"],o["planned_for"]) for o in occs.values() if o["content_ref"] in contents]
        ck(prefix+"no-double-known-opportunity",len(opportunity_keys)==len(set(opportunity_keys)))
        for oid,o in occs.items():
            c=contents.get(o["content_ref"]);ck(prefix+oid+"/retained-content",bool(c))
            ck(prefix+oid+"/disposition",o["evaluation_outcome"] in ["pending","start_requested","skipped","rejected","unknown"])
            related=[r for r in requests.values() if r.get("occurrence_id")==oid]
            ck(prefix+oid+"/one-intent-max",len(related)<=1)
            if o["evaluation_outcome"]=="start_requested":ck(prefix+oid+"/planned-eligible",c and eligible_planned_time(c["config"],o["planned_for"]))
            else:ck(prefix+oid+"/no-action-other-disposition",not related)
            if related:ck(prefix+oid+"/requested-disposition",o["evaluation_outcome"]=="start_requested")
        for rid,r in requests.items():
            b=bindings.get(r["binding_ref"]);ck(prefix+rid+"/binding",b is not None)
            ck(prefix+rid+"/source-kind",r["kind"] in ["START","STOP"] and r["trigger_source"] in ["schedule","local_button","remote"])
            ck(prefix+rid+"/status",r["status"] in ["requested","received","accepted","rejected","expired","outcome_unknown"])
            rp=request_projection(st,r)
            created=record_of(st,"request_created",rid);ck(prefix+rid+"/created-provenance",created is not None)
            if r["kind"]=="START":
                duration=rp["planned_duration_seconds"];ck(prefix+rid+"/duration",type(duration)is int and duration>0 and duration%15==0)
                if r["trigger_source"]=="schedule":
                    o=occs.get(r.get("occurrence_id"));ck(prefix+rid+"/scheduled-parent",o and contents[o["content_ref"]]["schedule_id"]==sch["schedule_id"] and b["zone_id"]==sch["zone_id"])
                    ck(prefix+rid+"/no-duplicate-duration","duration_seconds" not in r)
                else:ck(prefix+rid+"/no-fake-occurrence","occurrence_id" not in r)
                if r["trigger_source"]=="remote":
                    confirm=r.get("confirmed_context")
                    ck(prefix+rid+"/confirmation",confirm and confirm["binding_ref"]==r["binding_ref"] and confirm["duration_seconds"]==duration)
                    ck(prefix+rid+"/finite-validity",r.get("valid_until") is not None)
            else:ck(prefix+rid+"/STOP-minimal","duration_seconds" not in r and "occurrence_id" not in r)
            if r.get("valid_until") and created and created["time"]["at"]:ck(prefix+rid+"/valid-until",dt(r["valid_until"])>dt(created["time"]["at"]))
            linked=[s for s in sessions.values() if s["request_id"]==rid]
            ck(prefix+rid+"/one-session-max",len(linked)<=1)
            if r["kind"]=="STOP" or r["status"] in ["rejected","expired"]:ck(prefix+rid+"/no-phantom-session",not linked)
        for ssid,s in sessions.items():
            r=requests.get(s["request_id"]);ck(prefix+ssid+"/START-parent",r and r["kind"]=="START" and r["status"] not in ["rejected","expired"])
            if r is None or r["kind"]!="START":continue
            ck(prefix+ssid+"/minimal-own-facts",set(s)=={"session_id","request_id","reported_output_duration_seconds","result","end_reason"})
            start=record_of(st,"session_started",ssid);end=record_of(st,"session_ended",ssid)
            ck(prefix+ssid+"/start-evidence",start is not None)
            rp=request_projection(st,r);planned=rp["planned_duration_seconds"]
            if rp["occurrence_id"]:
                o=occs[rp["occurrence_id"]];cfg=contents[o["content_ref"]]["config"]
                if start and start["time"]["at"]:
                    ck(prefix+ssid+"/start-within-historical-window",within_effectivity(cfg,start["time"]["at"]))
                    ck(prefix+ssid+"/not-before-planned",dt(start["time"]["at"])>=dt(o["planned_for"]))
            if r.get("valid_until") and start and start["time"]["at"]:ck(prefix+ssid+"/start-before-expiry",dt(start["time"]["at"])<dt(r["valid_until"]))
            ck(prefix+ssid+"/reported-result-only",s["result"] in ["running","completed_reported","stopped_reported","fault_reported","outcome_unknown"])
            duration=s["reported_output_duration_seconds"]
            if end is None:ck(prefix+ssid+"/no-invented-terminal",duration is None and s["result"] in ["running","outcome_unknown"])
            else:
                ck(prefix+ssid+"/terminal-result",s["result"] in ["completed_reported","stopped_reported","fault_reported","outcome_unknown"])
                if duration is not None:
                    ck(prefix+ssid+"/bounded-reported-duration",isinstance(duration,(float,int)) and 0<=duration<=planned)
                    if start["time"]["at"] and end["time"]["at"] and start["time"]["quality"]==end["time"]["quality"]=="known":
                        ck(prefix+ssid+"/known-time-arithmetic",(dt(end["time"]["at"])-dt(start["time"]["at"])).total_seconds()==duration)
                if s["result"]=="completed_reported":ck(prefix+ssid+"/completed-planned",duration==planned)
        tables={"request":requests,"session":sessions,"occurrence":occs,"schedule":{sch["schedule_id"]:sch},"controller":ctrls}
        for rec in records.values():
            ck(prefix+rec["record_id"]+"/origin",rec["origin_ref"] in set(ctrls)|{"microSched"})
            ck(prefix+rec["record_id"]+"/subject",rec["subject_kind"] in tables and rec["subject_ref"] in tables.get(rec["subject_kind"],{}))
            tm=rec["time"]
            ck(prefix+rec["record_id"]+"/time-quality",tm["quality"] in ["known","untrusted","unknown"] and (tm["at"] is not None or tm["quality"]=="unknown"))
            if tm["at"]:dt(tm["at"])
            if rec["received_at_web"]:dt(rec["received_at_web"])
            if rec["event_kind"] in ["schedule_applied","controller_status"]:ck(prefix+rec["record_id"]+"/report-not-apply-time",tm["meaning"]=="reported_at")
        for ssid in sessions:
            for k in ["session_started","session_ended"]:ck(prefix+ssid+"/"+k+"-unique",len([r for r in records.values() if r["event_kind"]==k and r["subject_ref"]==ssid])<=1)
        status=records.get(st["controller_report_ref"])
        ck(prefix+"status-report",status and status["event_kind"]=="controller_status" and type(status["clock_usable_for_open"])is bool)
        active=status["active_session_id"]
        ck(prefix+"active-session",active is None or (active in sessions and sessions[active]["result"] in ["running","outcome_unknown"]))
        for delivery in st["ingest_view"]["deliveries"]:
            row=records.get(delivery["record_id"]);ck(prefix+"delivery/ref",row is not None)
            if row and row["time"]["at"] and row["time"]["quality"]=="known":ck(prefix+"delivery/after-event",dt(delivery["received_at_web"])>=dt(row["time"]["at"]))
            if delivery["disposition"]=="inserted":ck(prefix+"delivery/first-receipt",row["received_at_web"]==delivery["received_at_web"])
        for name,b in st["branches"].items():
            r=b["request"];ck(prefix+name+"/independent-rejection",b["independent_example"] and r["status"]=="rejected" and not b["new_sessions"] and r["binding_ref"] in bindings)
            if r["trigger_source"]=="remote":ck(prefix+name+"/confirmed",r["confirmed_context"]["binding_ref"]==r["binding_ref"])
        if any(r["status"]=="outcome_unknown" for r in requests.values()):
            ck(prefix+"unknown-block",st["safety_view"]["new_start_allowed"] is False and "OUTCOME_UNKNOWN" in st["safety_view"]["block_reasons"])
        expected=derive_state(st)
        ck(prefix+"projection-recomputed",st.get("projections")==expected)
        dt(scene["observed_at"])
        for ev in scene["timeline"]:
            ck(prefix+"timeline/mapped-lane",ev["lane"] in ["web","device","result"])
            ck(prefix+"timeline/not-future",dt(ev["at"])<=dt(scene["observed_at"]))
    s=data["scenarios"]
    w=lambda i:s[i]["state"]
    ck("W01/before-first-occurrence",not w("W01")["occurrences"] and not w("W01")["requests"] and not w("W01")["sessions"])
    ck("W01/desired-v2-applied-v1",w("W01")["schedule"]["desired_content_ref"]=="SC-A-v2" and derive_state(w("W01"))["schedule"]["last_known_applied"]["content_ref"]=="SC-A-v1")
    ck("W03/45-versus17",derive_state(w("W03"))["sessions"]["SES-DEMO-BUTTON"]["planned_duration_seconds"]==45 and w("W03")["sessions"][0]["reported_output_duration_seconds"]==17)
    ck("W04/no-fake-success",not w("W04")["sessions"] and w("W04")["safety_view"]["new_start_allowed"] is False)
    ck("W04/old-closed-current-unknown",derive_state(w("W04"))["controller"]["output_reported_state"]=="closed" and w("W04")["dashboard_view"]["current_output_assessment"]=="unknown")
    ck("W05/one-history-session",w("W05")["ingest_view"]["history_session_ids"]==["SES-DEMO-SCHED"] and w("W05")["ingest_view"]["duplicate_record_ids"]==["REC-DEMO-END"])
    ck("W06/v3-paused-v1-running45",derive_state(w("W06"))["schedule"]["last_known_applied"]["content_ref"]=="SC-A-v3" and
       derive_state(w("W06"))["schedule"]["applied_enabled"] is False and derive_state(w("W06"))["sessions"]["SES-DEMO-RUNNING"]["content_ref"]=="SC-A-v1" and
       derive_state(w("W06"))["sessions"]["SES-DEMO-RUNNING"]["planned_duration_seconds"]==45)
    if views is not None:
        ck("W00/eight-concepts",len(next(v for v in views["scenes"] if v["id"]=="W00")["cards"])==8)
        ck("seven-render-scenes",len(views["scenes"])==7 and {v["id"] for v in views["scenes"]}==set(s))
        expected=build_views(data)
        ck("views-single-source",views==expected)
        for v in views["scenes"]:
            ck(v["id"]+"/limits",bool(v["limitations"]) and bool(v["takeaways"]))
            ids={c["record_id"] for c in v["cards"]}
            for rel in v["relations"]:ck(v["id"]+"/relation",rel["from"] in ids and rel["to"] in ids)
            for card in v["cards"]:
                for f in card["fields"]:
                    try:match=pointer(data,f["source_path"])==f["value"]
                    except (KeyError,IndexError,TypeError):match=False
                    ck(v["id"]+"/card-provenance",match,f["source_path"])
    return [dict(check=x["check"],pass_=x["pass_"],detail=x["detail"]) for x in checks]

def passed(rows):return all(c["pass_"] for c in rows)
def refresh(data):
    for scene in data["scenarios"].values():scene["state"]["projections"]=derive_state(scene["state"])
def main():
    data=json.loads((ROOT/"synthetic-data.json").read_text(encoding="utf-8"));views=json.loads((ROOT/"views.json").read_text(encoding="utf-8"))
    checks=validate(data,views);negative=[]
    def mutation(name,fn):
        bad=deepcopy(data)
        try:
            fn(bad)
            # Refresh derived copies: invalid semantics must fail even with fresh presentation.
            try:refresh(bad)
            except KeyError:pass # validate's explicit retained-reference guard handles this
            rows=validate(bad,None);failures=[c["check"] for c in rows if not c["pass_"]]
            rejected=bool(failures)
        except (KeyError,ValueError,TypeError,StopIteration) as error:
            rejected=True;failures=[type(error).__name__+": "+str(error)]
        negative.append(dict(case=name,invalid_variant_rejected=rejected,failures=failures,mutation="in-memory; canonical untouched"))
    W=lambda d,i:d["scenarios"][i]["state"]
    mutation("dangling-historical-ref",lambda d:W(d,"W06")["occurrences"][0].update(content_ref="MISSING"))
    mutation("mutated-historical-content",lambda d:W(d,"W06")["schedule_contents"][0]["config"].update(duration_seconds=60))
    mutation("retarget-binding",lambda d:W(d,"W02")["target_bindings"][0].update(zone_id="ZONE-WRONG"))
    mutation("retime-running-via-latest-content",lambda d:W(d,"W06")["occurrences"][0].update(content_ref="SC-A-v3"))
    mutation("invented-physical-success",lambda d:W(d,"W03")["sessions"][0].update(result="water_verified"))
    mutation("deprecated-proof-field",lambda d:W(d,"W03")["sessions"][0].update(physical_water_evidence="verified"))
    mutation("rejected-request-with-session",lambda d:W(d,"W02")["requests"][0].update(status="rejected"))
    mutation("STOP-phantom-session",lambda d:W(d,"W03")["sessions"][0].update(request_id="REQ-DEMO-STOP"))
    mutation("same-id-conflicting-origin",lambda d:W(d,"W05")["records"].append(dict(deepcopy(W(d,"W05")["records"][-1]),event_kind="conflicting")))
    mutation("duplicate-session-replay",lambda d:W(d,"W02")["sessions"].append(dict(deepcopy(W(d,"W02")["sessions"][0]),session_id="SES-DUP")))
    mutation("new-id-same-known-opportunity-replay",lambda d:W(d,"W02")["occurrences"].append(dict(deepcopy(W(d,"W02")["occurrences"][0]),occurrence_id="OCC-REPLAY")))
    for outcome in ["skipped","rejected","pending","unknown"]:
        mutation(outcome+"-with-linked-intent",lambda d,o=outcome:W(d,"W00")["occurrences"][0].update(evaluation_outcome=o))
    mutation("expired-before-action",lambda d:W(d,"W02")["requests"][0].update(valid_until="2026-11-01T06:59:59+07:00"))
    def expire_content(d):
        row=W(d,"W00")["schedule_contents"][0];row["config"]["effective_until"]=ts("06:00:00")
        d["meta"]["immutable_content_sha256"][row["content_ref"]]=digest(row)
        # Update all appearances of the same content: detect eligibility, not just hash.
        for scene in d["scenarios"].values():
            for c in scene["state"]["schedule_contents"]:
                if c["content_ref"]==row["content_ref"]:c["config"]["effective_until"]=ts("06:00:00")
    from prepare_data import ts
    mutation("session-after-historical-window",expire_content)
    mutation("stale-applied-confirmation-rollback",lambda d:W(d,"W06")["schedule"].update(last_known_applied_report_ref="REP-APPLIED-1"))
    mutation("expiry-clears-unknown-block",lambda d:W(d,"W04")["safety_view"].update(new_start_allowed=True,block_reasons=[]))
    badview=deepcopy(views);badview["scenes"][1]["cards"][0]["fields"][0]["value"]="BAD"
    badview_checks=validate(data,badview)
    negative.append(dict(case="card-value-not-source",invalid_variant_rejected=not passed(badview_checks),failures=[c["check"] for c in badview_checks if not c["pass_"]],mutation="in-memory"))
    st=W(data,"W02");second=deepcopy(st["schedule"]);second.update(schedule_id="SCHED-SECOND",desired_content_ref="SC-SECOND-v1",last_known_applied_report_ref="REP-SECOND")
    other_content=deepcopy(st["schedule_contents"][0]);other_content.update(schedule_id="SCHED-SECOND",content_ref="SC-SECOND-v1")
    other_report=deepcopy(st["records"][0]);other_report.update(record_id="REP-SECOND",subject_ref="SCHED-SECOND",content_ref="SC-SECOND-v1")
    negative.append(dict(case="two-active-logical-schedules-one-zone",invalid_variant_rejected=not active_cap([(st["schedule"],st["schedule_contents"],st["records"]),(second,[other_content],[other_report])]),failures=["active-cap"],mutation="in-memory"))
    positive=[]
    def pos(name,fn):
        variant=deepcopy(data)
        try:fn(variant);refresh(variant);rows=validate(variant,None);ok=passed(rows);failures=[r["check"] for r in rows if not r["pass_"]]
        except (KeyError,ValueError,TypeError) as error:ok=False;failures=[str(error)]
        positive.append(dict(case=name,pass_=ok,failures=failures))
    def unknown_time(d):
        record_of(W(d,"W02"),"session_started","SES-DEMO-SCHED")["time"].update(at=None,quality="unknown")
    pos("historical-time-unknown-current-clock-usable",unknown_time)
    def expired_skip(d):
        st=W(d,"W00")
        st["occurrences"][0].update(planned_for="2026-10-31T07:00:00+07:00",evaluation_outcome="skipped",reason="OUTSIDE_EFFECTIVITY")
        st["requests"]=[];st["sessions"]=[]
        st["records"]=[r for r in st["records"] if r["subject_kind"] in ["schedule","controller","occurrence"]]
    pos("out-of-window-skipped-no-action",expired_skip)
    # Later arrival of source-order1 does not replace order3; duplicates preserve
    # source time and first receipt. These are fixture operations, not production sync.
    st=W(data,"W06");delayed=list(reversed(st["records"]))
    positive.append(dict(case="delayed-order-not-arrival-order",pass_=latest_confirmation(delayed,st["schedule"]["schedule_id"])=="REP-APPLIED-3"))
    st=W(data,"W05");rec=next(r for r in st["records"] if r["record_id"]=="REC-DEMO-END");retry=deepcopy(rec);retry["received_at_web"]=ts("07:10:05")
    positive.append(dict(case="dedup-retry-no-action",pass_=ingest_known(st["records"],retry)=="duplicate_same_payload" and len(st["sessions"])==1 and rec["received_at_web"]==ts("07:10:02")))
    retry["time"]["at"]=ts("07:00:46")
    positive.append(dict(case="same-id-payload-conflict-no-overwrite",pass_=ingest_known(st["records"],retry)=="conflict" and rec["time"]["at"]==ts("07:00:45")))
    patterns=json.loads((ROOT/"pattern-examples.json").read_text(encoding="utf-8"));pattern_checks=[]
    for example in patterns["examples"]:
        for label,key in [(True,"valid_planned_for"),(False,"invalid_planned_for")]:
            for t in example[key]:pattern_checks.append(dict(example=example["example_id"],time=t,pass_=eligible_planned_time(example["config"],t)==label))
    receipt=dict(scope="Public baseline fixture/projection semantics only; builder, not independent review",checked_at_utc=datetime.now(timezone.utc).isoformat(),
        checks=checks,passed=sum(c["pass_"] for c in checks),total=len(checks),negative_controls=negative,
        negative_controls_passed=sum(c["invalid_variant_rejected"] for c in negative),positive_controls=positive,pattern_checks=pattern_checks,
        restored_canonical_pass=passed(validate(data,views)),runtime_device_water="NOT_RUN",production_algorithm="NOT_IMPLEMENTED")
    ok=passed(checks) and all(n["invalid_variant_rejected"] for n in negative) and passed(positive) and passed(pattern_checks)
    print(json.dumps(dict(status="PASS" if ok else "FAIL",passed=receipt["passed"],total=len(checks),
        negative=f"{receipt['negative_controls_passed']}/{len(negative)}",positive=f"{sum(c['pass_'] for c in positive)}/{len(positive)}",
        patterns=f"{sum(c['pass_'] for c in pattern_checks)}/{len(pattern_checks)}")))
    if not ok:
        print(json.dumps(dict(failed_checks=[c for c in checks if not c["pass_"]],failed_negative=[c for c in negative if not c["invalid_variant_rejected"]],
            failed_positive=[c for c in positive if not c["pass_"]]),ensure_ascii=False,indent=2));raise SystemExit(1)
if __name__=="__main__":main()
