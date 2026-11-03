"""Public baseline synthetic information model. Imports are pure; no product/device IO."""
from pathlib import Path
from copy import deepcopy
from datetime import datetime, timedelta
import json
import hashlib

ROOT = Path(__file__).resolve().parent
SCENES = json.loads(r'''{
  "W00": {
    "id": "W00",
    "title": "Một phiên mẫu nối đủ 8 khái niệm",
    "question": "Một lần tưới tạo ra những đối tượng và dữ liệu nào?",
    "summary": "Instance map sau một phiên local; ID mẫu liên kết xuyên lịch → occurrence → request → session.",
    "uc_refs": [
      "DM01–DM08"
    ],
    "observed_at": "2026-11-01T07:00:46+07:00",
    "viewpoint": "domain-instance-map",
    "timeline": [],
    "takeaways": [
      "Một ESP32, một Zone mẫu, một lịch hoạt động; không có registry chậu hoặc group.",
      "Session kết nối START gốc; van là đối tượng vật lý, report không chứng minh flow."
    ],
    "limitations": [
      "Dữ liệu synthetic — không phải log thiết bị hay bằng chứng tưới đủ nước."
    ]
  },
  "W01": {
    "id": "W01",
    "title": "Web đổi lịch, ESP32 chưa xác nhận",
    "question": "Đã lưu v2 có nghĩa thiết bị dùng v2 chưa?",
    "summary": "Cùng SCHED-DEMO-A: desired v2 lúc 08:00/60 giây, last-known applied v1 lúc 07:00/45 giây.",
    "uc_refs": [
      "UC01",
      "DR02",
      "DR12"
    ],
    "observed_at": "2026-11-01T06:50:00+07:00",
    "viewpoint": "web-last-known",
    "timeline": [
      {
        "id": "applied",
        "at": "2026-11-01T06:40:00+07:00",
        "lane": "device",
        "title": "Báo applied v1",
        "detail": "07:00 • 45 giây; giữ cả nội dung và version",
        "kind": "event",
        "evidence": "controller_reported"
      },
      {
        "id": "save",
        "at": "2026-11-01T06:45:00+07:00",
        "lane": "web",
        "title": "Web lưu desired v2",
        "detail": "08:00 • 60 giây; không phải lịch active thứ hai",
        "kind": "change",
        "evidence": "web_saved"
      },
      {
        "id": "pending",
        "at": "2026-11-01T06:50:00+07:00",
        "lane": "result",
        "title": "application_status = pending",
        "detail": "occurrences = []; requests = []; sessions = []",
        "kind": "event",
        "evidence": "synthetic_snapshot"
      }
    ],
    "takeaways": [
      "Saved ≠ applied. Applied có nghĩa cả trước occurrence đầu tiên.",
      "Last-known report có tuổi; mất ack không chứng minh đã hoặc chưa áp dụng."
    ],
    "limitations": [
      "Dữ liệu synthetic — không phải log thiết bị hay bằng chứng tưới đủ nước."
    ]
  },
  "W02": {
    "id": "W02",
    "title": "Lịch local chạy khi web mất kết nối",
    "question": "Đã đến giờ, hệ thống ghi nhận lần tưới như thế nào?",
    "summary": "Giả định local đủ điều kiện: một occurrence, một START, một session 45 giây được báo.",
    "uc_refs": [
      "UC02",
      "FR02",
      "FR04",
      "FR05"
    ],
    "observed_at": "2026-11-01T07:00:46+07:00",
    "viewpoint": "controller-local",
    "timeline": [
      {
        "id": "due",
        "at": "2026-11-01T07:00:00+07:00",
        "lane": "device",
        "title": "Occurrence đến hạn",
        "detail": "OCC-DEMO-0700 • applied v1 • planned 45 giây",
        "kind": "event",
        "evidence": "controller_reported"
      },
      {
        "id": "start",
        "at": "2026-11-01T07:00:00+07:00",
        "lane": "result",
        "title": "START → phiên đã bắt đầu",
        "detail": "REQ-DEMO-SCHED → SES-DEMO-SCHED; reported start 07:00:00",
        "kind": "action",
        "evidence": "controller_reported"
      },
      {
        "id": "close",
        "at": "2026-11-01T07:00:45+07:00",
        "lane": "device",
        "title": "Đến deadline: yêu cầu đóng",
        "detail": "reported_output_duration_seconds = 45",
        "kind": "action",
        "evidence": "controller_reported"
      },
      {
        "id": "end",
        "at": "2026-11-01T07:00:45+07:00",
        "lane": "result",
        "title": "Có kết quả đầu ra được báo",
        "detail": "completed_reported; kết quả Controller báo, chưa đo nước",
        "kind": "event",
        "evidence": "controller_reported"
      }
    ],
    "takeaways": [
      "Không cần cloud xin phép mỗi occurrence.",
      "Chỉ có session sau execution-start evidence; completed_reported không phải physical-water proof."
    ],
    "limitations": [
      "Đây là góc nhìn record tại controller mẫu; website offline chưa nhìn thấy các record này.",
      "45 giây chỉ là giá trị synthetic, không là water policy."
    ]
  },
  "W03": {
    "id": "W03",
    "title": "Bấm START, rồi STOP sau 17 giây",
    "question": "Duration đặt 45 giây nhưng dừng sớm thì dữ liệu ra sao?",
    "summary": "Nút tạo START riêng; STOP không tạo phiên mới. Planned 45 giây, reported output 17 giây.",
    "uc_refs": [
      "UC03",
      "LC01",
      "FR03"
    ],
    "observed_at": "2026-11-01T07:15:17+07:00",
    "viewpoint": "controller-local",
    "timeline": [
      {
        "id": "button",
        "at": "2026-11-01T07:15:00+07:00",
        "lane": "device",
        "title": "Nhận START tại nút",
        "detail": "trigger_source = local_button; không ghi danh tính người bấm",
        "kind": "action",
        "evidence": "controller_reported"
      },
      {
        "id": "started",
        "at": "2026-11-01T07:15:00+07:00",
        "lane": "result",
        "title": "Một session bắt đầu",
        "detail": "SES-DEMO-BUTTON • planned_duration_seconds = 45",
        "kind": "event",
        "evidence": "controller_reported"
      },
      {
        "id": "stop",
        "at": "2026-11-01T07:15:17+07:00",
        "lane": "device",
        "title": "STOP yêu cầu CLOSE",
        "detail": "REQ-DEMO-STOP; không cần OPEN gates",
        "kind": "action",
        "evidence": "controller_reported"
      },
      {
        "id": "stopped",
        "at": "2026-11-01T07:15:17+07:00",
        "lane": "result",
        "title": "stopped_reported",
        "detail": "reported duration = 17; chưa đo lượng nước",
        "kind": "event",
        "evidence": "controller_reported"
      }
    ],
    "takeaways": [
      "Độ chia cấu hình 15 giây không bắt duration thực phải là bội 15.",
      "Nút local không cần cloud; không gán giả danh tính người bấm."
    ],
    "limitations": [
      "17 giây là thời lượng output được báo trong fixture, không chứng minh lượng nước."
    ]
  },
  "W04": {
    "id": "W04",
    "title": "Remote đã được nhận, kết quả chưa rõ",
    "question": "Có receipt có đồng nghĩa đã tưới không?",
    "summary": "Request có receipt nhận/giao thực thi, thiếu start/terminal evidence: không tạo session đã tưới giả.",
    "uc_refs": [
      "UC04",
      "SAFE08",
      "DR09"
    ],
    "observed_at": "2026-11-01T07:01:00+07:00",
    "viewpoint": "web-received-evidence",
    "timeline": [
      {
        "id": "confirm",
        "at": "2026-11-01T06:59:50+07:00",
        "lane": "web",
        "title": "Preview + confirm",
        "detail": "ZONE-DEMO-A • duration 45 giây • cfg-demo-1",
        "kind": "action",
        "evidence": "web_saved"
      },
      {
        "id": "received",
        "at": "2026-11-01T07:00:00+07:00",
        "lane": "device",
        "title": "Controller nhận request",
        "detail": "REC-REMOTE-RX; chưa phải started",
        "kind": "event",
        "evidence": "controller_reported"
      },
      {
        "id": "allowed",
        "at": "2026-11-01T07:00:01+07:00",
        "lane": "result",
        "title": "Có record giao thực thi",
        "detail": "execution_authorized; không phải completed",
        "kind": "event",
        "evidence": "controller_reported"
      },
      {
        "id": "unknown",
        "at": "2026-11-01T07:01:00+07:00",
        "lane": "result",
        "title": "outcome_unknown",
        "detail": "sessions = []; new_start_allowed = false",
        "kind": "alert",
        "evidence": "unknown"
      }
    ],
    "takeaways": [
      "Accepted/authorized không tự tạo session khi chưa có start evidence.",
      "Không retry action mới; unknown giữ chặn OPEN mọi nguồn đến recovery/xác minh."
    ],
    "limitations": [
      "sessions=[] nghĩa chưa có session được xác nhận trong evidence, không chứng minh chưa có tác động vật lý.",
      "Readiness trong fixture là expected design state, không đo được arbiter/valve thật."
    ]
  },
  "W05": {
    "id": "W05",
    "title": "Sync trễ và gửi lại record",
    "question": "Hai lần nhận cùng record có tạo hai phiên không?",
    "summary": "Record có event-time khác received-time; cùng REC-DEMO-END gửi lại vẫn chỉ một session lịch sử.",
    "uc_refs": [
      "UC05",
      "FR05",
      "FR07"
    ],
    "observed_at": "2026-11-01T07:10:05+07:00",
    "viewpoint": "web-received-data",
    "timeline": [
      {
        "id": "happened",
        "at": "2026-11-01T07:00:45+07:00",
        "lane": "device",
        "title": "Phiên đã kết thúc ở local",
        "detail": "REC-DEMO-END • occurred_at_reported = 07:00:45",
        "kind": "event",
        "evidence": "controller_reported"
      },
      {
        "id": "upload",
        "at": "2026-11-01T07:10:00+07:00",
        "lane": "web",
        "title": "Kết nối lại, nhận backlog",
        "detail": "Web bắt đầu nhận record cũ, không phát START mới",
        "kind": "recovery",
        "evidence": "web_received"
      },
      {
        "id": "endrx",
        "at": "2026-11-01T07:10:02+07:00",
        "lane": "result",
        "title": "Nhận REC-DEMO-END",
        "detail": "Một session SES-DEMO-SCHED trong history",
        "kind": "event",
        "evidence": "web_received"
      },
      {
        "id": "duplicate",
        "at": "2026-11-01T07:10:05+07:00",
        "lane": "result",
        "title": "Cùng ID + cùng payload gửi lại",
        "detail": "duplicate_same_payload; không nhân phiên, không replay",
        "kind": "event",
        "evidence": "web_received"
      }
    ],
    "takeaways": [
      "Event-time không bị thay bằng thời điểm upload.",
      "Last execution là reported 07:00; next planned lấy applied plan, không lấy một request đã gửi."
    ],
    "limitations": [
      "Dữ liệu synthetic — không phải log thiết bị hay bằng chứng tưới đủ nước."
    ]
  },
  "W06": {
    "id": "W06",
    "title": "Pause lịch không dừng/đổi phiên đang chạy",
    "question": "Applied v3 đã tạm dừng, sao session vẫn giữ 45 giây?",
    "summary": "Phiên bind v1/45 giây; applied v3 disabled chỉ ảnh hưởng lần tương lai. STOP/safety vẫn có thể dừng phiên.",
    "uc_refs": [
      "P01",
      "LC01–03",
      "SAFE05",
      "SAFE07"
    ],
    "observed_at": "2026-11-01T07:00:20+07:00",
    "viewpoint": "reported-running-session",
    "timeline": [
      {
        "id": "bind",
        "at": "2026-11-01T07:00:00+07:00",
        "lane": "device",
        "title": "Session bind v1",
        "detail": "SES-DEMO-RUNNING • planned_duration_seconds = 45",
        "kind": "action",
        "evidence": "controller_reported"
      },
      {
        "id": "edit",
        "at": "2026-11-01T07:00:10+07:00",
        "lane": "web",
        "title": "Sửa mong muốn v2",
        "detail": "Đổi giờ 08:00, duration 60 giây",
        "kind": "change",
        "evidence": "web_saved"
      },
      {
        "id": "apply2",
        "at": "2026-11-01T07:00:12+07:00",
        "lane": "device",
        "title": "Báo áp dụng v2",
        "detail": "Không retime session v1 đang chạy",
        "kind": "event",
        "evidence": "controller_reported"
      },
      {
        "id": "pause",
        "at": "2026-11-01T07:00:15+07:00",
        "lane": "web",
        "title": "Pause bằng desired v3",
        "detail": "enabled = false; không phải STOP session",
        "kind": "change",
        "evidence": "web_saved"
      },
      {
        "id": "applied3",
        "at": "2026-11-01T07:00:16+07:00",
        "lane": "device",
        "title": "Báo applied v3",
        "detail": "applied SC-A-v3: enabled = false; phiên cũ giữ SC-A-v1",
        "kind": "event",
        "evidence": "controller_reported"
      },
      {
        "id": "running",
        "at": "2026-11-01T07:00:20+07:00",
        "lane": "result",
        "title": "Phiên đang được báo chạy",
        "detail": "ended_at_reported = null; output duration chưa có kết luận",
        "kind": "event",
        "evidence": "controller_reported"
      }
    ],
    "takeaways": [
      "Sửa/pause không restart hay kéo/rút duration đã bind; dừng phiên dùng STOP/safety.",
      "Ba nhánh từ chối là ví dụ độc lập: request rejected không tạo session mới, không queue muộn."
    ],
    "limitations": [
      "Các nhánh BUSY/CLOCK_UNTRUSTED/CUMULATIVE_LIMIT là ví dụ độc lập, không nối thành timeline chính.",
      "Không chọn numeric cumulative budget hoặc đo khả năng safety arbiter trong fixture."
    ]
  }
}''')

