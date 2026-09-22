#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, os, re, subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any
try:
    import benchmark_manifest  # type: ignore
except ImportError:
    benchmark_manifest = None

SCHEMA_VERSION=1; MAX_RECEIPT_BYTES=1024*1024; SHA_RE=re.compile(r"^[0-9a-fA-F]{64}$"); COMMIT_RE=re.compile(r"^[0-9a-fA-F]{40}$")
ACCEPTANCE={"environment_binding_verified":False,"performance_budget_verified":False,"comparative_parity_verified":False,"clean_machine_compatibility_verified":False,"independent_acceptance":False}
LIMITATIONS=[
    "This receipt records selected Windows CIM properties and binds them to benchmark environment labels; it does not benchmark performance or prove hardware health.",
    "Win32_VideoController data may be inaccurate for hardware that is not compatible with Windows Display Driver Model (WDDM).",
    "GPU names and driver versions identify enumerated Windows video-controller instances but do not establish active-render-adapter selection or VRAM usage.",
    "The JSON receipt is provenance evidence, not cryptographic hardware attestation; independent acceptance still requires retained command logs and reviewer evidence.",
    "No performance budget, parity, clean-machine compatibility, or independent acceptance follows from environment capture or binding.",
]
class EnvironmentError(ValueError): pass

def _now(): return datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00","Z")
def _sha(path):
    h=hashlib.sha256()
    with Path(path).open("rb") as f:
        for c in iter(lambda:f.read(1<<20),b""): h.update(c)
    return h.hexdigest()
def _obj(v,l):
    if not isinstance(v,dict): raise EnvironmentError(f"{l} must be an object")
    return v
def _keys(v,k,l):
    if set(v)!=k: raise EnvironmentError(f"{l} keys mismatch")
def _text(v,l,n=512):
    if not isinstance(v,str): raise EnvironmentError(f"{l} must be text")
    v=" ".join(v.strip().split())
    if not v or len(v)>n: raise EnvironmentError(f"{l} must be non-empty text <= {n} characters")
    return v
def _opt(v,l,n=256): return None if v is None else _text(v,l,n)
def _uint(v,l,lo=0,hi=(1<<63)-1):
    if isinstance(v,bool) or not isinstance(v,int) or not lo<=v<=hi: raise EnvironmentError(f"{l} must be an integer in [{lo}, {hi}]")
    return v
def _arr(v,l):
    if isinstance(v,dict): return [v]
    if not isinstance(v,list): raise EnvironmentError(f"{l} must be an array")
    return v

def _derive(raw):
    o=_obj(raw["operating_system"],"raw.operating_system"); _keys(o,{"caption","version","build_number","architecture"},"raw.operating_system")
    caption=_text(o["caption"],"caption",128); version=_text(o["version"],"version",64); build=_text(o["build_number"],"build",64); arch=_text(o["architecture"],"architecture",64)
    c=_obj(raw["computer_system"],"raw.computer_system"); _keys(c,{"manufacturer","model","logical_processors","total_physical_memory_bytes"},"raw.computer_system")
    _opt(c["manufacturer"],"manufacturer"); _opt(c["model"],"model"); logical=_uint(c["logical_processors"],"logical_processors",1,4096); ram=_uint(c["total_physical_memory_bytes"],"ram",1)
    ps=[]
    for i,x in enumerate(raw["processors"] if isinstance(raw["processors"],list) else []):
        x=_obj(x,f"processor[{i}]"); _keys(x,{"name","manufacturer","cores","logical_processors"},f"processor[{i}]")
        ps.append({"name":_text(x["name"],"cpu.name",256),"manufacturer":_opt(x["manufacturer"],"cpu.manufacturer"),"cores":_uint(x["cores"],"cpu.cores",1,4096),"logical_processors":_uint(x["logical_processors"],"cpu.logical",1,4096)})
    if not ps: raise EnvironmentError("raw.processors must be non-empty")
    if ps!=sorted(ps,key=lambda x:(x["name"].casefold(),x["manufacturer"] or "",x["cores"],x["logical_processors"])): raise EnvironmentError("raw.processors must be deterministically sorted")
    vs=[]
    for i,x in enumerate(raw["video_controllers"] if isinstance(raw["video_controllers"],list) else []):
        x=_obj(x,f"video[{i}]"); _keys(x,{"name","driver_version","pnp_device_id","adapter_ram_bytes_reported","video_processor"},f"video[{i}]")
        ar=x["adapter_ram_bytes_reported"]; ar=None if ar is None else _uint(ar,"adapter_ram",0)
        vs.append({"name":_text(x["name"],"gpu.name",256),"driver_version":_text(x["driver_version"],"gpu.driver",128),"pnp_device_id":_opt(x["pnp_device_id"],"gpu.pnp",512),"adapter_ram_bytes_reported":ar,"video_processor":_opt(x["video_processor"],"gpu.processor",256)})
    if not vs: raise EnvironmentError("raw.video_controllers must be non-empty")
    if vs!=sorted(vs,key=lambda x:(x["name"].casefold(),x["pnp_device_id"] or "",x["driver_version"])): raise EnvironmentError("raw.video_controllers must be deterministically sorted")
    env={"os":f"{caption} {version} build {build} ({arch})","cpu":"; ".join(dict.fromkeys(x["name"] for x in ps)),"logical_cpus":logical,"ram_bytes":ram,"gpu":"; ".join(dict.fromkeys(x["name"] for x in vs)),"gpu_driver":"; ".join(f"{x['name']}={x['driver_version']}" for x in vs)}
    for k in ("os","cpu","gpu","gpu_driver"): env[k]=_text(env[k],k,256)
    return env

