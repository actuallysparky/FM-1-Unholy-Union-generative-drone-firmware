// SPDX-License-Identifier: GPL-3.0-only
// Mock transport only. No Web MIDI permission, device access, or native updater.
import assert from 'node:assert/strict';
import {Updater} from './fm1ota.js';
const originalTimer=globalThis.setTimeout;
globalThis.setTimeout=(fn,ms,...args)=>originalTimer(fn,Math.min(ms,1),...args);
const product='FM-1_80010',image=new Uint8Array(100);
let checks=0;
const device=(text='FM-1_014')=>({id:{text,model:text.split('_')[0]},link:{send:()=>true,close:()=>{}}});
function mock({first={finished:true},second={finished:true},loader=true,returned=product}={}){
 const updater=new Updater({});let reads=0,waits=0;
 updater.find=async()=>device();
 updater.serve=async()=>{const result=reads++===0?first:second;if(result instanceof Error)throw result;return {served:2,lost:false,...result};};
 updater.waitFor=async()=>waits++===0?(loader?device('ota-'+product):null):(returned?device(returned):null);
 return updater;
}
async function test(name,fn){await fn();checks++;console.log('ok '+name);}
const failure=async(fn)=>{try{await fn();assert.fail('Expected error');}catch(e){assert.notEqual(e.code,'ERR_ASSERTION');return e;}};
try{
 await test('Step-1 disconnect admits metadata writes and conditional recovery',async()=>{const e=await failure(()=>mock({first:{finished:false,lost:true}}).install(image,product));assert.equal(e.code,'lost');assert.match(e.message,/Loader or boot metadata may already have been written/);assert.match(e.message,/did not start the bulk application transfer/);assert.doesNotMatch(e.message,/nothing was written/);assert.match(e.message,/only if running firmware or the MIDI update loader is reachable/);});
 await test('Step-1 malformed request retains stage and original code',async()=>{const bad=Object.assign(Error('Bad request'),{code:'badreq'});const e=await failure(()=>mock({first:bad}).install(image,product));assert.equal(e.code,'badreq');assert.match(e.message,/boot metadata/);});
 await test('Missing loader does not promise persistent update mode',async()=>{const e=await failure(()=>mock({loader:false}).install(image,product));assert.equal(e.code,'noloader');assert.match(e.message,/boot metadata/);assert.doesNotMatch(e.message,/stays in update mode/);});
 await test('Step-2 stop warns about incomplete firmware',async()=>{const e=await failure(()=>mock({second:{finished:false}}).install(image,product));assert.equal(e.code,'stopped');assert.match(e.message,/Firmware writes may be incomplete/);assert.match(e.message,/only if the MIDI update loader is reachable/);});
 await test('Step-2 malformed request is not presented as pre-write refusal',async()=>{const bad=Object.assign(Error('Bad request'),{code:'badreq'});const e=await failure(()=>mock({second:bad}).install(image,product));assert.match(e.message,/Firmware writes may be incomplete/);});
 await test('No returned identity means transfer completed, not success',async()=>{const e=await failure(()=>mock({returned:null}).install(image,product));assert.equal(e.code,'noreturn');assert.match(e.message,/transfer finished/);assert.match(e.message,/Do not report success/);assert.doesNotMatch(e.message,/power-cycle/);});
 await test('Identity mismatch is explicit and retains actual identity',async()=>{const e=await failure(()=>mock({returned:'FM-1_014'}).install(image,product));assert.equal(e.code,'mismatch');assert.equal(e.detail,'FM-1_014');assert.match(e.message,/expected FM-1_80010/);assert.match(e.message,/Do not report success/);});
 await test('Resume false exposes incomplete-write recovery to caller',async()=>{const u=mock({first:{finished:false,lost:true}});u.find=async()=>device('ota-'+product);assert.equal(await u.resume(image,()=>{},product),false);assert.equal(u.lastFailure.code,'lost');assert.match(u.lastFailure.message,/Firmware writes may be incomplete/);});
 await test('Resume without loader separates prior uncertainty from no new update command',async()=>{const u=mock();u.find=async()=>null;assert.equal(await u.resume(image,()=>{},product),false);assert.match(u.lastFailure.message,/No new update command was sent/);assert.match(u.lastFailure.message,/previous session/);});
 await test('No device before install states no update command was sent',async()=>{const u=mock();u.find=async()=>null;const e=await failure(()=>u.install(image,product));assert.equal(e.code,'notfound');assert.match(e.message,/No update command was sent/);});
}finally{globalThis.setTimeout=originalTimer;}
console.log(`${checks} stage-aware failure checks passed; mock transport only.`);
