#!/usr/bin/env python3
"""Check immutable flash contracts and report build prerequisites; no vendor tools executed."""
import hashlib,json,os,shutil,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=json.loads((ROOT/'provenance.json').read_text());bad=[]
for rel,digest in p['flash_allocation_invariants'].items():
 if hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()!=digest:bad.append(rel)
tc=Path(os.environ.get('JIELI_TOOLCHAIN','/nonexistent'))
sdk=Path(os.environ.get('AC79_SDK','/nonexistent'))
report={'flash_map_byte_identical':not bad,'changed_contracts':bad,'app_slot_bytes':581564,'project_store_bytes':3840,'fm6_offset':3312,'macro_offset':3308,'new_project_marker':'0x46554E42 / FUN11','legacy_marker':'0x46554E41 / FUN10','new_preset_versions':[6,7],'docker_available':bool(shutil.which('docker')),'toolchain_configured':(tc/'pi32v2/bin/clang').exists(),'sdk_configured':sdk.exists(),'target_app_bytes':None,'target_ram_occupancy':None,'hardware_validation':'NOT RUN'}
print(json.dumps(report,indent=2));sys.exit(bool(bad))
