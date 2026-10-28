// SPDX-License-Identifier: GPL-3.0-only
// Public offering is pinned independently of the local experimental catalog.
const startingIdentities=Object.freeze(['FM-1_014','FM-1_015',...Array.from({length:15},(_,i)=>'FM-1_'+(80010+i))]);
const pkg=Object.freeze({"version":"0.2.4","build":"permutation-0.2.4","product":"FM-1_80024","filename":"unholyunion-permutation-0.2.4.fwsc","bytes":610567,"sha256":"bdd68bc54b39be1f0ef3e97a2586af65a2bb9b14a9a6b27cf6612ff3a02c16f2"});
export const offering=Object.freeze({title:'Apocryphal',package:pkg,
 release:Object.freeze({name:'UnholyUnion '+pkg.version,available:true,targetValidated:false,product:pkg.product,sha256:pkg.sha256,bytes:pkg.bytes,filename:pkg.filename,
 supportedStartingIdentities:startingIdentities,
 experimentalApproval:Object.freeze({enabled:true,build:pkg.build,model:'FM-1',supportedStartingIdentities:startingIdentities})})});
