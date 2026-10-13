#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build with the reviewed compiler in the existing isolated unholy-build VM.
Does not create a VM, install software, publish or access an instrument. The VM
mount must expose this repository read-only as /mnt/unholy-source. Setup and
input provenance are recorded in the canonical UnholyUnion build documentation.
"""
import argparse, hashlib, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--release',default='0.1.1')
p.add_argument('--compiler-archive',required=True,type=Path)
p.add_argument('--sdk',required=True,type=Path)
p.add_argument('--reproduce',action='store_true',help='Also build in a separate guest directory and compare app, ELF, loader and package bytes')
a=p.parse_args()
import re
if not re.fullmatch(r'\d\.\d\.\d',a.release):p.error('release must be X.Y.Z with single digits')
expected='a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd'
if hashlib.sha256(a.compiler_archive.read_bytes()).hexdigest()!=expected:p.error('compiler archive differs from reviewed 20260730.1 input')
sdk_pins={'uboot.boot':'4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4','cfg_tool.bin':'276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b','cfg/eq_cfg_hw.bin':'41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d'}
for name,digest in sdk_pins.items():
    if hashlib.sha256((a.sdk/'cpu/wl82/tools'/name).read_bytes()).hexdigest()!=digest:p.error('SDK input differs: '+name)
inputs=ROOT/'build/linux-input';inputs.mkdir(parents=True,exist_ok=True)
shutil.copyfile(a.compiler_archive,inputs/'jieli-linux-toolchains-20260730.1.tar.xz')
shutil.copytree(a.sdk,inputs/'sdk-data',dirs_exist_ok=True)
script=inputs/'build-reviewed.sh'
script.write_text('''#!/bin/bash
set -eu
mkdir -p /tmp/unholy-build/source /tmp/unholy-build/compiler
cd /tmp/unholy-build/source
cp -a /mnt/unholy-source/firmware /mnt/unholy-source/tools /mnt/unholy-source/assets /mnt/unholy-source/tests /mnt/unholy-source/LICENSES .
cp /mnt/unholy-source/BUILDING.md /mnt/unholy-source/LICENSE /mnt/unholy-source/LICENSING.md /mnt/unholy-source/README.md .
mkdir -p web
cp /mnt/unholy-source/web/fukiai.ttf web/
cp -a /mnt/unholy-source/build/linux-input/sdk-data /tmp/unholy-build/
tar -xJf /mnt/unholy-source/build/linux-input/jieli-linux-toolchains-20260730.1.tar.xz -C /tmp/unholy-build/compiler
export JIELI_TOOLCHAIN=/tmp/unholy-build/compiler/jieli-linux-toolchains-20260730.1
export AC79_SDK=/tmp/unholy-build/sdk-data
export JIELI_DOCKER=0
python3 tools/build.py --release '''+a.release+'\n'+('unholy_repro_dir=$(mktemp -d /tmp/unholy-repro.XXXXXX)\ncp -a /tmp/unholy-build/source/. "$unholy_repro_dir"\ncd "$unholy_repro_dir"\npython3 tools/build.py --release '+a.release+'\nfor artifact in felucca.bin felucca.elf loader/ota.bin unholyunion-'+a.release+'-experimental.fwsc; do cmp /tmp/unholy-build/source/build/"$artifact" build/"$artifact"; done\necho "Independent-directory reproducibility: PASS"\n' if a.reproduce else ''))
lima=shutil.which('limactl') or '/opt/homebrew/bin/limactl'
subprocess.run([lima,'shell','unholy-build','bash','/mnt/unholy-source/build/linux-input/build-reviewed.sh'],cwd='/tmp',check=True)
out=ROOT/'build/linux-output';out.mkdir(exist_ok=True)
subprocess.run([lima,'copy','-r','unholy-build:/tmp/unholy-build/source/build/.',str(out)],cwd='/tmp',check=True)
print('Target build outputs:',out)