def ts(value): return f"2026-11-01T{value}+07:00"
def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(',', ':')).encode()).hexdigest()
def config(hour="07:00", duration=45, enabled=True):
    return dict(enabled=enabled, effective_from=ts("00:00:00"), effective_until=None,
                recurrence=dict(kind="daily", at_local_time=hour), duration_seconds=duration)
def content(n):
    cfg = config() if n == 1 else config("08:00", 60, n != 3)
    return dict(content_ref=f"SC-A-v{n}", schedule_id="SCHED-DEMO-A", config=cfg)
def report(rid, kind, subject_kind, subject_ref, at, received=None, origin="CTRL-DEMO-01", quality="known", **payload):
    meaning = "reported_at" if kind in ["schedule_applied", "controller_status"] else ("created_at_web" if origin == "microSched" else "occurred_at_reported")
    return dict(record_id=rid, origin_ref=origin, event_kind=kind, subject_kind=subject_kind,
                subject_ref=subject_ref, time=dict(at=ts(at) if at else None, meaning=meaning, quality=quality),
                received_at_web=ts(received) if received else None, **payload)
def req(rid, source="schedule", kind="START", status="accepted", reason=None):
    r=dict(request_id=rid, kind=kind, trigger_source=source, binding_ref="BIND-A-1", status=status, reason=reason)
    if kind == "START":
        if source == "schedule": r["occurrence_id"]="OCC-DEMO-0700"
        else: r["duration_seconds"]=45
        r["valid_until"]=ts("07:00:30") if source == "remote" else None
        if source == "remote":
            r["confirmed_context"]=dict(binding_ref="BIND-A-1", duration_seconds=45, confirmed_at_web=ts("06:59:50"))
    return r
