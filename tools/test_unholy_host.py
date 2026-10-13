#!/usr/bin/env python3
"""Sequential native validation; no target compiler, connected hardware or installable image."""
import argparse,json,os,shlex,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--generate',action='store_true');p.add_argument('--tests',nargs='*');p.add_argument('--run-existing',action='store_true',help='Replay previously built native binaries; not a compilation claim');a=p.parse_args()
os.chdir(ROOT);out=ROOT/'build/host';out.mkdir(parents=True,exist_ok=True)
if a.generate:
 (ROOT/'build/gen').mkdir(parents=True,exist_ok=True)
 for tool,dest,extra in [('gen_aa_font','ui_fonts',['--preset','inter-tight']),('gen_aa_icons','ui_icons',[]),('gen_aa_keycaps','ui_keycaps',[]),('gen_ui_palettes','ui_palettes',[]),('gen_tables','felucca_tables',[]),('gen_fm6_patches','felucca_fm6',[]),('gen_samples','felucca_samples',[])]:
  subprocess.run([sys.executable,f'tools/{tool}.py',f'build/gen/{dest}.h',*extra],check=True)
cc=shlex.split(os.environ.get('CC','/usr/bin/clang'))
# Current machine's SDK 27 TAPI is incompatible with its linker; SDK 26.5 is already installed.
if sys.platform=='darwin' and Path('/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk').exists(): cc += ['-isysroot','/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk']
DEFAULT=['flash_live','live_save','storage','upreset','input','midi_uart','scale','chord','project','motion','ratchet','step15','detail','fun10','midi_control','theme','text_ref','settings','ui','audio','click','persistence','backup','fm6bank','editor','robust','mod','sound14','insert','sound15','perform','reverb','slice','sample','fm6','noise','drum','unholy','starter','holy','permutation_ui','choir']
# Generate real current descriptors before web parity; never rely on a stale mock-only pass.
desc=out/'descdump'
subprocess.run(cc+['-O2','-w','-Ibuild/gen','-Ifirmware/src','-Itests','-o',str(desc),'tests/descdump.c','-lm'],check=True)
with (out/'desc.json').open('w') as stream: subprocess.run([str(desc)],stdout=stream,check=True)
results=[]
for name in (a.tests or DEFAULT):
 src=ROOT/f'tests/{name}_test.c';exe=out/f'{name}_test';log=out/f'{name}_test.log';start=time.time()
 args=[]
 if name=='slice':
  Path('build/slice_demo').mkdir(parents=True,exist_ok=True)
  subprocess.run([sys.executable,'tests/slice_loop.py','build/slice_demo/loop'],check=True)
  subprocess.run([sys.executable,'tools/fm1_sample_upload.py','build','LOOP','build/slice_demo/loop','build/slice_demo/loop.wav:60'],check=True,stdout=subprocess.DEVNULL)
  args=['build/slice_demo/loop','build/slice_demo']
 if name=='sample':
  Path('build/piano_hd').mkdir(parents=True,exist_ok=True);Path('build/sample_demo').mkdir(parents=True,exist_ok=True)
  subprocess.run([sys.executable,'tools/gen_samples.py','--user-slot','PIANO','PIANO HD','build/piano_hd/PIANO_HD'],check=True,stdout=subprocess.DEVNULL)
  args=['build/piano_hd/PIANO_HD','build/sample_demo']
 cmd=cc+['-O2','-w','-Ibuild/gen','-Ifirmware/src','-Itests','-o',str(exe),str(src),'-lm']
 with log.open('w') as stream:
  rc=0 if a.run_existing and exe.exists() else subprocess.run(cmd,stdout=stream,stderr=subprocess.STDOUT).returncode
  if rc==0: rc=subprocess.run([str(exe),*args],stdout=stream,stderr=subprocess.STDOUT).returncode
 row={'suite':name,'returncode':rc,'seconds':round(time.time()-start,2),'log':str(log.relative_to(ROOT))};results.append(row)
 print(json.dumps(row),flush=True)
(out/'unholy-results.json').write_text(json.dumps({'target_build':'NOT RUN by this host-only runner; approved cloud results are separate','hardware':'NOT RUN','suites':results},indent=2)+'\n')
sys.exit(any(r['returncode'] for r in results))
