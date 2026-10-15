// SPDX-License-Identifier: GPL-3.0-only
// Public offering is pinned independently of the local experimental catalog.
const startingIdentities=Object.freeze(['FM-1_014',...Array.from({length:14},(_,i)=>'FM-1_'+(80010+i))]);
const pkg=Object.freeze({version:'0.2.3',build:'permutation-0.2.3',product:'FM-1_80023',filename:'unholyunion-permutation-0.2.3.fwsc',bytes:610567,sha256:'025bec05d71da1b106ecb05d52e165ea6592812e92209cfe2626278047f7abba'});
export const offering=Object.freeze({title:'Apocryphal',package:pkg,
 release:Object.freeze({name:'UnholyUnion '+pkg.version,available:true,targetValidated:false,product:pkg.product,sha256:pkg.sha256,bytes:pkg.bytes,filename:pkg.filename,
 supportedStartingIdentities:startingIdentities,
 experimentalApproval:Object.freeze({enabled:true,build:pkg.build,model:'FM-1',supportedStartingIdentities:startingIdentities})})});
