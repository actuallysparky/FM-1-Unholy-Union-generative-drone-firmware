// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {bundledPackage as expected,release} from './unholy-release.js';
import {verifyBundledPackage,fetchBundledPackage} from './unholy-bundled-package.js';
import {InstallerState} from './unholy-controller.js';
const bytes=new Uint8Array(await readFile(new URL(expected.filename,import.meta.url)));
let checks=0;
async function test(name,fn){await fn();checks++;console.log('ok '+name);}
await test('Exact staged package verifies size, SHA256, identity and parser integrity',async()=>{
 const result=await verifyBundledPackage(bytes,expected);
 assert.equal(result.product,'FM-1_80010');assert.equal(result.sha256,expected.sha256);
 assert.equal(result.bytes,610086);assert.ok(result.image.length>0);
});
await test('Same-size corruption is blocked',async()=>{const bad=bytes.slice();bad[100]^=1;await assert.rejects(()=>verifyBundledPackage(bad,expected),/checksum mismatch/);});
await test('Truncation is blocked',async()=>{await assert.rejects(()=>verifyBundledPackage(bytes.slice(1),expected),/size mismatch/);});
await test('Wrong product is blocked after exact checksum verification',async()=>{await assert.rejects(()=>verifyBundledPackage(bytes,{...expected,product:'FM-1_015'}),/identity mismatch/);});
await test('HTTP failure gives a repairable load error',async()=>{await assert.rejects(()=>fetchBundledPackage(expected,async()=>({ok:false,status:404})),/HTTP 404/);});
await test('Network failure gives a clear blocked status',async()=>{await assert.rejects(()=>fetchBundledPackage(expected,async()=>{throw Error('fetch failed');}),/local preview server/);});
await test('Fetch uses sole package and bypasses stale cache',async()=>{await fetchBundledPackage(expected,async(url,options)=>{assert.equal(url,expected.filename);assert.equal(options.cache,'no-store');return {ok:true,arrayBuffer:async()=>bytes.buffer};});});
await test('Verified bundled bytes cannot unlock the experimental release',async()=>{
 const verified=await verifyBundledPackage(bytes,expected);const state=new InstallerState(release);
 state.connect({model:'FM-1',text:'FM-1_014'});state.backup=state.consent=true;
 assert.throws(()=>state.load(verified));assert.equal(state.ready,false);assert.throws(()=>state.start());
 assert.equal(release.available,false);assert.equal(release.targetValidated,false);assert.equal(release.sha256,null);
});
console.log(`${checks} bundled package checks passed; no MIDI or hardware used.`);
