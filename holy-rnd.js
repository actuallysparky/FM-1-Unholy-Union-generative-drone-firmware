// SPDX-License-Identifier: GPL-3.0-only
// The device composes the music. These helpers encode recipes, never synthesize
// a browser approximation of the firmware's musical algorithm.
export const HOLY_CMD = {GET:82, GENERATE:83, UNDO:84};
export const HOLY_SCALES = ['AUTO','Minor','Dorian','Minor pentatonic','Major pentatonic'];
export function encodeRite(r) {
 const valid=(n,lo,hi)=>Number.isInteger(n)&&n>=lo&&n<=hi;
 if(!valid(r.key,0,12)||!valid(r.scale,0,4)||!valid(r.rite,0,2)||!valid(r.stir,0,127)||!(r.tempo===0||valid(r.tempo,40,110)))throw Error('Invalid HolyRND recipe');
 const given=r.seed!==null&&r.seed!==undefined;
 if(given&&!valid(r.seed,0,0xffffffff))throw Error('Seed must be an unsigned 32-bit integer');
 const seed=given?r.seed:0;
 return [1,r.key,r.scale,r.rite,r.stir,r.tempo,given?1:0,...Array.from({length:5},(_,i)=>(seed>>>(7*i))&(i===4?15:127))];
}
export function decodeRite(a) {
 if(a.length!==19||a[0]!==1||a[9]>15)throw Error('Unsupported HolyRND state');
 return {phase:a[1],status:a[2],undo:!!a[3],hasSeed:!!a[4],seed:(a[5]|a[6]<<7|a[7]<<14|a[8]<<21|a[9]<<28)>>>0,
 key:a[10],scale:a[11],rite:a[12],stir:a[13],tempo:a[14],root:a[15],scaleId:a[16],bpm:a[17]|a[18]<<7};
}
export const HOLY_ERRORS=['','Release held keys or finish the active transfer.','Invalid recipe.','No rite to undo.','Generated project was invalid.','Pitched sample source unavailable.','The rite timed out; transport is stopped.','Undo would replace later edits; confirm to continue.'];
