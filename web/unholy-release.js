// SPDX-License-Identifier: GPL-3.0-only
// Private a3e780b binary passed static build/package checks. Hardware approval and release pinning remain pending.
export const release = Object.freeze({name:'UnholyUnion 0.1.0 experimental',product:'FM-1_80010',available:false,targetValidated:false,sha256:null,bytes:null,filename:null});

// Static package provenance is separate from hardware/release authorization.
export const bundledPackage=Object.freeze({version:'0.1.0 experimental',build:'a3e780b',product:'FM-1_80010',filename:'unholyunion-a3e780b.fwsc',bytes:610086,sha256:'0f0685775ba9e0a00b1bfb03c1a3c70b188a712d0752689d0a63ae93cb38f517'});

// Sparky approved only this first stock-V14 experiment; this is not a public release.
export const firstTestRelease=Object.freeze({...release,sha256:bundledPackage.sha256,bytes:bundledPackage.bytes,filename:bundledPackage.filename,experimentalApproval:Object.freeze({enabled:true,build:bundledPackage.build,deviceIdentity:'FM-1_014',model:'FM-1'})});
