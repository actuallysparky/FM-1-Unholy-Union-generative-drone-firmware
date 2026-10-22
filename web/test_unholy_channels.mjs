// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {selectChannel,defaultChannel} from './unholy-channels.js';
import {InstallerState} from './unholy-controller.js';
const stable=selectChannel('stable'),experiment=selectChannel('experimental');
assert.equal(stable.package,null);assert.equal(stable.release.available,false);
assert.equal(defaultChannel(),'experimental');assert.throws(()=>selectChannel('__proto__'));
const state=new InstallerState(experiment.release,{experimental:true});
assert.equal(state.authorized,true);assert.equal(experiment.release.targetValidated,false);
state.connect({model:'FM-1',text:'FM-1_014'});
state.load({product:experiment.package.product,sha256:experiment.package.sha256,bytes:experiment.package.bytes});
assert.equal(state.ready,true);
const upgrade=new InstallerState(experiment.release,{experimental:true});upgrade.connect({model:'FM-1',text:'FM-1_80010'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80011'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80012'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80013'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80014'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80015'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80016'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80017'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80018'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80019'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80020'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80021'});assert.equal(upgrade.compatible,true);upgrade.connect({model:'FM-1',text:'FM-1_80022'});assert.equal(upgrade.compatible,true);
const stockV15=new InstallerState(experiment.release,{experimental:true});stockV15.connect({model:'FM-1',text:'FM-1_015'});stockV15.load({product:experiment.package.product,sha256:experiment.package.sha256,bytes:experiment.package.bytes});assert.equal(stockV15.ready,true);assert.equal(stockV15.busy,false);
for(const text of ['FM-1_016','FM-1_0150',experiment.package.product]){const other=new InstallerState(experiment.release,{experimental:true});other.connect({model:'FM-1',text});assert.equal(other.compatible,false);}
state.start();assert.throws(()=>state.cancel());

console.log('Local channel catalog pins and starting-identity restrictions pass. Public offering is tested separately.');
