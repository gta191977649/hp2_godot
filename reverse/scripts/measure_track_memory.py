"""TASK0060: sample the actual Windows Godot process, never its console launcher.

PageFaultCount includes soft faults; it is NOT a hard-fault or paging-I/O count.
Logs/artifacts must remain outside tracked evidence (asset-dependent output).
"""
import argparse, ctypes as c, json, subprocess, threading, time
from ctypes import wintypes as w
from pathlib import Path

class Counters(c.Structure):
    _fields_=[('cb',w.DWORD),('PageFaultCount',w.DWORD)]+[(n,c.c_size_t) for n in
        ('PeakWorkingSetSize','WorkingSetSize','QuotaPeakPagedPoolUsage','QuotaPagedPoolUsage',
         'QuotaPeakNonPagedPoolUsage','QuotaNonPagedPoolUsage','PagefileUsage','PeakPagefileUsage','PrivateUsage')]

class MemoryStatus(c.Structure):
    _fields_=[('length',w.DWORD),('load',w.DWORD)]+[(n,c.c_uint64) for n in
        ('physical_total','physical_available','pagefile_total','pagefile_available','virtual_total','virtual_available','extended_available')]

class PdhDouble(c.Structure):
    _fields_=[('status',w.DWORD),('value',c.c_double)]

class PagingCounters:
    """System-wide disk paging evidence, not attribution to a single process."""
    def __init__(self):
        self.dll=c.WinDLL('pdh');self.query=c.c_void_p();self.counters={}
        self.dll.PdhOpenQueryW(None,0,c.byref(self.query))
        for name,path in [('system_pages_input_per_sec',r'\Memory\Pages Input/sec'),
                          ('system_page_reads_per_sec',r'\Memory\Page Reads/sec')]:
            handle=c.c_void_p()
            if self.dll.PdhAddEnglishCounterW(self.query,c.c_wchar_p(path),0,c.byref(handle))==0:self.counters[name]=handle
        self.dll.PdhCollectQueryData(self.query)
    def sample(self):
        result={};self.dll.PdhCollectQueryData(self.query)
        for name,handle in self.counters.items():
            value=PdhDouble()
            if self.dll.PdhGetFormattedCounterValue(handle,0x200,None,c.byref(value))==0:result[name]=value.value
        return result
    def close(self):self.dll.PdhCloseQuery(self.query)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--engine',required=True,type=Path)
    p.add_argument('--project',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    p.add_argument('--enforce',action='store_true')
    a=p.parse_args()
    if 'console' in a.engine.name.lower():p.error('Use actual Godot executable, not console launcher')
    a.output.mkdir(parents=True,exist_ok=True)
    cmd=[str(a.engine),'--path',str(a.project),'--script','res://scripts/debug/track_memory_smoke.gd','--','--expect-debug=true',
         '--memory-capture='+str(a.output.resolve()/'window.png')]
    if a.enforce:cmd.append('--memory-enforce')
    proc=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf-8',errors='replace')
    kernel=c.WinDLL('kernel32',use_last_error=True)
    kernel.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];kernel.OpenProcess.restype=w.HANDLE
    kernel.CloseHandle.argtypes=[w.HANDLE]
    getmem=c.WinDLL('psapi').GetProcessMemoryInfo
    getmem.argtypes=[w.HANDLE,c.POINTER(Counters),w.DWORD]
    handle=kernel.OpenProcess(0x410,False,proc.pid)
    if not handle:raise c.WinError(c.get_last_error())
    paging=PagingCounters()
    def snapshot():
        m=Counters();m.cb=c.sizeof(m)
        if not getmem(handle,c.byref(m),m.cb):return {}
        return dict(private_bytes=m.PrivateUsage,working_set=m.WorkingSetSize,
                    peak_working_set=m.PeakWorkingSetSize,commit_bytes=m.PagefileUsage,
                    page_faults_including_soft=m.PageFaultCount)
    phase=['startup'];checkpoints=[]
    def read():
        with (a.output/'engine.log').open('w',encoding='utf-8') as log:
            for line in proc.stdout:
                log.write(line);log.flush()
                if line.startswith('MEMORY_CHECKPOINT '):
                    value=json.loads(line[len('MEMORY_CHECKPOINT '):]);value['process']=snapshot();checkpoints.append(value);phase[0]=value['phase']
    reader=threading.Thread(target=read);reader.start()
    samples=[];start=time.monotonic()
    try:
        while proc.poll() is None:
            value=snapshot()
            if value:
                memory=MemoryStatus();memory.length=c.sizeof(memory);kernel.GlobalMemoryStatusEx(c.byref(memory))
                samples.append(dict(seconds=round(time.monotonic()-start,3),phase=phase[0],**value,
                    system_physical_available=memory.physical_available,**paging.sample()))
            if time.monotonic()-start>240:
                proc.terminate();raise TimeoutError('Track memory test exceeded240seconds')
            time.sleep(.1)
    finally:
        reader.join(timeout=10);kernel.CloseHandle(handle);paging.close()
        result=dict(command=cmd,pid=proc.pid,exit_code=proc.poll(),samples=samples,checkpoints=checkpoints)
        (a.output/'measurements.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    log=(a.output/'engine.log').read_text(encoding='utf-8')
    passed=proc.returncode==0 and 'TRACK_MEMORY_OK' in log and 'SCRIPT ERROR:' not in log and '\nERROR:' not in log
    print(json.dumps(dict(exit_code=proc.returncode,passed=passed,samples=len(samples),
        peak_private_mib=round(max((s['private_bytes'] for s in samples),default=0)/1048576,2),
        phases=[v['phase'] for v in checkpoints])))
    raise SystemExit(0 if passed else (proc.returncode or 1))
if __name__=='__main__':main()
