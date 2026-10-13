import assert from 'node:assert/strict';
import {encodeRite,decodeRite} from './holy-rnd.js';
import {readProject,restoreProject,bkCrc,bkU32} from './fm1backup.js';
for(const seed of [0,1,127,128,0x7fffffff,0xffffffff]){
 const args=encodeRite({key:0,scale:4,rite:0,stir:32,tempo:0,seed});
 assert.equal(args.length,12);assert.ok(args.every(n=>n<=127));
 const state=decodeRite([1,0,0,1,1,...args.slice(7),0,4,0,32,0,9,5,72,0]);assert.equal(state.seed,seed);
}
assert.throws(()=>encodeRite({key:13,scale:4,rite:0,stir:32,tempo:0}));
assert.throws(()=>encodeRite({key:0,scale:5,rite:0,stir:32,tempo:0}));
const bytes=new Uint8Array(3840);new DataView(bytes.buffer).setUint32(0,0x46554e42,true);
const file={format:'unholyunion-project',version:1,object:{id:0,size:3840,crc:bkCrc(bytes),data:Buffer.from(bytes).toString('base64')}};
assert.equal(readProject(file).object.bytes.length,3840);
const sent=[];await restoreProject(async ([cmd,args])=>{sent.push([cmd,args]);return [args[0],0,0];},file);
assert.ok(sent.every(([cmd,a])=>cmd===67&&a[1]===0));assert.equal(sent[0][1][0],0);assert.equal(sent.at(-1)[1][0],2);
assert.throws(()=>readProject({...file,object:{...file.object,id:1}}));
assert.throws(()=>readProject({...file,object:{...file.object,crc:1}}));
assert.equal(decodeRite([1,0,0,0,0,0,0,0,0,0,0,4,0,32,0,0,5,44,2]).bpm,300);
assert.throws(()=>decodeRite([1,0]));
console.log('HolyRND recipe protocol and current-project-only import checks passed');