def occurrence():
    return dict(occurrence_id="OCC-DEMO-0700", content_ref="SC-A-v1", planned_for=ts("07:00:00"),
                evaluation_outcome="start_requested", reason=None)
def sess(sid, rid, duration=45, result="completed_reported", reason="planned_end"):
    return dict(session_id=sid, request_id=rid, reported_output_duration_seconds=duration, result=result, end_reason=reason)
def base():
    return dict(schedule=dict(schedule_id="SCHED-DEMO-A", zone_id="ZONE-DEMO-A", desired_content_ref="SC-A-v1",
                  desired_saved_at_web="2026-10-31T21:00:00+07:00", last_known_applied_report_ref="REP-APPLIED-1"),
                schedule_contents=[content(1)],
                target_bindings=[dict(binding_ref="BIND-A-1", zone_id="ZONE-DEMO-A", actuator_id="VALVE-DEMO-A", controller_id="CTRL-DEMO-01")],
                occurrences=[], requests=[], sessions=[],
                records=[report("REP-APPLIED-1","schedule_applied","schedule","SCHED-DEMO-A","06:40:00","06:40:01",content_ref="SC-A-v1",source_order=1),
                         report("REP-STATUS","controller_status","controller","CTRL-DEMO-01","06:40:00","06:40:01",
                                output_reported_state="closed",active_session_id=None,clock_usable_for_open=True)],
                controller_report_ref="REP-STATUS",
                safety_view=dict(new_start_allowed=True, block_reasons=[], basis="Expected synthetic local conditions; not runtime proof"),
                ingest_view=dict(deliveries=[], unique_record_ids=[], duplicate_record_ids=[], history_session_ids=[]),
                dashboard_view=dict(as_of=ts("06:50:00"),last_execution_at_reported=None,next_planned_at=ts("07:00:00"),
                    next_plan_basis="last_known_applied_SC-A-v1",freshness_note="Báo cáo có mốc, không cam kết realtime"),
                connectivity_view="online", branches={})