def normalize_cim_payload(p,machine_label,*,source="Windows_CIM_GetCimInstance"):
    p=_obj(p,"CIM payload"); _keys(p,{"operating_system","computer_system","processors","video_controllers"},"CIM payload")
    o=_obj(p["operating_system"],"operating_system"); _keys(o,{"Caption","Version","BuildNumber","OSArchitecture"},"operating_system")
    c=_obj(p["computer_system"],"computer_system"); _keys(c,{"Manufacturer","Model","NumberOfLogicalProcessors","TotalPhysicalMemory"},"computer_system")
    raw={"operating_system":{"caption":_text(o["Caption"],"Caption",128),"version":_text(o["Version"],"Version",64),"build_number":_text(o["BuildNumber"],"BuildNumber",64),"architecture":_text(o["OSArchitecture"],"OSArchitecture",64)},"computer_system":{"manufacturer":_opt(c["Manufacturer"],"Manufacturer"),"model":_opt(c["Model"],"Model"),"logical_processors":_uint(c["NumberOfLogicalProcessors"],"NumberOfLogicalProcessors",1,4096),"total_physical_memory_bytes":_uint(c["TotalPhysicalMemory"],"TotalPhysicalMemory",1)},"processors":[],"video_controllers":[]}
    for i,x in enumerate(_arr(p["processors"],"processors")):
        x=_obj(x,f"processors[{i}]"); _keys(x,{"Name","Manufacturer","NumberOfCores","NumberOfLogicalProcessors"},f"processors[{i}]")
        raw["processors"].append({"name":_text(x["Name"],"Name",256),"manufacturer":_opt(x["Manufacturer"],"Manufacturer"),"cores":_uint(x["NumberOfCores"],"NumberOfCores",1,4096),"logical_processors":_uint(x["NumberOfLogicalProcessors"],"NumberOfLogicalProcessors",1,4096)})
    for i,x in enumerate(_arr(p["video_controllers"],"video_controllers")):
        x=_obj(x,f"video_controllers[{i}]"); _keys(x,{"Name","DriverVersion","PNPDeviceID","AdapterRAM","VideoProcessor"},f"video_controllers[{i}]"); ar=x["AdapterRAM"]
        raw["video_controllers"].append({"name":_text(x["Name"],"Name",256),"driver_version":_text(x["DriverVersion"],"DriverVersion",128),"pnp_device_id":_opt(x["PNPDeviceID"],"PNPDeviceID",512),"adapter_ram_bytes_reported":None if ar is None else _uint(ar,"AdapterRAM",0),"video_processor":_opt(x["VideoProcessor"],"VideoProcessor",256)})
    raw["processors"].sort(key=lambda x:(x["name"].casefold(),x["manufacturer"] or "",x["cores"],x["logical_processors"])); raw["video_controllers"].sort(key=lambda x:(x["name"].casefold(),x["pnp_device_id"] or "",x["driver_version"]))
    if not raw["processors"] or not raw["video_controllers"]: raise EnvironmentError("CPU and GPU inventories must be non-empty")
    r={"schema_version":1,"generated_at_utc":_now(),"source":_text(source,"source",128),"capture_platform":"Windows" if source=="Windows_CIM_GetCimInstance" else "fixture","machine_label":_text(machine_label,"machine_label",128),"benchmark_environment":_derive(raw),"raw":raw,"acceptance":dict(ACCEPTANCE),"limitations":list(LIMITATIONS)}
    return validate_receipt(r)

