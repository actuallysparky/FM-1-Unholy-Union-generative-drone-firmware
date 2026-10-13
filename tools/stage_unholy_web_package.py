#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage the single pinned private preview package. Copies bytes only; no device access."""
from pathlib import Path
import hashlib,shutil
ROOT=Path(__file__).resolve().parents[1]
source=ROOT/'build/validated/a3e780b/firmware/unholyunion-experimental.fwsc'
data=source.read_bytes()
if len(data)!=610086 or hashlib.sha256(data).hexdigest()!='0f0685775ba9e0a00b1bfb03c1a3c70b188a712d0752689d0a63ae93cb38f517':
 raise SystemExit('Validated source differs from pinned a3e780b package; refusing to stage.')
destination=ROOT/'web/unholyunion-a3e780b.fwsc'
shutil.copyfile(source,destination)
print(f'Staged {len(data)} verified bytes at {destination}; installation gates unchanged.')
