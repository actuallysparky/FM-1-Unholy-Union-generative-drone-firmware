// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {InstallerState} from './unholy-controller.js';
import {release,firstTestRelease,bundledPackage} from './unholy-release.js';
const pkg={product:bundledPackage.product,sha256:bundledPackage.sha256,bytes:bundledPackage.bytes};
assert.equal(release.available,false);assert.equal(release.targetValidated,false);assert.equal(firstTestRelease.targetValidated,false);
for(const r of [release,firstTestRelease]){const s=new InstallerState(r);s.connect({model:'FM-1',text:'FM-1_014'});assert.throws(()=>s.load(pkg));s.backup=s.consent=true;assert.equal(s.ready,false);}
const s=new InstallerState(firstTestRelease,{experimental:true});assert.equal(s.phase,'idle');assert.equal(s.ready,false);s.connect({model:'FM-1',text:'FM-1_014'});s.load(pkg);assert.equal(s.phase,'validated');assert.equal(s.ready,false);s.backup=true;assert.equal(s.ready,false);s.consent=true;assert.equal(s.ready,true);assert.equal(s.busy,false);
for(const key of ['product','sha256','bytes'])assert.throws(()=>s.load({...pkg,[key]:key==='bytes'?1:'bad'}));
s.load(pkg);s.package.bytes=1;assert.equal(s.ready,false);s.load({...pkg,bytes:bundledPackage.bytes});s.start();assert.equal(s.phase,'starting');assert.throws(()=>s.cancel());
for(const id of [{model:'FM-1',text:'FM-1_015'},{model:'ota-FM-1',text:'ota-FM-1'},{model:'other',text:'FM-1_014'}]){const t=new InstallerState(firstTestRelease,{experimental:true});t.connect(id);assert.equal(t.compatible,false);assert.throws(()=>t.load({...pkg,bytes:bundledPackage.bytes}));t.backup=t.consent=true;assert.equal(t.ready,false);}
const disabled=new InstallerState({...firstTestRelease,experimentalApproval:{...firstTestRelease.experimentalApproval,enabled:false}},{experimental:true});assert.equal(disabled.authorized,false);
console.log('Experimental gating passed: public lock; exact identity; pinned product/digest/size; both acknowledgements; no automatic start; explicit start only; no target-validation claim.');
