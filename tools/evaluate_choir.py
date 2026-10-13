#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""A/B the Choir Master on identical real-DSP projects; no device access."""
import argparse,concurrent.futures,json,math,os,shlex,statistics,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'build/choir');p.add_argument('--seeds',type=int,default=120);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
cc=shlex.split(os.environ.get('CC','cc'))+['-O2','-w','-DCHOIR_APPLY=1'];sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk')
if sdk.exists():cc+=['-isysroot',str(sdk)]
probe=a.output/'probe_mix';subprocess.run(cc+['-I'+str(ROOT/'build/gen'),'-I'+str(ROOT/'firmware/src'),'-I'+str(ROOT/'tests'),'-o',str(probe),str(ROOT/'tests/choir_probe.c'),'-lm'],cwd=ROOT,check=True)
jobs=[(0x100000+n,0 if n%2==0 else 127,b) for n in range(a.seeds) for b in [0,1]]
def render(j):return json.loads(subprocess.check_output([str(probe),'mix',*map(str,j)],cwd=ROOT,text=True))
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:rows=list(pool.map(render,jobs))
(a.output/'ab-results.json').write_text(json.dumps(rows,indent=2)+'\n')
goal=[-28,-30,-33,-27]
def quant(xs,p):xs=sorted(xs);return xs[round((len(xs)-1)*p)]
summary={}
for b in [0,1]:
 data=[r for r in rows if r['balanced']==b];errs=[];spreads=[]
 for r in data:
  error=[x-y for x,y in zip(r['energy_db'][:4],goal)];errs.append(sum(map(abs,error))/4);spreads.append(max(error)-min(error))
 summary['balanced' if b else 'previous']={'mean_role_error_db':statistics.mean(errs),'median_role_error_db':statistics.median(errs),'p95_role_error_db':quant(errs,.95),'median_role_spread_db':statistics.median(spreads),'p95_role_spread_db':quant(spreads,.95),'max_pre_limiter_peak_dbfs':max(r['peak_db'][4] for r in data),'median_pre_limiter_peak_dbfs':statistics.median(r['peak_db'][4] for r in data),'limiter_samples':sum(r['limiter_samples'] for r in data),'maxvoices':max(r['maxvoices'] for r in data),'quietest_active_stem_db':min(min(r['energy_db'][:4]) for r in data)}
assert summary['balanced']['mean_role_error_db']<summary['previous']['mean_role_error_db']
assert summary['balanced']['p95_role_error_db']<summary['previous']['p95_role_error_db']
assert summary['balanced']['maxvoices']<=8
assert summary['balanced']['limiter_samples']<=summary['previous']['limiter_samples']
(a.output/'ab-summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary),flush=True)
# Independent live-register/maximum-velocity stress; same actual wet mix, pre limiter.
def stress(n):
 return json.loads(subprocess.check_output([str(probe),'mix',str(0x200000+n),'127','1','-',str(48 if n%2==0 else 84),'127'],cwd=ROOT,text=True))
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:stress_rows=list(pool.map(stress,range(min(a.seeds,60))))
assert max(r['maxvoices'] for r in stress_rows)<=8
stress_summary={'cases':len(stress_rows),'velocity':127,'notes':[48,84],'max_pre_limiter_peak_dbfs':max(r['peak_db'][4] for r in stress_rows),'limiter_samples':sum(r['limiter_samples'] for r in stress_rows)}
(a.output/'stress-results.json').write_text(json.dumps(stress_rows,indent=2)+'\n');(a.output/'stress-summary.json').write_text(json.dumps(stress_summary,indent=2)+'\n');print(json.dumps(stress_summary),flush=True)

# Preserve raw/balanced listening excerpts for the worst previous target mismatch.
worst=max((r for r in rows if not r['balanced']),key=lambda r:sum(abs(x-y) for x,y in zip(r['energy_db'][:4],goal)))
for b in [0,1]:subprocess.run([str(probe),'mix',str(worst['seed']),str(worst['stir']),str(b),str(a.output/('choir-after.wav' if b else 'choir-before.wav'))],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
