// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {InstallerState} from './unholy-controller.js';
import {release} from './unholy-release.js';
import {Package} from './fm1pkg.js';
const reviewed={...release,available:true,targetValidated:true,sha256:'reviewed',bytes:10};
let checks=0;function test(name,fn){fn();checks++;console.log('ok '+name);}
const packageOK={product:reviewed.product,sha256:'reviewed',bytes:10};
const s=new InstallerState(reviewed);
test('No connect or flash on creation',()=>assert.equal(s.ready,false));
test('A compatible connection and verified package enable the explicit install action',()=>{s.connect({model:'FM-1',text:'FM-1_015'});assert.equal(s.ready,false);s.load(packageOK);assert.equal(s.ready,true);assert.equal(s.busy,false);});
test('Wrong identity, digest or size rejected before start',()=>{for(const p of [{...packageOK,product:'FM-1_015'},{...packageOK,sha256:'bad'},{...packageOK,bytes:11}])assert.throws(()=>s.load(p));});
test('Cancellation before transfer clears authority',()=>{s.cancel();assert.equal(s.ready,false);assert.equal(s.device,null);assert.equal(s.package,null);});
test('Unknown device cannot arm',()=>{s.connect({model:'other',text:'other'});s.package=packageOK;assert.equal(s.ready,false);assert.throws(()=>s.start());});
test('Disconnect clears package; reconnect requires revalidation',()=>{s.disconnected();s.connect({model:'FM-1',text:'FM-1_015'});assert.equal(s.ready,false);assert.equal(s.package,null);});
test('Active-write cancellation refused',()=>{s.load(packageOK);s.start();s.step('write');assert.throws(()=>s.cancel());assert.equal(s.phase,'write');});
test('Return identity mismatch never becomes success',()=>{assert.throws(()=>s.step('done','FM-1_015'));assert.equal(s.phase,'write');});
test('Write error requires package validation again',()=>{s.fail(Error('lost'));assert.equal(s.phase,'error');assert.equal(s.ready,false);});
test('Unavailable/unvalidated real release cannot load or arm',()=>{const real=new InstallerState(release);real.connect({model:'FM-1',text:'FM-1_015'});assert.throws(()=>real.load(packageOK));assert.equal(real.ready,false);});
test('Corrupt/truncated package rejected by upstream parser',()=>{assert.throws(()=>new Package(new Uint8Array(0)));assert.throws(()=>new Package(new Uint8Array(1500)));});
console.log(`${checks} state/package safety checks passed; hardware NOT exercised.`);
