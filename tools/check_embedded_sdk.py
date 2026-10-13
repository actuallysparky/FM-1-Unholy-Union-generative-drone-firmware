#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only verification of SDK blobs embedded in a retained author package."""
import argparse, binascii, hashlib, json, struct
from pathlib import Path

def dec(data,key=65535):
    out=bytearray(data)
    for i in range(len(out)):
        out[i]^=key&255
        key=((key<<1)^(0x1021 if key&0x8000 else 0))&65535
    return bytes(out)

def crc(data): return binascii.crc_hqx(data,0)
def entry(raw):
    assert crc(raw[2:])==struct.unpack_from('<H',raw)[0]
    _,dc,off,size,flags,resvd,index,name=struct.unpack('<HHIIBBH16s',raw)
    return name.split(b'\0')[0].decode(),dc,off,size

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('package',type=Path,help='retained author Felucca v1.5.1 package; read only')
p=parser.parse_args().package
raw=p.read_bytes()
logical=b''.join(raw[i*48:i*48+47] for i in range(20))+raw[960:]
h=dec(logical[:64]);hc,lc,total,count=struct.unpack_from('<HHIH',h)
assert total==len(logical) and crc(h[2:])==hc and crc(logical[64:64+count*80])==lc
flash=None
for i in range(count):
    e=dec(logical[64+i*80:64+(i+1)*80]);off,size=struct.unpack_from('<II',e,8)
    d=logical[off:off+size];assert crc(d)==struct.unpack_from('<H',e,4)[0]
    if e[64:].split(b'\0')[0]==b'flash.bin':flash=d
assert flash is not None
name,c,off,size=entry(dec(flash[32:64]));assert name=='uboot.boot'
blobs={name:flash[off:off+size]};assert crc(blobs[name])==c
area=b''.join(dec(flash[0x4000+i:0x4000+i+32],0x980f^(i>>2)) for i in range(0,len(flash)-0x4000,32))
_,_,_,block_size=entry(area[:32])
name,c,off,size=entry(area[64:96]);assert name=='cfg_tool.bin'
blobs[name]=area[off:off+size];assert crc(blobs[name])==c
cfg=area[block_size:];name,c,off,size=entry(cfg[32:64]);assert name=='eq_cfg_hw.bin'
blobs['cfg/'+name]=cfg[off:off+size];assert crc(blobs['cfg/'+name])==c
expected={'uboot.boot':'4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4','cfg_tool.bin':'276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b','cfg/eq_cfg_hw.bin':'41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d'}
results=[]
for name,data in blobs.items():
    sha=hashlib.sha256(data).hexdigest();assert sha==expected[name]
    results.append({'path':'cpu/wl82/tools/'+name,'bytes':len(data),'sha256':sha,'matches_pinned_sdk':True})
print(json.dumps({'package':str(p),'package_sha256':hashlib.sha256(raw).hexdigest(),'blobs':results,'total_bytes':sum(map(len,blobs.values())),'vendor_binary_executed':False,'blobs_written':False},indent=2))