def lookup(rows, key): return {r[key]: r for r in rows}
def record_of(st, kind, ref):
    return next((r for r in st["records"] if r["event_kind"]==kind and r["subject_ref"]==ref),None)
def request_projection(st,r):
    binding=lookup(st["target_bindings"],"binding_ref")[r["binding_ref"]]
    c=None
    if r.get("occurrence_id"):
        o=lookup(st["occurrences"],"occurrence_id")[r["occurrence_id"]]
        c=lookup(st["schedule_contents"],"content_ref")[o["content_ref"]]
    created=record_of(st,"request_created",r["request_id"])
    return dict(**binding,trigger_source=r["trigger_source"],occurrence_id=r.get("occurrence_id"),content_ref=c["content_ref"] if c else None,
                planned_duration_seconds=c["config"]["duration_seconds"] if c else r.get("duration_seconds"),
                requested_at=created["time"]["at"] if created else None)
def derive_state(st):
    contents=lookup(st["schedule_contents"],"content_ref")
    reports=lookup(st["records"],"record_id")
    sch=st["schedule"];ap=reports.get(sch["last_known_applied_report_ref"])
    desired=contents[sch["desired_content_ref"]];applied=contents[ap["content_ref"]] if ap else None
    submission=sch.get("desired_submission")
    status="confirmed" if applied and applied["content_ref"]==desired["content_ref"] else (submission["state"] if submission else "unknown")
    p=dict(schedule=dict(desired=deepcopy(desired),last_known_applied=deepcopy(applied),
                         application_status=status,applied_enabled=applied["config"]["enabled"] if applied else None,
                         last_known_reported_at=ap["time"]["at"] if ap else None,
                         first_received_at_web=ap["received_at_web"] if ap else None),
           requests={}, sessions={})
    for r in st["requests"]: p["requests"][r["request_id"]]=request_projection(st,r)
    for s in st["sessions"]:
        r=p["requests"][s["request_id"]];start=record_of(st,"session_started",s["session_id"]);end=record_of(st,"session_ended",s["session_id"])
        p["sessions"][s["session_id"]]=dict(**r,started_at_reported=start["time"]["at"] if start else None,
            ended_at_reported=end["time"]["at"] if end else None,reported_output_duration_seconds=s["reported_output_duration_seconds"],result=s["result"])
    status_report=reports[st["controller_report_ref"]]
    p["controller"]=deepcopy(status_report)
    return p
def set_status(st, at, received=None, **kwargs):
    r=next(r for r in st["records"] if r["record_id"]=="REP-STATUS")
    r.update(kwargs);r["time"]["at"]=ts(at);r["received_at_web"]=ts(received) if received else None