def validate_receipt(r):
    r=_obj(r,"environment receipt"); _keys(r,{"schema_version","generated_at_utc","source","capture_platform","machine_label","benchmark_environment","raw","acceptance","limitations"},"environment receipt")
    if isinstance(r["schema_version"],bool) or r["schema_version"]!=1: raise EnvironmentError("schema mismatch")
    _text(r["generated_at_utc"],"generated_at_utc",64); src=_text(r["source"],"source",128); _text(r["machine_label"],"machine_label",128)
    if r["capture_platform"] not in ("Windows","fixture") or (src=="Windows_CIM_GetCimInstance" and r["capture_platform"]!="Windows"): raise EnvironmentError("capture platform/source mismatch")
    raw=_obj(r["raw"],"raw"); _keys(raw,{"operating_system","computer_system","processors","video_controllers"},"raw")
    if r["benchmark_environment"]!=_derive(raw): raise EnvironmentError("benchmark_environment does not exactly derive from raw CIM properties")
    a=_obj(r["acceptance"],"acceptance"); _keys(a,set(ACCEPTANCE),"acceptance")
    if any(a[k] is not False for k in ACCEPTANCE) or r["limitations"]!=LIMITATIONS: raise EnvironmentError("receipt claim boundaries changed")
    return r

def _powershell_payload(timeout=30):
    if os.name!="nt": raise EnvironmentError("Windows CIM capture is only available on Windows")
    ps=r'''$ErrorActionPreference='Stop'; $o=Get-CimInstance Win32_OperatingSystem -Property Caption,Version,BuildNumber,OSArchitecture; $c=Get-CimInstance Win32_ComputerSystem -Property Manufacturer,Model,NumberOfLogicalProcessors,TotalPhysicalMemory; $p=@(Get-CimInstance Win32_Processor -Property Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors); $v=@(Get-CimInstance Win32_VideoController -Property Name,DriverVersion,PNPDeviceID,AdapterRAM,VideoProcessor); [ordered]@{operating_system=[ordered]@{Caption=$o.Caption;Version=$o.Version;BuildNumber=$o.BuildNumber;OSArchitecture=$o.OSArchitecture};computer_system=[ordered]@{Manufacturer=$c.Manufacturer;Model=$c.Model;NumberOfLogicalProcessors=[int64]$c.NumberOfLogicalProcessors;TotalPhysicalMemory=[int64]$c.TotalPhysicalMemory};processors=@($p|%{[ordered]@{Name=$_.Name;Manufacturer=$_.Manufacturer;NumberOfCores=[int64]$_.NumberOfCores;NumberOfLogicalProcessors=[int64]$_.NumberOfLogicalProcessors}});video_controllers=@($v|%{[ordered]@{Name=$_.Name;DriverVersion=$_.DriverVersion;PNPDeviceID=$_.PNPDeviceID;AdapterRAM=if($null-eq$_.AdapterRAM){$null}else{[int64]$_.AdapterRAM};VideoProcessor=$_.VideoProcessor}})}|ConvertTo-Json -Depth 6 -Compress'''
    try: q=subprocess.run(["powershell.exe","-NoLogo","-NoProfile","-NonInteractive","-Command",ps],capture_output=True,text=True,timeout=timeout)
    except (FileNotFoundError,subprocess.TimeoutExpired) as e: raise EnvironmentError(f"Windows CIM capture failed: {e}") from e
    if q.returncode: raise EnvironmentError(f"Windows CIM capture exit {q.returncode}: {q.stderr.strip()}")
    try: return _obj(json.loads(q.stdout),"CIM capture")
    except Exception as e: raise EnvironmentError(f"invalid CIM JSON: {e}") from e

def write_fresh_json(path,value):
    path=Path(path)
    if path.exists() or path.is_symlink(): raise EnvironmentError(f"refusing to overwrite existing output: {path}")
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open("x",encoding="utf-8",newline="\n") as f: json.dump(value,f,indent=2,sort_keys=True); f.write("\n")
def _load(path,label,limit=MAX_RECEIPT_BYTES):
    path=Path(path)
    if path.is_symlink() or not path.is_file() or path.stat().st_size>limit: raise EnvironmentError(f"invalid {label} file")
    try: v=json.loads(path.read_text(encoding="utf-8"))
    except Exception as e: raise EnvironmentError(f"cannot read {label}: {e}") from e
    return _obj(v,label)
def load_receipt(path): return validate_receipt(_load(path,"environment receipt"))

def _safe_receipt(root,rel,receipt):
    if not isinstance(rel,str) or not rel or "\\" in rel or "\0" in rel: raise EnvironmentError("unsafe environment evidence path")
    q=PurePosixPath(rel)
    if q.is_absolute() or any(x in ("",".","..") for x in q.parts) or q.as_posix()!=rel: raise EnvironmentError("unsafe environment evidence path")
    root=Path(root)
    if root.is_symlink(): raise EnvironmentError("evidence root must not be symlink")
    rr=root.resolve(strict=True); p=rr
    for x in q.parts:
        p=p/x
        if p.is_symlink(): raise EnvironmentError("symlinked environment evidence")
    p=p.resolve(strict=True)
    if os.path.commonpath((str(rr),str(p)))!=str(rr) or p!=Path(receipt).resolve(strict=True): raise EnvironmentError("environment receipt path mismatch")
    return p

