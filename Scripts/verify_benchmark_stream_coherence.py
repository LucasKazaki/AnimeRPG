#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, os, re, sys
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any
try:
    import benchmark_manifest
    import analyze_frame_timing_capture as frame
    import analyze_frame_phase_timing_capture as phase
    import analyze_process_memory_capture as memory
except ImportError:
    benchmark_manifest = frame = phase = memory = None

analyze_frame_timing_capture = frame
analyze_frame_phase_timing_capture = phase
analyze_process_memory_capture = memory

SCHEMA_VERSION = 1
MAX_JSON_BYTES = 4 * 1024 * 1024
COMMIT_RE = re.compile(r"^[0-9a-fA-F]{40}$")
SHA_RE = re.compile(r"^[0-9a-fA-F]{64}$")
ROLES = {"whole_frame":"cpu_frame_timing_csv","main_thread_phases":"cpu_phase_timing_csv","process_memory":"process_memory_csv"}
ACCEPTANCE = {k:False for k in (
    "performance_budget_verified","ram_budget_verified","vram_budget_verified",
    "memory_leak_free_verified","gpu_timing_verified","comparative_parity_verified",
    "instrumentation_overhead_verified","clean_machine_compatibility_verified","independent_acceptance")}
LIMITATIONS = [
    "Coherence proves one verified package/benchmark identity and compatible frame interval only.",
    "Frame indices do not prove clock synchronization, timing accuracy, hardware-label truth, or negligible instrumentation overhead.",
    "Process memory is OS process-level evidence; allocator attribution, leak freedom, VRAM, and GPU timing remain unverified.",
    "No budget, Unreal/Unity parity, clean-machine, soak, or independent acceptance follows from this receipt.",
]
class CoherenceError(ValueError): pass
BenchmarkStreamCoherenceError = CoherenceError

def _sha(p:Path)->str:
    h=hashlib.sha256()
    with p.open("rb") as f:
        for c in iter(lambda:f.read(1024*1024),b""): h.update(c)
    return h.hexdigest()

def _uint(v:Any,label:str,minimum=0)->int:
    if isinstance(v,bool) or not isinstance(v,int) or v<minimum: raise CoherenceError(f"{label} must be integer >= {minimum}")
    return v

def _meta(c:dict[str,Any], label:str)->dict[str,Any]:
    m=c.get("source_metadata")
    if not isinstance(m,dict): raise CoherenceError(f"{label} metadata malformed")
    return m

def _range(c:dict[str,Any], label:str)->tuple[int,int,int]:
    n=_uint(c.get("sample_count"),f"{label}.sample_count",1); a=_uint(c.get("start_frame_index"),f"{label}.start"); b=_uint(c.get("end_frame_index"),f"{label}.end")
    if b<a: raise CoherenceError(f"{label} frame interval is reversed")
    return n,a,b