def add_created(st,r,at,quality="known"):
    st["records"].append(report("CREATED-"+r["request_id"],"request_created","request",r["request_id"],at,
        received=at if r["trigger_source"]=="remote" else None,origin="microSched" if r["trigger_source"]=="remote" else "CTRL-DEMO-01",quality=quality))
def normal_state():
    st=base();st["occurrences"]=[occurrence()];r=req("REQ-DEMO-SCHED");st["requests"]=[r];st["sessions"]=[sess("SES-DEMO-SCHED",r["request_id"])]
    add_created(st,r,"07:00:00")
    st["records"] += [
        report("REC-EVAL","occurrence_evaluated","occurrence","OCC-DEMO-0700","07:00:00"),
        report("REC-DEMO-START","session_started","session","SES-DEMO-SCHED","07:00:00"),
        report("REC-DEMO-CLOSE","close_requested","session","SES-DEMO-SCHED","07:00:45"),
        report("REC-DEMO-END","session_ended","session","SES-DEMO-SCHED","07:00:45")]
    set_status(st,"07:00:45");st["connectivity_view"]="offline"
    st["dashboard_view"].update(as_of=ts("07:00:45"),last_execution_at_reported=ts("07:00:00"),
        next_planned_at="2026-11-02T07:00:00+07:00",freshness_note="Góc nhìn local; web chưa nhận các record offline")
    return st
