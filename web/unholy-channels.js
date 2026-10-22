// SPDX-License-Identifier: GPL-3.0-only
import {release} from './unholy-release.js?v=channels-1';

import {latestExperimental} from './unholy-catalog.js?v=holy-1';
const bundledPackage=latestExperimental.package;
// Promotion is explicit. A successful new build updates Experimental, never Stable.
export const channels = Object.freeze({
 stable:Object.freeze({id:'stable',title:'The Canon',description:'Stable · the explicitly approved, tested build.',package:null,
  release:Object.freeze({...release,name:'UnholyUnion Stable',available:false})}),
 experimental:Object.freeze({id:'experimental',title:'The Apocrypha',description:'Experimental · the latest checked firmware package.',package:bundledPackage,
  release:Object.freeze({...release,product:bundledPackage.product,name:'UnholyUnion '+bundledPackage.version,available:true,
   sha256:bundledPackage.sha256,bytes:bundledPackage.bytes,filename:bundledPackage.filename,wipeUserData:bundledPackage.wipeUserData===true,
   experimentalApproval:Object.freeze({enabled:true,build:bundledPackage.build,model:'FM-1',supportedStartingIdentities:Object.freeze(['FM-1_014','FM-1_015','FM-1_80010','FM-1_80011','FM-1_80012','FM-1_80013','FM-1_80014','FM-1_80015','FM-1_80016','FM-1_80017','FM-1_80018','FM-1_80019','FM-1_80020','FM-1_80021','FM-1_80022'])}),
   hardwareEvidence:'Immaculate Permutation and Evening Vow included. Host audio, reproducibility, package integrity and static target checks pass. Hardware test pending.'})})
});

export function selectChannel(id) {
 if(!Object.hasOwn(channels,id))throw Error('Unknown firmware channel.');
 return channels[id];
}
export function defaultChannel() { return channels.stable.package&&channels.stable.release.available&&channels.stable.release.targetValidated?'stable':'experimental'; }
