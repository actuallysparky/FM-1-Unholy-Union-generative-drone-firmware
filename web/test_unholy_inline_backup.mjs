// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {gatherScriptures} from './unholy-inline-backup.js';
import {BACKUP_IDS,bkCrc,bkU32,bkR32,bkPack} from './fm1backup.js';
function fixture(corrupt=false){
 const objects=new Map([[0,new Uint8Array([1,2,3])],[1,new Uint8Array([4,5])]]),sent=[];
 const previous=()=>{},input={name:'FM-1',state:'connected',onmidimessage:previous,open:async()=>{}};
 const output={name:'FM-1',state:'connected',open:async()=>{},send(frame){
  const cmd=frame[4],args=frame.slice(5,-1);sent.push(cmd);let answer;
  if(cmd===65){answer=[1,0,BACKUP_IDS.length];for(const id of BACKUP_IDS){const b=objects.get(id)||new Uint8Array();answer.push(id,...bkU32(b.length),...bkU32(b.length?bkCrc(b):0));}}
  else if(cmd===66){const id=args[0],off=bkR32(args,1),size=args[6]|args[7]<<7,b=objects.get(id).slice();if(corrupt)b[0]^=1;answer=[id,0,...bkU32(off),size&127,size>>>7,...bkPack(b.subarray(off,off+size))];}
  else throw Error('Unexpected write');
  queueMicrotask(()=>input.onmidimessage({data:Uint8Array.from([240,125,70,76,cmd,...answer,247])}));
 }};
 return {access:{inputs:new Map([[1,input]]),outputs:new Map([[1,output]])},sent,input,previous};
}
let f=fixture(),downloads=0;
const archive=await gatherScriptures(f.access,'FM-1_80010',file=>{downloads++;assert.equal(file.format,'felucca-backup');});
assert.equal(downloads,1);assert.equal(archive.objects.length,13);assert.ok(f.sent.every(cmd=>cmd===65||cmd===66));assert.equal(f.input.onmidimessage,f.previous);
f=fixture(true);await assert.rejects(gatherScriptures(f.access,'TEST',()=>{downloads++;}),/changed/);assert.equal(downloads,1);assert.equal(f.input.onmidimessage,f.previous);
f=fixture();await assert.rejects(gatherScriptures(f.access,'TEST',()=>{throw Error('Download blocked');}),/Download blocked/);assert.equal(f.input.onmidimessage,f.previous);
await assert.rejects(gatherScriptures({inputs:new Map(),outputs:new Map()},'TEST',()=>{}),/exactly one/);
console.log('Inline backup: complete read-only archive, download after validation, failure cleanup and ambiguous port rejection passed.');