def verify_capture_alignment(w:dict[str,Any], p:dict[str,Any], m:dict[str,Any])->dict[str,Any]:
    wm,pm,mm=_meta(w,"whole"),_meta(p,"phase"),_meta(m,"memory")
    warm=[_uint(x.get("warmup_frames"),"warmup_frames") for x in (wm,pm,mm)]
    if len(set(warm))!=1: raise CoherenceError(f"stream warmup mismatch: {warm}")
    wr,pr,mr=_range(w,"whole"),_range(p,"phase"),_range(m,"memory")
    if any(r[1]!=warm[0] for r in (wr,pr,mr)): raise CoherenceError("all streams must start exactly at warmup")
    if wr[0]!=wr[2]-wr[1]+1 or pr[0]!=pr[2]-pr[1]+1: raise CoherenceError("timing stream is not contiguous")
    if wr!=pr: raise CoherenceError("whole-frame and phase intervals differ")
    wmax,pmax=_uint(wm.get("max_samples"),"whole.max",1),_uint(pm.get("max_samples"),"phase.max",1)
    ws,ps=wm.get("samples_saturated"),pm.get("samples_saturated")
    wl,pl=w.get("sample_count_reached_limit"),p.get("sample_count_reached_limit")
    if wmax!=pmax or not isinstance(ws,bool) or not isinstance(ps,bool) or ws!=ps or not isinstance(wl,bool) or not isinstance(pl,bool) or wl!=pl:
        raise CoherenceError("whole-frame and phase capture limit state differs")
    stride=_uint(mm.get("sample_every_frames"),"memory.stride",1); mmax=_uint(mm.get("max_samples"),"memory.max",1); ms=mm.get("samples_saturated")
    if not isinstance(ms,bool) or mr[0]>mmax or (ms and mr[0]!=mmax): raise CoherenceError("process-memory sample limit state is invalid")
    expected=((wr[2]-wr[1])//stride)+1; expected_end=wr[1]+(expected-1)*stride
    if mr[0]!=expected or mr[2]!=expected_end or mr[2]>wr[2]: raise CoherenceError("process-memory scheduled samples do not cover the timing interval")
    return {"warmup_frames":warm[0],"measured_frame_interval":{"start_frame_index":wr[1],"end_frame_index":wr[2],"frame_count":wr[0]},
            "whole_frame":{"sample_count":wr[0],"max_samples":wmax,"samples_saturated":ws},
            "main_thread_phases":{"sample_count":pr[0],"max_samples":pmax,"samples_saturated":ps},
            "process_memory":{"sample_count":mr[0],"sample_every_frames":stride,"end_frame_index":mr[2],"expected_sample_count_for_interval":expected}}

def validate_memory_source_for_provenance(c:dict[str,Any], evidence_class:str)->None:
    source=_meta(c,"memory").get("sample_source")
    if evidence_class=="local_native" and source!="Windows_GetProcessMemoryInfo_PROCESS_MEMORY_COUNTERS_EX": raise CoherenceError("local_native requires production Windows process-memory source")
    if evidence_class not in ("local_native","hosted_contract_fixture"): raise CoherenceError("invalid evidence_class")

def _source(b:dict[str,Any], root:Path, role:str)->tuple[str,Path,int,str]:
    items=[x for x in b.get("evidence",[]) if isinstance(x,dict) and x.get("role")==role]
    if len(items)!=1: raise CoherenceError(f"benchmark must contain exactly one {role}")
    it=items[0]; rel=it.get("path")
    if not isinstance(rel,str) or not rel or "\\" in rel or ":" in rel.split("/",1)[0]: raise CoherenceError("unsafe evidence path")
    q=PurePosixPath(rel)
    if q.is_absolute() or any(x in ("",".","..") for x in q.parts) or q.as_posix()!=rel: raise CoherenceError("unsafe evidence path")
    if root.is_symlink(): raise CoherenceError("evidence root is symlink")
    base=root.resolve(strict=True); cur=base
    for part in q.parts:
        cur=cur/part
        if cur.is_symlink(): raise CoherenceError("symlinked evidence component")
    path=cur.resolve(strict=True)
    if os.path.commonpath((str(base),str(path)))!=str(base) or not path.is_file(): raise CoherenceError("evidence escapes root")
    size,digest=path.stat().st_size,_sha(path)
    if isinstance(it.get("bytes"),bool) or it.get("bytes")!=size or not isinstance(it.get("sha256"),str) or it["sha256"].lower()!=digest: raise CoherenceError(f"{role} bytes/hash mismatch")
    return rel,path,size,digest

def verify_bound_stream_coherence(b:dict[str,Any], package:Path, release:Path, evidence:Path, *, expected_commit:str, expected_executable_sha256:str)->dict[str,Any]:
    if any(x is None for x in (benchmark_manifest,frame,phase,memory)): raise CoherenceError("repository profiling modules unavailable")
    if not COMMIT_RE.fullmatch(expected_commit) or not SHA_RE.fullmatch(expected_executable_sha256): raise CoherenceError("expected candidate identity malformed")
    try: verified=benchmark_manifest.verify_benchmark_manifest(b,package,release,evidence)
    except Exception as exc: raise CoherenceError(f"benchmark manifest verification failed: {exc}") from exc
    candidate=b.get("candidate")
    if not isinstance(candidate,dict) or str(candidate.get("commit","")).lower()!=expected_commit.lower() or str(candidate.get("executable_sha256","")).lower()!=expected_executable_sha256.lower(): raise CoherenceError("benchmark candidate identity mismatch")
    descriptor=verified.get("benchmark_descriptor_sha256") if isinstance(verified,dict) else None
    if not isinstance(descriptor,str) or not SHA_RE.fullmatch(descriptor): raise CoherenceError("verified descriptor malformed")
    sources={}; paths={}
    for name,role in ROLES.items():
        rel,path,size,digest=_source(b,evidence,role); paths[name]=path; sources[name]={"role":role,"path":rel,"bytes":size,"sha256":digest}
    try:
        w=frame.parse_frame_timing_csv(paths["whole_frame"]); p=phase.parse_frame_phase_timing_csv(paths["main_thread_phases"]); m=memory.parse_process_memory_csv(paths["process_memory"])
    except Exception as exc: raise CoherenceError(f"profiling parser rejected evidence: {exc}") from exc
    alignment=verify_capture_alignment(w,p,m); prov=b.get("provenance")
    if not isinstance(prov,dict) or not isinstance(prov.get("evidence_class"),str): raise CoherenceError("benchmark provenance malformed")
    validate_memory_source_for_provenance(m,prov["evidence_class"])
    required=["workload","run_protocol","environment","reference_versions"]
    if not all(isinstance(b.get(k),dict) for k in required): raise CoherenceError("benchmark descriptor fields malformed")
    return {"schema_version":1,"verification_kind":"astral_benchmark_stream_coherence","generated_at_utc":datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00","Z"),
            "stream_coherence_verified":True,"benchmark_descriptor_sha256":descriptor.lower(),"candidate_commit":expected_commit.lower(),"executable_sha256":expected_executable_sha256.lower(),
            **{k:json.loads(json.dumps(b[k])) for k in required},"provenance":json.loads(json.dumps(prov)),"sources":sources,"alignment":alignment,"acceptance":dict(ACCEPTANCE),"limitations":list(LIMITATIONS)}

def _load(path:Path)->dict[str,Any]:
    if path.stat().st_size>MAX_JSON_BYTES: raise CoherenceError("benchmark manifest byte limit exceeded")
    x=json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(x,dict): raise CoherenceError("benchmark manifest root must be object")
    return x

def _write(path:Path, value:dict[str,Any])->None:
    if not path.parent.is_dir() or path.exists() or path.is_symlink(): raise CoherenceError("output parent invalid or output already exists")
    text=json.dumps(value,indent=2,sort_keys=True)+"\n"
    if len(text.encode())>MAX_JSON_BYTES: raise CoherenceError("output byte limit exceeded")
    with path.open("x",encoding="utf-8",newline="\n") as f: f.write(text)

_write_new_json = _write

def main(argv:list[str]|None=None)->int:
    p=argparse.ArgumentParser(); p.add_argument("--benchmark-manifest",required=True,type=Path); p.add_argument("--package-root",required=True,type=Path); p.add_argument("--release-manifest",required=True,type=Path); p.add_argument("--evidence-root",required=True,type=Path); p.add_argument("--expected-commit",required=True); p.add_argument("--expected-executable-sha256",required=True); p.add_argument("--output",required=True,type=Path); a=p.parse_args(argv)
    try: _write(a.output,verify_bound_stream_coherence(_load(a.benchmark_manifest),a.package_root,a.release_manifest,a.evidence_root,expected_commit=a.expected_commit,expected_executable_sha256=a.expected_executable_sha256))
    except (CoherenceError,OSError,ValueError) as exc: print(f"benchmark stream coherence failed: {exc}",file=sys.stderr); return 1
    return 0
if __name__=="__main__": raise SystemExit(main())