def build_data():
    normal=normal_state()
    states={"W00":deepcopy(normal),"W02":normal}
    offline=base();offline["schedule_contents"].append(content(2))
    offline["schedule"].update(desired_content_ref="SC-A-v2",desired_saved_at_web=ts("06:45:00"),
        desired_submission=dict(state="pending",observed_at_web=ts("06:45:01")))
    offline["connectivity_view"]="offline";offline["dashboard_view"]["freshness_note"]="Last-known SC-A-v1; chưa có report applied SC-A-v2"
    states["W01"]=offline
    manual=base();a=req("REQ-DEMO-BUTTON","local_button");b=req("REQ-DEMO-STOP","local_button","STOP")
    manual["requests"]=[a,b];manual["sessions"]=[sess("SES-DEMO-BUTTON",a["request_id"],17,"stopped_reported","manual_stop")]
    add_created(manual,a,"07:15:00");add_created(manual,b,"07:15:17")
    manual["records"] += [report("REC-BUTTON-START","session_started","session","SES-DEMO-BUTTON","07:15:00"),
        report("REC-BUTTON-STOP","stop_received","request",b["request_id"],"07:15:17"),
        report("REC-BUTTON-END","session_ended","session","SES-DEMO-BUTTON","07:15:17")]
    set_status(manual,"07:15:17");manual["connectivity_view"]="offline"
    manual["dashboard_view"].update(as_of=ts("07:15:17"),last_execution_at_reported=ts("07:15:00"),next_planned_at="2026-11-02T07:00:00+07:00")
    states["W03"]=manual
    unknown=base();r=req("REQ-DEMO-REMOTE","remote",status="outcome_unknown",reason="TERMINAL_EVIDENCE_MISSING")
    unknown["requests"]=[r];add_created(unknown,r,"07:00:00")
    unknown["records"] += [report("REC-REMOTE-RX","request_received","request",r["request_id"],"07:00:00","07:00:00"),
        report("REC-REMOTE-AUTH","execution_authorized","request",r["request_id"],"07:00:01","07:00:01")]
    set_status(unknown,"06:59:45","06:59:46")
    unknown["connectivity_view"]="unknown"
    unknown["safety_view"]=dict(new_start_allowed=False,block_reasons=["OUTCOME_UNKNOWN"],basis="Expected block: mọi nguồn/Zone trên nguồn-accounting chung chưa xác minh")
    unknown["dashboard_view"].update(as_of=ts("07:01:00"),next_planned_at=None,next_plan_basis="unknown_until_reconciliation",
        current_output_assessment="unknown",freshness_note="Last-known closed 06:59:45 không chứng minh trạng thái sau remote command")
    states["W04"]=unknown
    sync=deepcopy(normal);set_status(sync,"07:10:00","07:10:01");sync["connectivity_view"]="online"
    deliveries=[]
    for rid,at in [("REC-DEMO-START","07:10:00"),("REC-DEMO-CLOSE","07:10:01"),("REC-DEMO-END","07:10:02")]:
        row=next(r for r in sync["records"] if r["record_id"]==rid);row["received_at_web"]=ts(at)
        deliveries.append(dict(record_id=rid,received_at_web=ts(at),disposition="inserted"))
    deliveries.append(dict(record_id="REC-DEMO-END",received_at_web=ts("07:10:05"),disposition="duplicate_same_payload"))
    sync["ingest_view"]=dict(deliveries=deliveries,unique_record_ids=["REC-DEMO-START","REC-DEMO-CLOSE","REC-DEMO-END"],
        duplicate_record_ids=["REC-DEMO-END"],history_session_ids=["SES-DEMO-SCHED"])
    sync["dashboard_view"].update(as_of=ts("07:10:05"),freshness_note="Event 07:00–07:00:45, nhận 07:10; không phải vừa tưới")
    states["W05"]=sync
    boundary=base();boundary["schedule_contents"]=[content(n) for n in [1,2,3]]
    boundary["occurrences"]=[occurrence()];r=req("REQ-DEMO-RUNNING");boundary["requests"]=[r]
    boundary["sessions"]=[sess("SES-DEMO-RUNNING",r["request_id"],None,"running",None)];add_created(boundary,r,"07:00:00")
    boundary["records"] += [report("REC-EVAL","occurrence_evaluated","occurrence","OCC-DEMO-0700","07:00:00"),
        report("REC-RUNNING-START","session_started","session","SES-DEMO-RUNNING","07:00:00"),
        report("REP-APPLIED-2","schedule_applied","schedule","SCHED-DEMO-A","07:00:12","07:00:13",content_ref="SC-A-v2",source_order=2),
        report("REP-APPLIED-3","schedule_applied","schedule","SCHED-DEMO-A","07:00:16","07:00:17",content_ref="SC-A-v3",source_order=3)]
    boundary["schedule"].update(desired_content_ref="SC-A-v3",desired_saved_at_web=ts("07:00:15"),last_known_applied_report_ref="REP-APPLIED-3")
    set_status(boundary,"07:00:20","07:00:20",active_session_id="SES-DEMO-RUNNING",output_reported_state="open")
    boundary["safety_view"]=dict(new_start_allowed=False,block_reasons=["BUSY"],basis="Expected: phiên cũ đang chạy; không queue START riêng")
    boundary["dashboard_view"].update(as_of=ts("07:00:20"),last_execution_at_reported=ts("07:00:00"),next_planned_at=None,
        next_plan_basis="schedule_paused",freshness_note="SC-A-v3 pause không retime phiên đã bind SC-A-v1")
    for name,source,reason in [("busy","remote","BUSY"),("untrusted_clock","local_button","CLOCK_UNTRUSTED"),("cumulative_limit","local_button","CUMULATIVE_LIMIT_REACHED")]:
        branch=req("REQ-BRANCH-"+name.upper(),source,status="rejected",reason=reason)
        boundary["branches"][name]=dict(independent_example=True,request=branch,new_sessions=[],
            basis="Independent expected rejection, không chọn ngưỡng số/không thực thi arbiter")
    states["W06"]=boundary
    scenes=deepcopy(SCENES)
    for sid,s in scenes.items():
        s["state"]=states[sid];s["state"]["projections"]=derive_state(states[sid])
    cat=dict(areas=[dict(area_id="AREA-DEMO-01",name="Ban công mẫu")],
        zones=[dict(zone_id="ZONE-DEMO-A",name="Zone A mẫu",area_id="AREA-DEMO-01")],
        controllers=[dict(controller_id="CTRL-DEMO-01",label="ESP32 mẫu")],
        actuators=[dict(actuator_id="VALVE-DEMO-A",zone_id="ZONE-DEMO-A",controller_id="CTRL-DEMO-01")])
    immutable={}
    for st in states.values():
        for row in st["schedule_contents"]+st["target_bindings"]:
            immutable[row.get("content_ref",row.get("binding_ref"))]=digest(row)
    return dict(meta=dict(title="miGarden — dữ liệu synthetic cho 03/04",version="public-01-04-v1",information_model="04-v0.2",
        synthetic=True,timezone="Asia/Saigon",case_histories_are_independent=True,not_water_policy=True,
        all_timing_values_illustrative=True,immutable_content_sha256=immutable,
        projections_are_derived_not_storage=True,result_contract="Controller-reported; chưa đo nước",
        notice="Synthetic design, không phải runtime/device/water evidence"),catalog=cat,scenarios=scenes)

def pointer(data,path):
    v=data
    for k in path.strip("/").split("/"):
        k=k.replace("~1","/").replace("~0","~");v=v[int(k)] if isinstance(v,list) else v[k]
    return v
