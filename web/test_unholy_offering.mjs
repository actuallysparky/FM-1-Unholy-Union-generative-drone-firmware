// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {offering} from './unholy-offering.js';
import {InstallerState} from './unholy-controller.js';
import {Updater} from './fm1ota.js?v=coverage-1';
const html=await readFile(new URL('unholy-installer.html',import.meta.url),'utf8');
assert.doesNotMatch(html,/type=["']checkbox|Choose your testament|channel-stable|channel-experimental/);
assert.equal((html.match(/class="number"/g)||[]).length,3);
assert.match(html,/Optional\. Connect your FM-1/);
assert.equal(offering.package.build,'permutation-0.2.4');
assert.equal(offering.package.product,'FM-1_80024');
const pkg={product:offering.package.product,sha256:offering.package.sha256,bytes:offering.package.bytes};
for(const text of ['FM-1_014','FM-1_015','FM-1_80010','FM-1_80022','FM-1_80023','FM-1_80024']){
 const s=new InstallerState(offering.release,{experimental:!offering.release.targetValidated});s.connect({model:'FM-1',text});s.load(pkg);assert.equal(s.ready,true);assert.equal(s.busy,false);assert.equal(s.wipeUserData,false);
}
for(const id of [{model:'other',text:'FM-1_014'},{model:'FM-1',text:'FM-1_016'},{model:'FM-1',text:'FM-1_0150'},{model:'other',text:'FM-1_015'},{model:'FM-1',text:'FM-1_80025'},{model:'ota-FM-1',text:'ota-FM-1_999'}]){
 const s=new InstallerState(offering.release,{experimental:!offering.release.targetValidated});s.connect(id);assert.equal(s.compatible,false);assert.equal(s.ready,false);assert.throws(()=>s.start());
}
// V15 approval never relaxes package identity, digest or size verification.
for(const bad of [{...pkg,product:'FM-1_015'},{...pkg,sha256:'bad'},{...pkg,bytes:pkg.bytes+1}]){
 const s=new InstallerState(offering.release,{experimental:true});s.connect({model:'FM-1',text:'FM-1_015'});
 assert.throws(()=>s.load(bad));assert.equal(s.ready,false);assert.throws(()=>s.start());
}
const simulated=new InstallerState({...offering.release,targetValidated:true,sha256:'SIMULATION',bytes:1},{simulation:true});
simulated.connect({model:'FM-1',text:'FM-1_SIMULATED'});simulated.load({product:offering.package.product,sha256:'SIMULATION',bytes:1});assert.equal(simulated.ready,true);
class Element{
 constructor(){this.listeners={};this.textContent='';this.disabled=false;this.hidden=false;this.scrollTop=0;}
 addEventListener(n,fn){this.listeners[n]=fn;}setAttribute(){}removeAttribute(){}
}
const elements=new Map([...html.matchAll(/\bid="([^"]+)"/g)].map(m=>[m[1],new Element()]));
globalThis.document={getElementById:id=>elements.get(id)||null};
globalThis.location={search:'?channel=stable&wipe=true'};
globalThis.isSecureContext=true;
let midiCalls=0,writes=0,finish;
const access={sysexEnabled:true,inputs:new Map([['in',{state:'connected',name:'BlessedMIDI'}]]),outputs:new Map()};
Object.defineProperty(globalThis,'navigator',{configurable:true,value:{async requestMIDIAccess(){midiCalls++;return access;}}});
const bytes=await readFile(new URL(offering.package.filename,import.meta.url));
let resolveFetch;
globalThis.fetch=()=>new Promise(resolve=>resolveFetch=()=>resolve({ok:true,arrayBuffer:async()=>bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength)}));
let finishIdentity;let firstIdentity=true;let startingIdentity='FM-1_015';
Updater.prototype.find=async()=>{if(firstIdentity){firstIdentity=false;await new Promise(resolve=>finishIdentity=resolve);}return {id:{model:'FM-1',text:startingIdentity},link:{close(){}}};};
Updater.prototype.install=async(image,product,step,options)=>{writes++;assert.equal(image.length>0,true);assert.deepEqual(options,{wipeUserData:false});step('write',1,{sentBytes:512,totalBytes:bytes.length});await new Promise(resolve=>finish=()=>{step('done',product);resolve();});};
await import('./unholy-installer.js?single-offering-test');
assert.equal(midiCalls,0);assert.equal(writes,0);
elements.get('cancel').listeners.click();resolveFetch();
await new Promise(resolve=>setTimeout(resolve,40));
assert.equal(elements.get('phase').textContent,'Session cancelled');
assert.equal(midiCalls,0);assert.equal(writes,0);assert.equal(elements.get('install').disabled,true);
const connecting=elements.get('connect').listeners.click();
await new Promise(resolve=>setTimeout(resolve,0));assert.equal(elements.get('connect').disabled,true);assert.equal(elements.get('install').disabled,true);
await elements.get('connect').listeners.click();assert.equal(midiCalls,1);finishIdentity();await connecting;
assert.equal(elements.get('connect').hidden,true);
assert.equal(midiCalls,1);assert.equal(writes,0);assert.equal(elements.get('install').disabled,false,elements.get('package-status').textContent+' / '+elements.get('status').textContent);
assert.equal(elements.get('install').textContent,'Install Apocryphal');
// Stock V15 is admitted without pretending to provide a complete stock backup.
assert.equal(elements.get('download-backup').disabled,true);
assert.match(elements.get('backup-status').textContent,/cannot export a complete backup from stock/);
assert.match(elements.get('install-notes').textContent,/not converted/);
await elements.get('download-backup').listeners.click();assert.equal(writes,0);
const stockWriting=elements.get('install').listeners.click();assert.equal(writes,1);
finish();await stockWriting;assert.match(elements.get('status').textContent,/PLAY to pray/);
elements.get('cancel').listeners.click();startingIdentity='FM-1_80023';await elements.get('connect').listeners.click();
assert.equal(elements.get('download-backup').disabled,false);
assert.match(elements.get('install-notes').textContent,/saved UnholyUnion music/);
// Backup remains optional even when an attempted backup fails.
await elements.get('download-backup').listeners.click();
assert.match(elements.get('backup-status').textContent,/could not be gathered/);
assert.equal(elements.get('install').disabled,false);assert.equal(writes,1);
const writing=elements.get('install').listeners.click();
assert.equal(writes,2);assert.equal(elements.get('connect').disabled,true);assert.equal(elements.get('download-backup').disabled,true);assert.equal(elements.get('install').disabled,true);
finish();await writing;assert.match(elements.get('status').textContent,/PLAY to pray/);
const log=elements.get('log');const oldLog=log.textContent;log.scrollTop=200;elements.get('cancel').listeners.click();
assert.equal(log.scrollTop,0);assert.equal(log.textContent.slice(log.textContent.indexOf('\n')+1),oldLog);assert.equal(elements.get('install').disabled,true);
// V16 stays blocked, and diagnostics include its actual identity.
startingIdentity='FM-1_016';await elements.get('connect').listeners.click();
assert.equal(elements.get('install').disabled,true);assert.equal(elements.get('download-backup').disabled,true);
assert.match(elements.get('status').textContent,/FM-1_016/);
assert.match(elements.get('status').textContent,/No update has been started/);
await elements.get('install').listeners.click();assert.equal(writes,2);
elements.get('cancel').listeners.click();startingIdentity='FM-1_80023';
// A changed device set is caught again immediately before the write.
await elements.get('connect').listeners.click();access.inputs.set('second',{state:'connected',name:'BlessedMIDI 2'});
await elements.get('install').listeners.click();assert.equal(writes,2);assert.match(elements.get('status').textContent,/device set changed/);
console.log('V14/V15 admitted, V16 blocked, stock backup limits; single pinned offering: optional/failed backup, no acknowledgements, explicit install only, preserving writes, repeat install, busy/disconnect and device-set checks pass.');
