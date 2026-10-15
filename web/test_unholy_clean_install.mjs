// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {Updater,pack7,unpack7,transferLog} from './fm1ota.js';
import {InstallerState} from './unholy-controller.js';
function request(addr,len){const u=[0,89,48,0,0,0,0,addr&255,addr>>>8&255,addr>>>16&255,addr>>>24&255,len&255,len>>>8,0];u.push(~u.slice(6).reduce((a,b)=>a+b,0)&255);return [240,...pack7(u),247];}
async function serve(wipeUserData){const reads=[[0,512],[0,512],[512,512],[0xf0000000,16],[0xf0000000,8]],sent=[],progress=[];
 const link={read:async()=>request(...reads.shift()),send:p=>{sent.push(unpack7(p.slice(1,-1)));return true;}};
 const result=await new Updater({}).serve(link,new Uint8Array(1024),0xf0000000,1000,(n,a,p)=>progress.push(p),{wipeUserData});
 assert.equal(result.finished,true);assert.equal(sent.length,5);assert.equal(progress[2].sentBytes,1536);assert.equal(progress[2].totalBytes,1024);
 assert.equal(new TextDecoder().decode(Uint8Array.from(sent[3].slice(14,22))),'success\0');
 assert.deepEqual(sent[3].slice(22,30),wipeUserData?[87,73,80,69,65,76,76,0]:new Array(8).fill(0));
 assert.equal(sent[4][11],8);assert.equal(progress.some(p=>p.wiping),wipeUserData);
 assert.match(transferLog('write',3,progress[2]),/2 KiB sent \/ 1 KiB package/);
}
await serve(false);await serve(true);
let upgrades=0;const old={id:{text:'ota-FM-1_900'},link:{close(){},send(){upgrades++;}}};
await assert.rejects(new Updater({}).write(old,new Uint8Array(),()=>{},{wipeUserData:true}),/cannot erase/);assert.equal(upgrades,0);
const release={available:true,targetValidated:true,product:'FM-1_80022',sha256:'x',bytes:1,wipeUserData:true,experimentalApproval:{enabled:true,model:'FM-1',supportedStartingIdentities:['FM-1_80021']}};
const s=new InstallerState(release,{experimental:true});s.connect({model:'FM-1',text:release.product});assert.equal(s.compatible,false);s.wipeUserData=true;assert.equal(s.compatible,true);s.load({product:release.product,sha256:'x',bytes:1});assert.equal(s.ready,true);s.start();s.step('wipe');assert.equal(s.busy,true);assert.throws(()=>s.cancel());s.fail(Error('no'));assert.equal(s.wipeUserData,false);
s.connect({model:'ota-FM-1',text:'ota-FM-1_901'});assert.equal(s.compatible,true);s.connect({model:'ota-FM-1',text:'ota-FM-1_999'});assert.equal(s.compatible,false);
console.log('Clean install: explicit/default tokens, byte counts with rereads, final acknowledgement, old-loader refusal and same-identity opt-in gates pass.');