def build_views(data):
    def card(sid,title,model,rid,*fields):
        return dict(title=title,model=model+" · projection, không là fact lưu riêng",record_id=rid,
            fields=[dict(label=label,value=deepcopy(pointer(data,p)),source_path=p) for label,p in fields])
    def C(sid,title,model,rid,*fields):
        return card(sid,title,model,rid,*[(label,f"/scenarios/{sid}/state/{p}") for label,p in fields])
    views=[]
    for sid,s in data["scenarios"].items():
        v={k:deepcopy(s[k]) for k in ["id","title","question","summary","uc_refs","observed_at","takeaways","limitations"]}
        v.update(mode="overview" if sid=="W00" else "timeline",viewpoint=s["viewpoint"],
            lanes=[dict(id="web",label="microSched / web"),dict(id="device",label="Controller ESP32"),dict(id="result",label="Record / điều biết được")],
            events=deepcopy(s["timeline"]),cards=[],relations=[])
        ps="projections/schedule/"
        if sid=="W00":
            v["cards"]=[
                card(sid,"Khu vực","Area","AREA-DEMO-01",("name","/catalog/areas/0/name")),
                card(sid,"Đích tưới","Zone","ZONE-DEMO-A",("area_id","/catalog/zones/0/area_id")),
                card(sid,"Bộ thực thi duy nhất","Controller","CTRL-DEMO-01",("label","/catalog/controllers/0/label")),
                card(sid,"Van nhánh","Actuator","VALVE-DEMO-A",("controller_id","/catalog/actuators/0/controller_id"),("zone_id","/catalog/actuators/0/zone_id")),
                C(sid,"Lịch active","Schedule","SCHED-DEMO-A",("applied content_ref",ps+"last_known_applied/content_ref"),("duration_seconds",ps+"last_known_applied/config/duration_seconds")),
                C(sid,"Cơ hội đến hạn","Occurrence","OCC-DEMO-0700",("content_ref","occurrences/0/content_ref"),("disposition","occurrences/0/evaluation_outcome")),
                C(sid,"Ý định START","ControlRequest","REQ-DEMO-SCHED",("status","requests/0/status")),
                C(sid,"Thực thi đã bắt đầu","Session","SES-DEMO-SCHED",("result","sessions/0/result"),("reported_output_duration_seconds","sessions/0/reported_output_duration_seconds"))]
            v["relations"]=[dict(from_=a,to=b,label=l) for a,b,l in [
                ("AREA-DEMO-01","ZONE-DEMO-A","nhóm vị trí"),("ZONE-DEMO-A","VALVE-DEMO-A","van nhánh"),
                ("CTRL-DEMO-01","VALVE-DEMO-A","điều khiển"),("ZONE-DEMO-A","SCHED-DEMO-A","0..1 lịch active"),
                ("SCHED-DEMO-A","OCC-DEMO-0700","cơ hội"),("OCC-DEMO-0700","REQ-DEMO-SCHED","START"),("REQ-DEMO-SCHED","SES-DEMO-SCHED","đã bắt đầu")]]
            for rel in v["relations"]:rel["from"]=rel.pop("from_")
        elif sid=="W01":
            v["cards"]=[C(sid,"Web mong muốn","Schedule desired","SCHED-DEMO-A",("content_ref",ps+"desired/content_ref"),("at_local_time",ps+"desired/config/recurrence/at_local_time"),("duration_seconds",ps+"desired/config/duration_seconds"),("saved_at_web","schedule/desired_saved_at_web")),
                C(sid,"Bản applied cuối được biết","Schedule supporting info","SCHED-DEMO-A",("content_ref",ps+"last_known_applied/content_ref"),("at_local_time",ps+"last_known_applied/config/recurrence/at_local_time"),("duration_seconds",ps+"last_known_applied/config/duration_seconds"),("reported_at, không actual apply time",ps+"last_known_reported_at")),
                C(sid,"Chưa phải một lần tưới","Derived observation","W01",("application_status",ps+"application_status"),("occurrences","occurrences"),("requests","requests"),("sessions","sessions"))]
        elif sid=="W02":
            sp="projections/sessions/SES-DEMO-SCHED/"
            v["cards"]=[C(sid,"Cơ hội đã xử lý","Occurrence","OCC-DEMO-0700",("planned_for","occurrences/0/planned_for"),("content_ref","occurrences/0/content_ref"),("evaluation_outcome","occurrences/0/evaluation_outcome")),
                C(sid,"Một session có evidence bắt đầu","Session","SES-DEMO-SCHED",("request_id","sessions/0/request_id"),("planned qua ref",sp+"planned_duration_seconds"),("started_at_reported",sp+"started_at_reported"),("ended_at_reported",sp+"ended_at_reported"),("reported_output_duration_seconds","sessions/0/reported_output_duration_seconds"),("result","sessions/0/result")),
                C(sid,"Góc nhìn local, web chưa nhận","Report freshness","REP-STATUS",("connectivity_view","connectivity_view"),("received_at_web", "projections/controller/received_at_web"))]
        elif sid=="W03":
            sp="projections/sessions/SES-DEMO-BUTTON/"
            v["cards"]=[C(sid,"START tại nút","ControlRequest","REQ-DEMO-BUTTON",("trigger_source","requests/0/trigger_source"),("binding_ref","requests/0/binding_ref"),("duration_seconds","requests/0/duration_seconds")),
                C(sid,"STOP không tạo session mới","ControlRequest","REQ-DEMO-STOP",("kind","requests/1/kind"),("status","requests/1/status")),
                C(sid,"45 dự kiến, 17 được báo","Session","SES-DEMO-BUTTON",("planned qua START",sp+"planned_duration_seconds"),("reported_output_duration_seconds","sessions/0/reported_output_duration_seconds"),("result","sessions/0/result"),("end_reason","sessions/0/end_reason"))]
        elif sid=="W04":
            v["cards"]=[C(sid,"Yêu cầu chưa rõ kết quả","ControlRequest","REQ-DEMO-REMOTE",("trigger_source","requests/0/trigger_source"),("status","requests/0/status"),("valid_until","requests/0/valid_until"),("reason","requests/0/reason")),
                C(sid,"Không tạo success giả","Evidence state","W04",("sessions","sessions"),("current_output_assessment","dashboard_view/current_output_assessment"),("last_known_output","projections/controller/output_reported_state"),("last_reported_at","projections/controller/time/at")),
                C(sid,"Expected interlock","Safety view","W04",("new_start_allowed","safety_view/new_start_allowed"),("block_reasons","safety_view/block_reasons"),("next_planned_at","dashboard_view/next_planned_at"))]
        elif sid=="W05":
            idx=next(i for i,r in enumerate(s["state"]["records"]) if r["record_id"]=="REC-DEMO-END")
            v["cards"]=[C(sid,"Nguồn record không đổi","ExecutionRecord","REC-DEMO-END",("record_id",f"records/{idx}/record_id"),("occurred_at_reported",f"records/{idx}/time/at"),("subject_ref",f"records/{idx}/subject_ref")),
                C(sid,"Nhận lại cùng ID","Ingest view","REC-DEMO-END",("first receipt","ingest_view/deliveries/2/received_at_web"),("duplicate receipt","ingest_view/deliveries/3/received_at_web"),("disposition","ingest_view/deliveries/3/disposition"),("history_session_ids","ingest_view/history_session_ids")),
                C(sid,"Dashboard hiểu đúng thời gian","Derived view","W05",("last_execution_at_reported","dashboard_view/last_execution_at_reported"),("next_planned_at","dashboard_view/next_planned_at"),("freshness_note","dashboard_view/freshness_note"))]
        else:
            sp="projections/sessions/SES-DEMO-RUNNING/"
            v["cards"]=[C(sid,"Lịch đã pause tại local","Schedule applied","SCHED-DEMO-A",("applied content_ref",ps+"last_known_applied/content_ref"),("applied enabled",ps+"applied_enabled"),("applied duration_seconds",ps+"last_known_applied/config/duration_seconds")),
                C(sid,"Phiên cũ không retime","Session","SES-DEMO-RUNNING",("bound content_ref",sp+"content_ref"),("planned qua ref",sp+"planned_duration_seconds"),("result","sessions/0/result"),("ended_at_reported",sp+"ended_at_reported"),("reported_output_duration_seconds","sessions/0/reported_output_duration_seconds")),
                C(sid,"Nhánh từ chối độc lập","Request examples","W06-branches",("busy.status","branches/busy/request/status"),("busy.reason","branches/busy/request/reason"),("clock.reason","branches/untrusted_clock/request/reason"),("limit.reason","branches/cumulative_limit/request/reason"),("new_sessions","branches/cumulative_limit/new_sessions"))]
        views.append(v)
    return dict(meta=dict(title="miGarden — nhìn dữ liệu qua 03/04",locale="vi",timezone="Asia/Saigon",synthetic=True,
                         source_file="synthetic-data.json",independent_cases=True,card_contract="Derived projections, not independent stored facts"),scenes=views)
