// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {selectChannel,defaultChannel} from './unholy-channels.js';
import {InstallerState} from './unholy-controller.js';
const stable=selectChannel('stable'),experiment=selectChannel('experimental');
assert.equal(stable.package,null);assert.equal(stable.release.available,false);
assert.equal(defaultChannel(),'experimental');assert.throws(()=>selectChannel('__proto__'));
const state=new InstallerState(experiment.release,{experimental:true});
assert.equal(state.authorized,true);assert.equal(experiment.release.targetValidated,false);
state.connect({model:'FM-1',text:'FM-1_014'});
state.load({product:experiment.package.product,sha256:experiment.package.sha256,bytes:experiment.package.bytes});
assert.equal(state.ready,false);state.backup=true;assert.equal(state.ready,false);state.consent=true;assert.equal(state.ready,true);
const upgrade=new InstallerState(experiment.release,{experimental:true});upgrade.connect({model:'FM-1',text:'FM-1_80010'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80011'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80012'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80013'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80014'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80015'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80016'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80017'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80018'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80019'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80020'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80021'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80022'});assert.equal(upgrade.compatible,true);
for(const text of ['FM-1_015',experiment.package.product]){const other=new InstallerState(experiment.release,{experimental:true});other.connect({model:'FM-1',text});assert.equal(other.compatible,false);}
state.start();assert.throws(()=>state.cancel());

// Exercise real browser initialization and channel switching with a tiny DOM,
// deferred fetch and zero MIDI access. A late result must not arm another channel.
class Element {
 constructor(){this.listeners={};this.textContent='';this.hidden=false;this.disabled=false;this.checked=false;this.parentElement={lastChild:{textContent:''}};}
 addEventListener(name,fn){this.listeners[name]=fn;}
 setAttribute(){} removeAttribute(){} append(){}
}
const elements=new Map();
globalThis.document={getElementById(id){if(!elements.has(id))elements.set(id,new Element());return elements.get(id);},createElement(){return new Element();},querySelector(){return new Element();}};
globalThis.location={search:''};
let midiCalls=0;
Object.defineProperty(globalThis,'navigator',{configurable:true,value:{requestMIDIAccess(){midiCalls++;throw Error('Unexpected MIDI');}}});
let resolveFetch;
globalThis.fetch=()=>new Promise(resolve=>resolveFetch=resolve);
await import('./unholy-installer.js?channels-test');
assert.equal(midiCalls,0);
elements.get('backup').checked=true;elements.get('backup').listeners.change({target:elements.get('backup')});
elements.get('consent').checked=true;elements.get('consent').listeners.change({target:elements.get('consent')});
elements.get('channel-stable').listeners.click();
const log=elements.get('log');
const previousLog=log.textContent;
log.scrollTop=200;
elements.get('cancel').listeners.click();
assert.equal(log.textContent.slice(log.textContent.indexOf('\n')+1),previousLog);
assert.match(log.textContent.split('\n')[0],/Cancelled before update entry/);
assert.equal(log.scrollTop,0);
assert.equal(elements.get('backup').checked,false);assert.equal(elements.get('consent').checked,false);
const stableMessage=elements.get('package-status').textContent;
const bytes=await readFile(new URL(experiment.package.filename,import.meta.url));
resolveFetch({ok:true,arrayBuffer:async()=>bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength)});
await new Promise(resolve=>setTimeout(resolve,30));
assert.equal(elements.get('package-status').textContent,stableMessage);
assert.equal(elements.get('install').disabled,true);assert.equal(midiCalls,0);
console.log('Channel pins, explicit experimental approval, identity restrictions, consent reset, stale fetch isolation and no automatic MIDI/update passed.');