def verify_binding(m,r,*,receipt_sha256,evidence_path,expected_commit,expected_executable_sha256):
    validate_receipt(r)
    if not isinstance(expected_commit,str) or not COMMIT_RE.fullmatch(expected_commit): raise EnvironmentError("invalid expected commit")
    if not isinstance(expected_executable_sha256,str) or not SHA_RE.fullmatch(expected_executable_sha256) or not SHA_RE.fullmatch(receipt_sha256): raise EnvironmentError("invalid SHA-256")
    c=m.get("candidate",{}); p=m.get("provenance",{})
    if c.get("commit","").lower()!=expected_commit.lower() or c.get("executable_sha256","").lower()!=expected_executable_sha256.lower(): raise EnvironmentError("candidate/package identity mismatch")
    if m.get("environment")!=r["benchmark_environment"] or not isinstance(p,dict) or p.get("machine_label")!=r["machine_label"]: raise EnvironmentError("benchmark environment/machine mismatch")
    if p.get("evidence_class")=="local_native" and (r["source"]!="Windows_CIM_GetCimInstance" or r["capture_platform"]!="Windows"): raise EnvironmentError("local_native requires Windows CIM receipt")
    hits=[x for x in m.get("evidence",[]) if isinstance(x,dict) and x.get("role")=="windows_benchmark_environment_json"]
    if len(hits)!=1 or hits[0].get("path")!=evidence_path or hits[0].get("sha256","").lower()!=receipt_sha256.lower(): raise EnvironmentError("environment evidence binding mismatch")
    return {"schema_version":1,"generated_at_utc":_now(),"benchmark_descriptor_sha256":m.get("benchmark_descriptor_sha256"),"candidate_commit":expected_commit.lower(),"executable_sha256":expected_executable_sha256.lower(),"machine_label":r["machine_label"],"environment_receipt":{"path":evidence_path,"sha256":receipt_sha256.lower()},"environment":r["benchmark_environment"],"verification":{"manifest_environment_exact_match":True,"machine_label_exact_match":True,"environment_evidence_hash_match":True,"native_source_required_and_verified":p.get("evidence_class")=="local_native"},"acceptance":dict(ACCEPTANCE),"limitations":list(LIMITATIONS)}

def verify_manifest_bound_environment(a):
    if benchmark_manifest is None: raise EnvironmentError("repository benchmark_manifest module unavailable")
    m=_load(a.benchmark_manifest,"benchmark manifest",4*1024*1024)
    try: benchmark_manifest.verify_benchmark_manifest(m,Path(a.package_root),Path(a.release_manifest),Path(a.evidence_root))
    except Exception as e: raise EnvironmentError(f"benchmark manifest verification failed: {e}") from e
    r=load_receipt(a.environment_receipt); hits=[x for x in m.get("evidence",[]) if isinstance(x,dict) and x.get("role")=="windows_benchmark_environment_json"]
    if len(hits)!=1: raise EnvironmentError("exactly one environment evidence role required")
    p=_safe_receipt(a.evidence_root,hits[0].get("path"),a.environment_receipt)
    out=verify_binding(m,r,receipt_sha256=_sha(p),evidence_path=hits[0]["path"],expected_commit=a.expected_commit,expected_executable_sha256=a.expected_executable_sha256)
    write_fresh_json(a.output,out); return out

def main(argv=None):
    p=argparse.ArgumentParser(); s=p.add_subparsers(dest="cmd",required=True)
    c=s.add_parser("capture"); c.add_argument("--machine-label",required=True); c.add_argument("--output",required=True); c.add_argument("--timeout-seconds",type=int,default=30)
    v=s.add_parser("validate"); v.add_argument("--receipt",required=True)
    b=s.add_parser("verify")
    for x in ("benchmark-manifest","package-root","release-manifest","evidence-root","environment-receipt","expected-commit","expected-executable-sha256","output"): b.add_argument("--"+x,required=True)
    a=p.parse_args(argv)
    try:
        if a.cmd=="capture":
            if not 1<=a.timeout_seconds<=300: raise EnvironmentError("timeout out of range")
            r=normalize_cim_payload(_powershell_payload(a.timeout_seconds),a.machine_label); write_fresh_json(a.output,r); print(json.dumps(r["benchmark_environment"],sort_keys=True))
        elif a.cmd=="validate": print(json.dumps(load_receipt(a.receipt)["benchmark_environment"],sort_keys=True))
        else: print(json.dumps(verify_manifest_bound_environment(a)["verification"],sort_keys=True))
    except (EnvironmentError,OSError,ValueError) as e: print(f"ERROR: {e}",file=__import__('sys').stderr); return 2
    return 0
if __name__=="__main__": raise SystemExit(main())