def write_outputs():
    data=build_data();views=build_views(data)
    for name,obj in [("synthetic-data.json",data),("views.json",views)]:
        (ROOT/name).write_text(json.dumps(obj,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    md=["# miGarden — kịch bản và dữ liệu 03/04", "", "public-01-04-v1 · synthetic · UTC+7. Bảy cảnh độc lập, không phải runtime/device/water evidence.", "",
        "Nguồn: prepare_data.py → synthetic-data.json → views.json → SVG/PNG/index. Các card là projection, không là fact lưu độc lập. Nội dung/binding bất biến chỉ resolve qua ref; kết quả hiện tại Controller-reported, chưa đo nước.", ""]
    for v in views["scenes"]:
        md += [f"## {v['id']} — {v['title']}","",v["question"],"",v["summary"],"",f"Góc nhìn {v['viewpoint']}; quan sát {v['observed_at']}.",f"![{v['id']}](figures/{v['id']}.png)",""]
        if v["events"]:
            md += ["| Mốc +07:00 | Bên | Sự kiện |","|---|---|---|"]
            md += [f"| {e['at']} | {e['lane']} | {e['title']}: {e['detail']} |" for e in v["events"]]
        for c in v["cards"]:
            md += ["",f"### {c['title']} — projection","", "| Thông tin | Giá trị mẫu |","|---|---|"]
            for f in c["fields"]: md.append(f"| {f['label']} | {json.dumps(f['value'],ensure_ascii=False).replace('|',' / ')} |")
        md += ["","Điều cần thấy: "+" ".join(v["takeaways"]),"","Giới hạn: "+" ".join(v["limitations"])]
    md += ["","## Kiểm tra và giới hạn","","validate_data.py kiểm immutable refs/target/disposition/duration/reports/dedup/expiry và card provenance, với mutation trong bộ nhớ. python -B imports không ghi nguồn. Kiểm tra fixture không phải independent review hoặc chứng nhận renderer/thiết bị. Safety/clock/budget là expected design, không thực thi firmware hoặc chọn policy số.",""]
    (ROOT/"scenarios.vi.md").write_text("\n".join(md),encoding="utf-8")
    print(json.dumps(dict(scenarios=len(data["scenarios"]),views=len(views["scenes"]),cards=sum(len(v["cards"]) for v in views["scenes"])),ensure_ascii=False))
if __name__ == "__main__": write_outputs()
